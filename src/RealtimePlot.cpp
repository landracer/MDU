/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file RealtimePlot.cpp
 * @brief PlotEngine implementations: Qwt-backed engine (real curves rendered over
 *        the pure PlotDataModel) and a no-Qwt latest-value-label fallback.
 */
#include "RealtimePlot.h"
#include "UnitConverter.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDebug>
#include <QtNumeric>
#include <algorithm>

#ifdef ENABLE_QWT_PLOTTING
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_legend.h>
#include <qwt_legend_data.h>
#include <qwt_picker_machine.h>
#include <qwt_text.h>
#include <QPen>
#include <QMenu>
#include <QCursor>
#include <QMouseEvent>
#include <QSettings>
#include <QInputDialog>
#include <QColorDialog>
#endif

// 20 visually distinct colors, chosen for a dark plot background.
static const QColor kSeriesPalette[] = {
    QColor(0, 255, 0), QColor(255, 165, 0), QColor(0, 150, 255), QColor(255, 255, 0),
    QColor(255, 0, 255), QColor(0, 255, 255), QColor(255, 80, 80), QColor(160, 120, 255),
    QColor(120, 255, 160), QColor(255, 190, 120), QColor(80, 200, 120), QColor(255, 120, 200),
    QColor(150, 220, 255), QColor(220, 220, 150), QColor(255, 100, 0), QColor(100, 255, 100),
    QColor(200, 160, 255), QColor(255, 220, 0), QColor(0, 220, 180), QColor(240, 240, 240),
};
static constexpr int kSeriesPaletteCount = int(sizeof(kSeriesPalette) / sizeof(kSeriesPalette[0]));

static QColor getSourceColorHelper(const QString& source) {
    if (source.contains("MD-ECU")) return QColor(0, 255, 0);
    if (source.contains("BMS")) return QColor(255, 165, 0);
    if (source.contains("ESP-NOW")) return QColor(0, 150, 255);
    if (source.contains("CSV")) return QColor(255, 255, 0);
    return Qt::white;
}

QColor RealtimePlot::nextPaletteColor() {
    auto inUse = [this](const QColor& c) {
        return std::any_of(m_colors.cbegin(), m_colors.cend(),
                           [&c](const QColor& v) { return v == c; });
    };
    for (int attempt = 0; attempt < kSeriesPaletteCount; ++attempt) {
        const int idx = (m_nextColorIdx + attempt) % kSeriesPaletteCount;
        const QColor& candidate = kSeriesPalette[idx];
        if (!inUse(candidate)) {
            m_nextColorIdx = (idx + 1) % kSeriesPaletteCount;
            return candidate;
        }
    }
    const QColor& c = kSeriesPalette[m_nextColorIdx];   // all in use — wrap around
    m_nextColorIdx = (m_nextColorIdx + 1) % kSeriesPaletteCount;
    return c;
}

// Shared: does this id already hold the exact hint color (or is hint taken)?
static bool hintColorTaken(const QHash<DataPointID, QColor>& colors, const QColor& hint) {
    return std::any_of(colors.cbegin(), colors.cend(),
                       [&hint](const QColor& v) { return v == hint; });
}

// ── engine API (backend-agnostic parts) ──

void RealtimePlot::setTimeWindow(double seconds) {
    m_timeWindow = qMax(1.0, seconds);
    m_dataDirty = true;
#ifdef ENABLE_QWT_PLOTTING
    m_plot->replot();
#endif
}

void RealtimePlot::setPaused(bool paused) {
    m_paused = paused;
#ifdef ENABLE_QWT_PLOTTING
    m_panner->setEnabled(paused);
    m_magnifier->setEnabled(paused);
    if (paused) renderPass(true);   // freeze: full session into view, axes fitted to it
#endif
    emit pausedChanged(paused);
}

void RealtimePlot::setShowAllHistory(bool showAll) {
    m_showAll = showAll;
    m_dataDirty = true;
}

void RealtimePlot::setXAxis(DataPointID id) {
    m_model.setXAxis(id);
    m_dataDirty = true;
#ifdef ENABLE_QWT_PLOTTING
    m_plot->setAxisTitle(QwtPlot::xBottom,
                         id == DataPointID::Invalid ? QStringLiteral("Time (s)")
                                                    : DataPointName(id));
    m_plot->replot();
#endif
}

void RealtimePlot::setTitle(const QString& title) {
#ifdef ENABLE_QWT_PLOTTING
    m_plot->setTitle(title);
    m_plot->replot();
#else
    Q_UNUSED(title);
#endif
}

