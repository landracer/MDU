/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file PlotDataModel.h
 * @brief Pure plotting data layer: per-series sample rings + window/decimation/XY algorithms.
 *
 * Deliberately free of Qwt/QWidget so every behavior (ring eviction, min/max decimation,
 * time-window slicing, XY-mode pairing) is deterministically unit-testable. RealtimePlot is
 * a thin render adapter over this model.
 */
#ifndef PLOT_DATA_MODEL_H
#define PLOT_DATA_MODEL_H

#include "UnifiedDataPoint.h"
#include <QHash>
#include <QPointF>
#include <QVector>
#include <QString>

class PlotDataModel {
public:
    // Hard memory-safety cap per series (~1.8 h of 30 Hz data); oldest samples drop first.
    static constexpr int kCapacity = 200000;

    // ── series management ──
    void addSeries(DataPointID id, const QString& unit);
    void removeSeries(DataPointID id);
    bool contains(DataPointID id) const { return m_series.contains(id); }
    QVector<DataPointID> ids() const;
    QString unit(DataPointID id) const;
    int count(DataPointID id) const;

    // ── sample intake (auto-creates an unknown series with empty unit) ──
    void append(DataPointID id, double t, double v);
    void setUnit(DataPointID id, const QString& unit);  // change display-family key
    void clearData();          // keep series, drop all samples
    bool isEmpty() const;

    // ── XY mode: X axis = another DP's value instead of time ──
    void setXAxis(DataPointID id);            // Invalid (default) = time on X
    DataPointID xAxis() const { return m_xAxis; }
    bool isXyMode() const { return m_xAxis != DataPointID::Invalid; }

    // ── render pipeline (pure, unit-tested) ──
    // Time mode: samples with x in [x0, x1] (plus one point before the edge),
    // min/max-decimated into `buckets` buckets.
    // XY mode: pairs (xAxis-value at sample time, own value) — window/buckets ignored.
    QVector<QPointF> renderPoints(DataPointID id, double x0, double x1, int buckets) const;

    // ── primitives (exposed for tests) ──
    static int firstIndexAtOrAfter(const QVector<QPointF>& pts, double t);
    static QVector<QPointF> decimateMinMax(const QVector<QPointF>& pts,
                                           double x0, double x1, int buckets);
    static double sampleAt(const QVector<QPointF>& pts, double t);  // NaN if empty/NaN input

    const QVector<QPointF>& samples(DataPointID id) const;  // canonical (t, value) pairs

private:
    struct Series {
        QVector<QPointF> samples;  // x = monotonic seconds, y = canonical value
        QString unit;              // canonical unit for display conversion upstream
    };
    QHash<DataPointID, Series> m_series;
    DataPointID m_xAxis = DataPointID::Invalid;
};

#endif // PLOT_DATA_MODEL_H