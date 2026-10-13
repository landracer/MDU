/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file PlotDataModel.cpp
 * @brief Pure plot storage + decimation + XY pairing (no Qwt, no widgets).
 */
#include "PlotDataModel.h"
#include <QtNumeric>

// ── series management ──

void PlotDataModel::addSeries(DataPointID id, const QString& unit) {
    if (m_series.contains(id)) return;
    Series s;
    s.unit = unit;
    s.samples.reserve(2048);
    m_series.insert(id, std::move(s));
}

void PlotDataModel::removeSeries(DataPointID id) {
    m_series.remove(id);
}

QVector<DataPointID> PlotDataModel::ids() const {
    QVector<DataPointID> out;
    out.reserve(m_series.size());
    for (auto it = m_series.constBegin(); it != m_series.constEnd(); ++it)
        out.append(it.key());
    return out;
}

QString PlotDataModel::unit(DataPointID id) const {
    auto it = m_series.constFind(id);
    return it == m_series.constEnd() ? QString() : it->unit;
}

int PlotDataModel::count(DataPointID id) const {
    auto it = m_series.constFind(id);
    return it == m_series.constEnd() ? 0 : it->samples.size();
}

void PlotDataModel::append(DataPointID id, double t, double v) {
    auto it = m_series.find(id);
    if (it == m_series.end()) {
        addSeries(id, QString());
        it = m_series.find(id);
    }
    it->samples.append(QPointF(t, v));
    if (it->samples.size() > kCapacity) {
        // Oldest samples drop first; single memmove-style trim, not per-sample.
        it->samples.remove(0, it->samples.size() - kCapacity);
    }
}

void PlotDataModel::clearData() {
    for (auto it = m_series.begin(); it != m_series.end(); ++it)
        it->samples.clear();
}

void PlotDataModel::setUnit(DataPointID id, const QString& unit) {
    auto it = m_series.find(id);
    if (it != m_series.end()) it->unit = unit;
}

bool PlotDataModel::isEmpty() const {
    for (auto it = m_series.constBegin(); it != m_series.constEnd(); ++it)
        if (!it->samples.isEmpty()) return false;
    return true;
}

void PlotDataModel::setXAxis(DataPointID id) {
    m_xAxis = id;
}

const QVector<QPointF>& PlotDataModel::samples(DataPointID id) const {
    static const QVector<QPointF> kEmpty;
    auto it = m_series.constFind(id);
    return it == m_series.constEnd() ? kEmpty : it->samples;
}

// ── primitives ──

int PlotDataModel::firstIndexAtOrAfter(const QVector<QPointF>& pts, double t) {
    int lo = 0, hi = pts.size();
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (pts[mid].x() < t) lo = mid + 1; else hi = mid;
    }
    return lo;
}

QVector<QPointF> PlotDataModel::decimateMinMax(const QVector<QPointF>& pts,
                                               double x0, double x1, int buckets) {
    // Min/max bucket decimation: one bucket keeps its extreme points in
    // chronological order, so every spike survives at <=2 points/bucket.
    QVector<QPointF> out;
    if (pts.isEmpty() || buckets <= 0 || !(x1 > x0)) return out;

    const double w = (x1 - x0) / buckets;
    QVector<double> lo(buckets, qQNaN()), hi(buckets, qQNaN());
    QVector<double> tLo(buckets, 0.0), tHi(buckets, 0.0);

    for (const QPointF& p : pts) {
        int b = int((p.x() - x0) / w);
        if (b < 0) b = 0;
        if (b > buckets - 1) b = buckets - 1;
        if (qIsNaN(lo[b]) || p.y() < lo[b]) { lo[b] = p.y(); tLo[b] = p.x(); }
        if (qIsNaN(hi[b]) || p.y() > hi[b]) { hi[b] = p.y(); tHi[b] = p.x(); }
    }
    for (int b = 0; b < buckets; b++) {
        if (qIsNaN(lo[b])) continue;  // empty bucket
        if (qFuzzyCompare(tLo[b], tHi[b]) || qIsNaN(hi[b])) {
            out.append(QPointF(tLo[b], lo[b]));
        } else if (tLo[b] < tHi[b]) {
            out.append(QPointF(tLo[b], lo[b]));
            out.append(QPointF(tHi[b], hi[b]));
        } else {
            out.append(QPointF(tHi[b], hi[b]));
            out.append(QPointF(tLo[b], lo[b]));
        }
    }
    return out;
}

double PlotDataModel::sampleAt(const QVector<QPointF>& pts, double t) {
    // Linear interpolation on (monotonic-x) samples; clamp outside the range.
    const int n = pts.size();
    if (n == 0) return qQNaN();
    if (t <= pts.first().x()) return pts.first().y();
    if (t >= pts.last().x()) return pts.last().y();
    const int i = firstIndexAtOrAfter(pts, t);  // i in [1, n-1]
    const QPointF& a = pts[i - 1];
    const QPointF& b = pts[i];
    if (qFuzzyCompare(a.x(), b.x())) return b.y();
    return a.y() + (b.y() - a.y()) * (t - a.x()) / (b.x() - a.x());
}

// ── render pipeline ──

QVector<QPointF> PlotDataModel::renderPoints(DataPointID id, double x0, double x1, int buckets) const {
    auto it = m_series.constFind(id);
    if (it == m_series.constEnd()) return {};
    const QVector<QPointF>& pts = it->samples;
    if (pts.isEmpty()) return {};

    if (isXyMode()) {
        // X = the selected series' value at each of our sample times. If the
        // X series has no data yet, nothing is plottable.
        auto xit = m_series.constFind(m_xAxis);
        if (xit == m_series.constEnd() || xit->samples.isEmpty()) return {};
        const QVector<QPointF>& xs = xit->samples;
        QVector<QPointF> out;
        out.reserve(pts.size());
        for (const QPointF& p : pts) {
            const double xv = sampleAt(xs, p.x());
            if (!qIsNaN(xv)) out.append(QPointF(xv, p.y()));
        }
        return out;
    }

    // Time mode: keep one sample left of the window edge so the trace enters
    // from the left instead of popping in.
    const int first = qMax(0, firstIndexAtOrAfter(pts, x0) - 1);
    QVector<QPointF> window;
    window.reserve(qMin(pts.size() - first, buckets > 0 ? 2 * buckets + 2 : pts.size()));
    for (int i = first; i < pts.size(); i++) window.append(pts[i]);

    // Cheap path: fewer raw points than buckets — no decimation needed.
    if (buckets > 0 && window.size() > 2 * buckets)
        return decimateMinMax(window, x0, x1, buckets);
    return window;
}