void RealtimePlot::setYLabel(const QString& label) {
#ifdef ENABLE_QWT_PLOTTING
    m_plot->setAxisTitle(QwtPlot::yLeft, label);
    m_plot->replot();
#else
    Q_UNUSED(label);
#endif
}

void RealtimePlot::clear() {
    m_model.clearData();
#ifdef ENABLE_QWT_PLOTTING
    for (QwtPlotCurve* c : m_curves) c->setSamples(QVector<QPointF>());
    m_plot->replot();
#endif
    m_dataDirty = true;
}

void RealtimePlot::clearMarkers() {
#ifdef ENABLE_QWT_PLOTTING
    for (QwtPlotMarker* m : m_markers) { m->detach(); delete m; }
    m_markers.clear();
    m_plot->replot();
#endif
}


#ifdef ENABLE_QWT_PLOTTING

// ──────────────────────────────────────────────
// Qwt-backed engine
// ──────────────────────────────────────────────
RealtimePlot::RealtimePlot(QWidget* parent)
    : PlotEngine(parent)
    , m_refreshTimer(new QTimer(this))
{
    setupUi();
    m_clock.start();

    // Single refresh pass (~20 Hz): samples only mark dirty; one replot per tick.
    m_refreshTimer->setInterval(50);
    connect(m_refreshTimer, &QTimer::timeout, this, &RealtimePlot::onRefreshTimer);
    m_refreshTimer->start();
}

RealtimePlot::~RealtimePlot() {
    for (QwtPlotMarker* m : m_markers) { m->detach(); delete m; }
    m_markers.clear();
    for (QwtPlotCurve* c : m_curves) { c->detach(); delete c; }
    m_curves.clear();
}

void RealtimePlot::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_plot = new QwtPlot(this);
    m_plot->setTitle("MultiDisplay Unified — Real-Time Data");
    m_plot->setAxisTitle(QwtPlot::xBottom, "Time (s)");
    m_plot->setAxisTitle(QwtPlot::yLeft, "Value");
    m_plot->setPalette(QPalette(Qt::black));
    m_plot->canvas()->setPalette(QPalette(Qt::darkGray));
    m_plot->setAxisScale(QwtPlot::xBottom, 0, m_timeWindow);
    m_plot->setAxisScale(QwtPlot::yLeft, 0, 100);
    layout->addWidget(m_plot, 1);

    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::gray, 0, Qt::DotLine);
    grid->attach(m_plot);

    // Clickable legend: rename series / change color / cycle display unit
    QwtLegend* legend = new QwtLegend();
    legend->setDefaultItemMode(QwtLegendData::Clickable);
    m_plot->insertLegend(legend, QwtPlot::RightLegend);
    connect(legend, &QwtLegend::clicked, this, &RealtimePlot::onLegendClicked);

    // Point picker: crosshair tracker; left-click drops a marker, right-click
    // (eventFilter below) removes the last one.
    m_picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                 QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn,
                                 m_plot->canvas());
    m_picker->setStateMachine(new QwtPickerClickPointMachine());
    m_picker->setRubberBandPen(QColor(Qt::green));
    m_picker->setTrackerPen(QColor(Qt::white));
    connect(m_picker, qOverload<const QPointF&>(&QwtPlotPicker::selected),
            this, &RealtimePlot::onPickerSelected);
    m_plot->canvas()->installEventFilter(this);

    // Review navigation: middle-drag pan + wheel zoom (enabled while paused)
    m_panner = new QwtPlotPanner(m_plot->canvas());
    m_panner->setMouseButton(Qt::MiddleButton);
    m_panner->setEnabled(false);
    m_magnifier = new QwtPlotMagnifier(m_plot->canvas());
    m_magnifier->setMouseButton(Qt::NoButton);  // wheel only
    m_magnifier->setEnabled(false);

    // Re-render when a display unit changes anywhere in the app (°C→°F, ...)
    connect(&UnitConverter::instance(), &UnitConverter::unitsChanged,
            this, [this](const QString&) {
        m_dataDirty = true;
        if (m_paused) renderPass(true);
    });
}

void RealtimePlot::applyStoredCustomization(DataPointID id, QwtPlotCurve* curve) {
    QSettings settings;
    const QString key = QStringLiteral("plot/dp_%1")
        .arg(static_cast<uint32_t>(id), 4, 16, QLatin1Char('0'));

    const QString customName = settings.value(key + "/name").toString();
    if (!customName.isEmpty()) curve->setTitle(customName);

    const QString customColor = settings.value(key + "/color").toString();
    if (!customColor.isEmpty()) {
        QColor c(customColor);
        if (c.isValid()) {
            m_colors[id] = c;
            curve->setPen(c, 2);
        }
    }
}

bool RealtimePlot::addSeries(DataPointID id, const QColor& color, const QString& source) {
#ifdef ENABLE_QWT_PLOTTING
    if (m_model.contains(id)) return true;

    QColor unique = color;
    if (unique.isValid() && hintColorTaken(m_colors, unique)) unique = QColor();
    const QColor chosen = unique.isValid() ? unique : nextPaletteColor();

    QwtPlotCurve* curve = new QwtPlotCurve(DataPointName(id));
    curve->setPen(chosen, 2);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    // No per-point symbols: at 30 Hz symbols on every sample kill paint perf.
    curve->attach(m_plot);

    m_model.addSeries(id, DataPointUnit(id));
    m_curves[id] = curve;
    m_colors[id] = chosen;
    applyStoredCustomization(id, curve);
    m_plot->updateLegend();
    return true;
#else
    Q_UNUSED(source);
    if (m_model.contains(id)) return true;
    m_model.addSeries(id, DataPointUnit(id));
    QLabel* label = new QLabel(QStringLiteral("%1: -- %2")
                                   .arg(DataPointName(id), DataPointUnit(id)), this);
    label->setStyleSheet(QStringLiteral("color: #%1; font-size: 14px; padding: 8px;"
                                        "background-color: #2a2a2a; border-radius: 4px;")
                             .arg(getSourceColorHelper(source).name().mid(1)));
    m_labels[id] = label;
    if (m_scroll && m_scroll->widget() && m_scroll->widget()->layout())
        static_cast<QVBoxLayout*>(m_scroll->widget()->layout())
            ->insertWidget(m_scroll->widget()->layout()->count() - 1, label);
    return true;
#endif
}

void RealtimePlot::removeSeries(DataPointID id) {
    if (!m_model.contains(id)) return;
#ifdef ENABLE_QWT_PLOTTING
    auto it = m_curves.find(id);
    if (it != m_curves.end()) {
        it.value()->detach();
        delete it.value();
        m_curves.erase(it);
        m_plot->updateLegend();
    }
#else
    auto it = m_labels.find(id);
    if (it != m_labels.end()) { it.value()->deleteLater(); m_labels.erase(it); }
#endif
    m_colors.remove(id);
    m_model.removeSeries(id);
    m_dataDirty = true;
}

bool RealtimePlot::hasSeries(DataPointID id) const {
    return m_model.contains(id);
}

void RealtimePlot::updateValue(DataPointID id, double value, const QDateTime& timestamp) {
    Q_UNUSED(timestamp);  // Callers pass "now"; the monotonic clock is the time base.

#ifdef ENABLE_QWT_PLOTTING
    if (!m_model.contains(id)) addSeries(id, QColor());
    // X = seconds since plot creation (monotonic); ring eviction lives in the model.
    m_model.append(id, m_clock.elapsed() / 1000.0, value);
    m_dataDirty = true;
    // No replot here — onRefreshTimer batches all series into one replot per tick.
#else
    auto it = m_labels.constFind(id);
    if (it == m_labels.constEnd()) { addSeries(id, QColor(), QString()); it = m_labels.constFind(id); }
    if (it == m_labels.constEnd()) return;
    m_model.append(id, 0.0, value);
    const QString fmt = (id == DataPointID::RPM) ? QStringLiteral("%1").arg(static_cast<int>(value))
                          : (id == DataPointID::ThrottlePosition || id == DataPointID::IntakeTemp)
                              ? QStringLiteral("%1\u00b0").arg(value, 0, 'f', 1)
                              : QStringLiteral("%1").arg(value, 0, 'f', 2);
    it.value()->setText(QStringLiteral("%1: %2 %3").arg(DataPointName(id), fmt, DataPointUnit(id)));
#endif
}


// ── Qwt render pass (batched, decimated) ──

void RealtimePlot::renderPass(bool forceRender) {
    Q_UNUSED(forceRender);
    const bool timeMode = !m_model.isXyMode();
    const double now = m_clock.elapsed() / 1000.0;
    const double x0 = (m_showAll || m_paused) ? 0.0 : now - m_timeWindow;
    const int buckets = qMax(64, int(m_plot->canvas()->width()));

    auto& conv = UnitConverter::instance();
    const DataPointID xId = m_model.xAxis();
    const QString ux = timeMode ? QString() : m_model.unit(xId);
    const double xa = timeMode ? 1.0 : conv.toDisplay(ux, 1.0) - conv.toDisplay(ux, 0.0);
    const double xb = timeMode ? 0.0 : conv.toDisplay(ux, 0.0);

    bool hasData = false;
    double yMin = HUGE_VAL, yMax = -HUGE_VAL, xMin = HUGE_VAL, xMax = -HUGE_VAL;

    for (const DataPointID id : m_model.ids()) {
        QwtPlotCurve* curve = m_curves.value(id, nullptr);
        if (!curve) continue;

        // Points stay canonical in the model; convert to the display unit only here.
        const QString uy = m_model.unit(id);
        const double by = conv.toDisplay(uy, 0.0);
        const double ay = conv.toDisplay(uy, 1.0) - by;

        const QVector<QPointF> raw =
            m_model.renderPoints(id, x0, now, timeMode ? buckets : 0);
        QVector<QPointF> disp;
        disp.reserve(raw.size());
        for (const QPointF& p : raw) {
            const double vy = (ay == 1.0 && by == 0.0) ? p.y() : (ay * p.y() + by);
            double vx = p.x();
            if (!timeMode) vx = (xa == 1.0 && xb == 0.0) ? p.x() : (xa * p.x() + xb);
            disp.append(QPointF(vx, vy));
            if (timeMode && p.x() < x0) continue;  // left-edge continuity point excluded
            if (vy < yMin) yMin = vy;
            if (vy > yMax) yMax = vy;
            if (vx < xMin) xMin = vx;
            if (vx > xMax) xMax = vx;
            hasData = true;
        }
        curve->setSamples(disp);
    }

    if (timeMode) {
        // Scrolling window, pinned at 0 until the first window has elapsed.
        if (m_showAll || m_paused || now < m_timeWindow)
            m_plot->setAxisScale(QwtPlot::xBottom, 0, qMax(now, 1.0));
        else
            m_plot->setAxisScale(QwtPlot::xBottom, x0, now);
    } else if (xMax > xMin) {
        const double pad = (xMax - xMin) * 0.05;
        m_plot->setAxisScale(QwtPlot::xBottom, xMin - pad, xMax + pad);
    }

    if (hasData && yMax >= yMin) {
        double range = yMax - yMin;
        if (range < 0.1) range = 1.0;
        const double padding = range * 0.1;
        m_plot->setAxisScale(QwtPlot::yLeft, yMin - padding, yMax + padding);
    }
    m_plot->replot();
}

void RealtimePlot::onRefreshTimer() {
    // Review mode: axes and samples are frozen \u2014 the user is panning/zooming.
    // Data keeps buffering in the background; unit changes re-render directly.
    if (m_paused || !m_dataDirty) return;
    m_dataDirty = false;
    renderPass();
}

QString RealtimePlot::valuesAtTime(double t) const {
    QStringList lines;
    for (const DataPointID id : m_model.ids()) {
        const double v = PlotDataModel::sampleAt(m_model.samples(id), t);
        if (qIsNaN(v)) continue;
        const QString uy = m_model.unit(id);
        lines << QStringLiteral("%1: %2 %3")
                     .arg(DataPointName(id))
                     .arg(UnitConverter::instance().toDisplay(uy, v), 0, 'f', 2)
                     .arg(uy);
    }
    return lines.join('\n');
}

void RealtimePlot::onLegendClicked(const QVariant& itemInfo, int index) {
    Q_UNUSED(index);
    QwtPlotItem* plotItem = m_plot->infoToItem(itemInfo);
    if (!plotItem) return;

    DataPointID id = DataPointID::Invalid;
    QwtPlotCurve* curve = nullptr;
    for (auto it = m_curves.constBegin(); it != m_curves.constEnd(); ++it) {
        if (it.value() == plotItem) { id = it.key(); curve = it.value(); break; }
    }
    if (!curve) return;

    const QString key = QStringLiteral("plot/dp_%1")
        .arg(static_cast<uint32_t>(id), 4, 16, QLatin1Char('0'));

    QMenu menu;
    QAction* renameAction = menu.addAction("Rename\u2026");
    QAction* colorAction = menu.addAction("Change Color\u2026");

    // Unit cycling for convertible families (deg C->F, kPa->PSI->bar, lambda->AFR, ...)
    auto& conv = UnitConverter::instance();
    const QString family = m_model.unit(id);
    QAction* unitAction = nullptr;
    if (conv.isConvertible(family))
        unitAction = menu.addAction(QStringLiteral("Switch Unit: %1 -> %2")
                                       .arg(conv.displayUnit(family), conv.nextUnit(family)));

    QAction* resetAction = menu.addAction("Reset to Default");
    QAction* chosen = menu.exec(QCursor::pos());

    QSettings settings;
    if (chosen && chosen == unitAction) {
        m_model.setUnit(id, conv.cycleUnit(family));  // display-only; data stays canonical
        m_dataDirty = true;
        renderPass(true);
    } else if (chosen == renameAction) {
        bool ok = false;
        const QString name = QInputDialog::getText(
            this, "Rename Series", QStringLiteral("Name for %1:").arg(DataPointName(id)),
            QLineEdit::Normal, curve->title().text(), &ok);
        if (ok && !name.isEmpty()) {
            curve->setTitle(name);
            settings.setValue(key + "/name", name);
            m_plot->updateLegend();
            m_plot->replot();
        }
    } else if (chosen == colorAction) {
        const QColor c = QColorDialog::getColor(m_colors.value(id), this,
                                                QStringLiteral("Color for %1").arg(curve->title().text()));
        if (c.isValid()) {
            m_colors[id] = c;
            curve->setPen(c, 2);
            settings.setValue(key + "/color", c.name());
            m_plot->updateLegend();
            m_plot->replot();
        }
    } else if (chosen == resetAction) {
        settings.remove(key + "/name");
        settings.remove(key + "/color");
        curve->setTitle(DataPointName(id));
        const QColor c = nextPaletteColor();
        m_colors[id] = c;
        curve->setPen(c, 2);
        m_plot->updateLegend();
        m_plot->replot();
    }
}

void RealtimePlot::onPickerSelected(const QPointF& pos) {
    QString summary = m_model.isXyMode()
        ? QStringLiteral("x=%1  y=%2").arg(pos.x(), 0, 'f', 2).arg(pos.y(), 0, 'f', 2)
        : valuesAtTime(pos.x());

    QwtPlotMarker* m = new QwtPlotMarker();
    m->setLineStyle(QwtPlotMarker::VLine);
    m->setXValue(pos.x());
    m->setLinePen(QPen(Qt::white, 0, Qt::DashDotLine));
    // Alternate label placement so adjacent markers don't overlap (original behavior)
    m->setLabelOrientation(Qt::Horizontal);
    m->setLabelAlignment(m_markers.size() % 2 == 0
                             ? (Qt::AlignRight | Qt::AlignBottom)
                             : (Qt::AlignRight | Qt::AlignTop));

    QwtText label(summary);
    label.setColor(Qt::white);
    label.setBackgroundBrush(QBrush(QColor(0, 0, 0, 160)));
    m->setLabel(label);

    m->attach(m_plot);
    m_markers.push_back(m);
    m_plot->replot();

    emit pointMarked(summary.replace('\n', QStringLiteral("  |  ")));
}

void RealtimePlot::removeLastMarker() {
    if (m_markers.isEmpty()) return;
    QwtPlotMarker* m = m_markers.takeLast();
    m->detach();
    delete m;
    m_plot->replot();
}

bool RealtimePlot::eventFilter(QObject* obj, QEvent* event) {
    // Right-click on the canvas removes the last marker (original behavior)
    if (obj == m_plot->canvas() && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::RightButton) {
            removeLastMarker();
            return true;
        }
    }
    return PlotEngine::eventFilter(obj, event);
}

#else  // !ENABLE_QWT_PLOTTING — latest-value label fallback

RealtimePlot::RealtimePlot(QWidget* parent)
    : PlotEngine(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("MultiDisplay Unified", this);
    title->setAlignment(Qt::AlignCenter);
    title->setFont(QFont("Arial", 14, QFont::Bold));
    layout->addWidget(title);

    QLabel* sub = new QLabel("Qwt not available \u2014 showing latest values", this);
    sub->setAlignment(Qt::AlignCenter);
    sub->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(sub);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet("background-color: #1a1a1a; border: none;");
    QWidget* content = new QWidget(m_scroll);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setAlignment(Qt::AlignTop);
    QLabel* placeholder = new QLabel("Connect hardware to see data...", content);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color: #888; font-size: 12px; padding: 40px;");
    contentLayout->addWidget(placeholder);
    contentLayout->addStretch();
    m_scroll->setWidget(content);
    layout->addWidget(m_scroll);
}

RealtimePlot::~RealtimePlot() = default;

#endif // ENABLE_QWT_PLOTTING
// SPLIT_5