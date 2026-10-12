/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "RealtimePlot.h"
#include "UnitConverter.h"
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QMenu>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QInputDialog>
#include <QColorDialog>

#ifdef ENABLE_QWT_PLOTTING
#include <qwt_symbol.h>
#include <qwt_plot_zoneitem.h>
#include <qwt_legend.h>
#include <qwt_legend_data.h>
#include <qwt_text.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_marker.h>
#endif

// ──────────────────────────────────────────────
// Helper function (shared by both Qwt and non-Qwt)
// ──────────────────────────────────────────────
static QColor getSourceColorHelper(const QString& source) {
    if (source.contains("MD-ECU")) return QColor(0, 255, 0);
    if (source.contains("BMS")) return QColor(255, 165, 0);
    if (source.contains("ESP-NOW")) return QColor(0, 150, 255);
    if (source.contains("CSV")) return QColor(255, 255, 0);
    
    return Qt::white;
}

// 20 visually distinct colors, chosen for a dark plot background.
// Every new series gets the next free palette entry so no two live
// streams on the same plot share a color.
static const QColor kSeriesPalette[] = {
    QColor(0, 255, 0),     // green
    QColor(255, 165, 0),   // orange
    QColor(0, 150, 255),   // azure
    QColor(255, 255, 0),   // yellow
    QColor(255, 0, 255),   // magenta
    QColor(0, 255, 255),   // cyan
    QColor(255, 80, 80),   // light red
    QColor(160, 120, 255), // violet
    QColor(120, 255, 160), // mint
    QColor(255, 190, 120), // peach
    QColor(80, 200, 120),  // emerald
    QColor(255, 120, 200), // pink
    QColor(150, 220, 255), // sky
    QColor(220, 220, 150), // khaki
    QColor(255, 100, 0),   // deep orange
    QColor(100, 255, 100), // pale green
    QColor(200, 160, 255), // lavender
    QColor(255, 220, 0),   // gold
    QColor(0, 220, 180),   // teal
    QColor(240, 240, 240), // near-white
};
static constexpr int kSeriesPaletteCount = int(sizeof(kSeriesPalette) / sizeof(kSeriesPalette[0]));

#ifdef ENABLE_QWT_PLOTTING
// Binary search: first index whose x >= t (points are appended in time order)
static int firstIndexAtOrAfter(const QVector<QPointF>& pts, double t) {
    int lo = 0, hi = pts.size();
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (pts[mid].x() < t) lo = mid + 1; else hi = mid;
    }
    return lo;
}
#endif

// ──────────────────────────────────────────────
// Qwt-enabled implementation
// ──────────────────────────────────────────────
#ifdef ENABLE_QWT_PLOTTING

RealtimePlot::RealtimePlot(QWidget* parent)
    : QwtPlot(parent)
    , m_refreshTimer(new QTimer(this))
{
    setupUI();

    m_clock.start();

    // Single refresh pass (~20Hz): scroll axes, trim old points, replot once.
    // Individual samples only append data; they never trigger a replot.
    m_refreshTimer->setInterval(50);
    connect(m_refreshTimer, &QTimer::timeout, this, &RealtimePlot::onRefreshTimer);
    m_refreshTimer->start();
}

RealtimePlot::~RealtimePlot() {
    // Curves and the grid are attached to this plot; QwtPlot's destructor
    // deletes all attached items (autoDelete). Only free our bookkeeping.
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        delete it.value();
    }
    m_series.clear();
}

void RealtimePlot::setupUI() {
    setTitle("MultiDisplay Unified — Real-Time Data");
    setAxisTitle(QwtPlot::xBottom, "Time (s)");
    setAxisTitle(QwtPlot::yLeft, "Value");
    
    m_grid = new QwtPlotGrid();
    m_grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    m_grid->setMinorPen(Qt::gray, 0, Qt::DotLine);
    m_grid->attach(this);
    
    setAxisScale(QwtPlot::xBottom, 0, m_timeWindow);
    setAxisScale(QwtPlot::yLeft, 0, 100);
    
    setPalette(QPalette(Qt::black));
    canvas()->setPalette(QPalette(Qt::darkGray));
    
    // Clickable legend: click a series entry to rename it or change its color
    QwtLegend* legend = new QwtLegend();
    legend->setDefaultItemMode(QwtLegendData::Clickable);
    insertLegend(legend, QwtPlot::RightLegend);
    connect(legend, &QwtLegend::clicked, this, &RealtimePlot::onLegendClicked);
    
    // Point picker (original MdPlotPicker behavior): crosshair tracker follows
    // the mouse, left-click drops a marker showing every series' value at that
    // instant, right-click removes the last marker.
    m_picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                                 QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn,
                                 canvas());
    m_picker->setStateMachine(new QwtPickerClickPointMachine());
    m_picker->setRubberBandPen(QColor(Qt::green));
    m_picker->setTrackerPen(QColor(Qt::white));
    connect(m_picker, qOverload<const QPointF&>(&QwtPlotPicker::selected),
            this, &RealtimePlot::onPickerSelected);
    canvas()->installEventFilter(this); // right-click = remove last marker
    
    // Review navigation: middle-drag pan + wheel zoom (enabled while paused)
    m_panner = new QwtPlotPanner(canvas());
    m_panner->setMouseButton(Qt::MiddleButton);
    m_panner->setEnabled(false);
    m_magnifier = new QwtPlotMagnifier(canvas());
    m_magnifier->setMouseButton(Qt::NoButton); // wheel only
    m_magnifier->setEnabled(false);
    
    // Re-render when a display unit changes anywhere in the app (°C→°F, ...)
    connect(&UnitConverter::instance(), &UnitConverter::unitsChanged,
            this, [this](const QString&) {
                m_dataDirty = true;
                if (m_paused) { rebuildSamplesFull(); replot(); }
            });
}

QColor RealtimePlot::nextPaletteColor() {
    // Pick the next palette color not already used by a series on this plot
    for (int attempt = 0; attempt < kSeriesPaletteCount; ++attempt) {
        const QColor& candidate = kSeriesPalette[(m_nextColorIdx + attempt) % kSeriesPaletteCount];
        bool taken = false;
        for (auto it = m_series.constBegin(); it != m_series.constEnd(); ++it) {
            if (it.value()->color == candidate) { taken = true; break; }
        }
        if (!taken) {
            m_nextColorIdx = (m_nextColorIdx + attempt + 1) % kSeriesPaletteCount;
            return candidate;
        }
    }
    // All colors in use — wrap around
    const QColor& c = kSeriesPalette[m_nextColorIdx];
    m_nextColorIdx = (m_nextColorIdx + 1) % kSeriesPaletteCount;
    return c;
}

void RealtimePlot::applyStoredCustomization(DataPointID id, PlotSeries* series) {
    QSettings settings;
    const QString key = QStringLiteral("plot/dp_%1")
        .arg(static_cast<uint32_t>(id), 4, 16, QLatin1Char('0'));
    
    const QString customName = settings.value(key + "/name").toString();
    if (!customName.isEmpty()) {
        series->curve->setTitle(customName);
    }
    
    const QString customColor = settings.value(key + "/color").toString();
    if (!customColor.isEmpty()) {
        QColor c(customColor);
        if (c.isValid()) {
            series->color = c;
            series->curve->setPen(series->color, 2);
        }
    }
}

bool RealtimePlot::addSeries(DataPointID id, const QColor& color, const QString& source) {
    Q_UNUSED(source);
    if (m_series.contains(id)) {
        return true;
    }
    
    PlotSeries* series = new PlotSeries();
    // Each stream gets a unique palette color; an explicit caller color is
    // only a hint and still deduplicated against colors already on the plot.
    QColor unique = color;
    if (unique.isValid()) {
        for (auto it = m_series.constBegin(); it != m_series.constEnd(); ++it) {
            if (it.value()->color == unique) { unique = QColor(); break; }
        }
    }
    series->color = unique.isValid() ? unique : nextPaletteColor();
    series->points.reserve(2048);
    
    series->curve = new QwtPlotCurve(DataPointName(id));
    series->curve->setPen(series->color, 2);
    series->curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    series->unit = DataPointUnit(id);
    // No per-point symbols: at 30Hz a symbol on every sample kills paint
    // performance and turns the trace into a smear.
    
    series->curve->attach(this);
    
    m_series[id] = series;
    
    // Restore any user rename/recolor from a previous session
    applyStoredCustomization(id, series);
    
    updateLegend();
    
    qDebug() << "[RealtimePlot] Added series:" << DataPointName(id) << "with color" << series->color.name();
    return true;
}

void RealtimePlot::removeSeries(DataPointID id) {
    auto it = m_series.find(id);
    if (it != m_series.end()) {
        if (it.value()->curve) {
            it.value()->curve->detach();
            delete it.value()->curve;   // detached items are no longer owned by the plot
        }
        delete it.value();
        m_series.erase(it);
        updateLegend();
        m_dataDirty = true;
        qDebug() << "[RealtimePlot] Removed series:" << DataPointName(id);
    }
}

bool RealtimePlot::hasSeries(DataPointID id) const {
    return m_series.contains(id);
}

void RealtimePlot::setTimeWindow(double seconds) {
    m_timeWindow = qMax(1.0, seconds);
    m_dataDirty = true;
}

void RealtimePlot::updateValue(DataPointID id, double value, const QDateTime& timestamp) {
    Q_UNUSED(timestamp); // Callers pass "now"; the monotonic clock is the time base.
    
    auto it = m_series.find(id);
    if (it == m_series.end()) {
        // Series not registered yet — add it automatically (color by DP category
        // is handled upstream in DashboardWidget; fall back to white here).
        addSeries(id, QColor());
        it = m_series.find(id);
        if (it == m_series.end()) {
            return;
        }
    }
    
    PlotSeries* series = it.value();
    
    // X = seconds since this plot was created (monotonic, sub-millisecond).
    // The refresh timer scrolls the visible axis window; the full session
    // history is retained so review mode can pan back through everything.
    const double now = m_clock.elapsed() / 1000.0;
    series->points.append(QPointF(now, value));
    
    // Hard cap as a memory safety net. 200k points ≈ 1.8 hours of 30Hz data
    // per series; oldest samples drop first.
    if (series->points.size() > 200000) {
        series->points.remove(0, series->points.size() - 200000);
    }
    
    m_dataDirty = true;
    // NOTE: no replot() here — onRefreshTimer() batches all series into one
    // replot per refresh tick. Per-sample replots at 30Hz froze the UI.
}

void RealtimePlot::clear() {
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        it.value()->points.clear();
        if (it.value()->curve) {
            it.value()->curve->setSamples(QVector<QPointF>());
        }
    }
    
    clearMarkers();
    
    setAxisScale(QwtPlot::xBottom, 0, m_timeWindow);
    setAxisScale(QwtPlot::yLeft, 0, 100);
    
    replot();
}

void RealtimePlot::setTitle(const QString& title) {
    QwtPlot::setTitle(title);
    replot();
}

void RealtimePlot::setYLabel(const QString& label) {
    setAxisTitle(QwtPlot::yLeft, label);
    replot();
}

void RealtimePlot::onLegendClicked(const QVariant& itemInfo, int index) {
    Q_UNUSED(index);
    
    QwtPlotItem* plotItem = infoToItem(itemInfo);
    if (!plotItem) return;
    
    // Find which series this curve belongs to
    DataPointID id = DataPointID::Invalid;
    PlotSeries* series = nullptr;
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        if (it.value()->curve == plotItem) {
            id = it.key();
            series = it.value();
            break;
        }
    }
    if (!series) return;
    
    const QString key = QStringLiteral("plot/dp_%1")
        .arg(static_cast<uint32_t>(id), 4, 16, QLatin1Char('0'));
    
    QMenu menu;
    QAction* renameAction = menu.addAction("Rename\u2026");
    QAction* colorAction = menu.addAction("Change Color\u2026");
    
    // Unit cycling for convertible families (°C→°F, kPa→PSI→bar, λ→AFR, ...)
    auto& conv = UnitConverter::instance();
    QAction* unitAction = nullptr;
    if (conv.isConvertible(series->unit)) {
        unitAction = menu.addAction(QStringLiteral("Switch Unit: %1 \u2192 %2")
            .arg(conv.displayUnit(series->unit), conv.nextUnit(series->unit)));
    }
    
    QAction* resetAction = menu.addAction("Reset to Default");
    QAction* chosen = menu.exec(QCursor::pos());
    
    QSettings settings;
    if (chosen && chosen == unitAction) {
        const QString newUnit = conv.cycleUnit(series->unit);
        qDebug() << "[RealtimePlot] Unit family" << series->unit << "switched to" << newUnit;
        m_dataDirty = true; // onRefreshTimer re-renders with the new unit
    } else if (chosen == renameAction) {
        bool ok = false;
        const QString current = series->curve->title().text();
        const QString name = QInputDialog::getText(this, "Rename Series",
            QStringLiteral("Name for %1:").arg(DataPointName(id)),
            QLineEdit::Normal, current, &ok);
        if (ok && !name.isEmpty()) {
            series->curve->setTitle(name);
            settings.setValue(key + "/name", name);
            updateLegend();
            replot();
        }
    } else if (chosen == colorAction) {
        const QColor newColor = QColorDialog::getColor(series->color, this,
            QStringLiteral("Color for %1").arg(series->curve->title().text()));
        if (newColor.isValid()) {
            series->color = newColor;
            series->curve->setPen(newColor, 2);
            settings.setValue(key + "/color", newColor.name());
            updateLegend();
            replot();
        }
    } else if (chosen == resetAction) {
        settings.remove(key + "/name");
        settings.remove(key + "/color");
        series->curve->setTitle(DataPointName(id));
        series->color = nextPaletteColor();
        series->curve->setPen(series->color, 2);
        updateLegend();
        replot();
    }
}

void RealtimePlot::onRefreshTimer() {
    // Review mode: axes and samples are frozen — the user is panning/zooming.
    // Data keeps buffering in the background; unit changes rebuild directly.
    if (m_paused) {
        return;
    }
    
    const double now = m_clock.elapsed() / 1000.0;
    const double windowStart = m_showAll ? 0.0 : (now - m_timeWindow);
    
    bool hasData = false;
    double globalMin = HUGE_VAL;
    double globalMax = -HUGE_VAL;
    
    auto& conv = UnitConverter::instance();
    
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        PlotSeries* series = it.value();
        
        // History is kept for review mode — only the *visible* slice is handed
        // to the curve. Keep one point left of the window edge so the trace
        // enters from the left instead of popping in.
        int first = 0;
        if (!m_showAll) {
            first = qMax(0, firstIndexAtOrAfter(series->points, windowStart) - 1);
        }
        
        // Points are stored in canonical units; convert to the selected
        // display unit (linear: y' = a·y + b) only when a conversion is active.
        const double b = conv.toDisplay(series->unit, 0.0);
        const double a = conv.toDisplay(series->unit, 1.0) - b;
        
        QVector<QPointF> displayPoints;
        displayPoints.reserve(series->points.size() - first);
        for (int i = first; i < series->points.size(); ++i) {
            const QPointF& p = series->points[i];
            const double y = (a == 1.0 && b == 0.0) ? p.y() : (a * p.y() + b);
            displayPoints.append(QPointF(p.x(), y));
            if (p.x() < windowStart) continue;
            if (y < globalMin) globalMin = y;
            if (y > globalMax) globalMax = y;
            hasData = true;
        }
        series->curve->setSamples(displayPoints);
    }
    
    // X axis: either the whole session compacted ("crunch") or a scrolling
    // window. Before the first window has elapsed, pin the left edge at 0.
    if (m_showAll || now < m_timeWindow) {
        setAxisScale(QwtPlot::xBottom, 0, m_showAll ? qMax(now, 1.0) : m_timeWindow);
    } else {
        setAxisScale(QwtPlot::xBottom, windowStart, now);
    }
    
    if (hasData) {
        double range = globalMax - globalMin;
        if (range < 0.1) range = 1.0;
        const double padding = range * 0.1;
        setAxisScale(QwtPlot::yLeft, globalMin - padding, globalMax + padding);
    }
    
    if (m_dataDirty || hasData) {
        m_dataDirty = false;
        replot();
    }
}

void RealtimePlot::rebuildSamplesFull() {
    // Load the entire buffered session into every curve (converted to display
    // units) and fit both axes around it, so review mode starts "crunched".
    auto& conv = UnitConverter::instance();
    double globalMin = HUGE_VAL;
    double globalMax = -HUGE_VAL;
    double maxX = 1.0;
    bool hasData = false;
    
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        PlotSeries* series = it.value();
        const double b = conv.toDisplay(series->unit, 0.0);
        const double a = conv.toDisplay(series->unit, 1.0) - b;
        
        QVector<QPointF> displayPoints;
        displayPoints.reserve(series->points.size());
        for (const QPointF& p : series->points) {
            const double y = (a == 1.0 && b == 0.0) ? p.y() : (a * p.y() + b);
            displayPoints.append(QPointF(p.x(), y));
            if (y < globalMin) globalMin = y;
            if (y > globalMax) globalMax = y;
            if (p.x() > maxX) maxX = p.x();
            hasData = true;
        }
        series->curve->setSamples(displayPoints);
    }
    
    setAxisScale(QwtPlot::xBottom, 0, maxX);
    if (hasData) {
        double range = globalMax - globalMin;
        if (range < 0.1) range = 1.0;
        const double padding = range * 0.1;
        setAxisScale(QwtPlot::yLeft, globalMin - padding, globalMax + padding);
    }
}

void RealtimePlot::setPaused(bool paused) {
    if (m_paused == paused) return;
    m_paused = paused;
    
    if (m_panner) m_panner->setEnabled(paused);
    if (m_magnifier) m_magnifier->setEnabled(paused);
    
    if (paused) {
        // Crunch the whole session into view for review
        rebuildSamplesFull();
        replot();
    } else {
        // Snap back to the live view on the next refresh tick
        m_dataDirty = true;
    }
    emit pausedChanged(paused);
}

void RealtimePlot::setShowAllHistory(bool showAll) {
    if (m_showAll == showAll) return;
    m_showAll = showAll;
    m_dataDirty = true;
}

QString RealtimePlot::valuesAtTime(double t) const {
    auto& conv = UnitConverter::instance();
    QStringList lines;
    
    const int mins = int(t) / 60;
    const double secs = t - mins * 60;
    lines << QStringLiteral("t = %1:%2").arg(mins).arg(secs, 0, 'f', 1);
    
    for (auto it = m_series.constBegin(); it != m_series.constEnd(); ++it) {
        const PlotSeries* series = it.value();
        if (!series->curve->isVisible() || series->points.isEmpty()) continue;
        
        // Linear interpolation between the two samples bracketing t
        const QVector<QPointF>& pts = series->points;
        const int idx = firstIndexAtOrAfter(pts, t);
        double y;
        if (idx <= 0) {
            y = pts.first().y();
        } else if (idx >= pts.size()) {
            y = pts.last().y();
        } else {
            const QPointF& l = pts[idx - 1];
            const QPointF& r = pts[idx];
            const double f = (r.x() > l.x()) ? (t - l.x()) / (r.x() - l.x()) : 0.0;
            y = l.y() + f * (r.y() - l.y());
        }
        
        lines << QStringLiteral("%1: %2 %3")
                     .arg(series->curve->title().text())
                     .arg(conv.toDisplay(series->unit, y), 0, 'f', 2)
                     .arg(conv.displayUnit(series->unit));
    }
    return lines.join('\n');
}

void RealtimePlot::onPickerSelected(const QPointF& pos) {
    const QString summary = valuesAtTime(pos.x());
    
    QwtPlotMarker* m = new QwtPlotMarker();
    m->setLineStyle(QwtPlotMarker::VLine);
    m->setXValue(pos.x());
    m->setLinePen(QPen(Qt::white, 0, Qt::DashDotLine));
    // Alternate label placement so adjacent markers don't overlap (original behavior)
    if (m_markers.size() % 2 == 0)
        m->setLabelAlignment(Qt::AlignRight | Qt::AlignBottom);
    else
        m->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
    m->setLabelOrientation(Qt::Horizontal);
    
    QwtText label(summary);
    label.setColor(Qt::white);
    label.setBackgroundBrush(QBrush(QColor(0, 0, 0, 160)));
    m->setLabel(label);
    
    m->attach(this);
    m_markers.push_back(m);
    replot();
    
    emit pointMarked(QString(summary).replace('\n', QStringLiteral("  |  ")));
}

void RealtimePlot::removeLastMarker() {
    if (m_markers.isEmpty()) return;
    QwtPlotMarker* m = m_markers.takeLast();
    m->detach();
    delete m;
    replot();
}

void RealtimePlot::clearMarkers() {
    for (QwtPlotMarker* m : m_markers) {
        m->detach();
        delete m;
    }
    m_markers.clear();
    replot();
}

bool RealtimePlot::eventFilter(QObject* obj, QEvent* event) {
    // Right-click on the canvas removes the last marker (original behavior)
    if (obj == canvas() && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::RightButton) {
            removeLastMarker();
            return true;
        }
    }
    return QwtPlot::eventFilter(obj, event);
}


// ──────────────────────────────────────────────
// Non-Qwt stub implementation
// ──────────────────────────────────────────────
#else // !ENABLE_QWT_PLOTTING

RealtimePlot::RealtimePlot(QWidget* parent)
    : QWidget(parent)
    , m_timeWindow(30.0)
{
    m_layout = new QVBoxLayout(this);
    
    QLabel* label = new QLabel("MultiDisplay Unified", this);
    label->setAlignment(Qt::AlignCenter);
    label->setFont(QFont("Arial", 14, QFont::Bold));
    m_layout->addWidget(label);
    
    QLabel* sublabel = new QLabel("Qwt not available — showing latest values", this);
    sublabel->setAlignment(Qt::AlignCenter);
    sublabel->setStyleSheet("color: gray; font-style: italic;");
    m_layout->addWidget(sublabel);
    
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background-color: #1a1a1a; border: none;");
    m_layout->addWidget(m_scrollArea);
    
    QWidget* scrollContent = new QWidget(m_scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setAlignment(Qt::AlignTop);
    
    // Add a placeholder label
    QLabel* placeholder = new QLabel("Connect hardware to see data...", scrollContent);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color: #888; font-size: 12px; padding: 40px;");
    scrollLayout->addWidget(placeholder);
    
    scrollLayout->addStretch();
    m_scrollArea->setWidget(scrollContent);
}

RealtimePlot::~RealtimePlot() {
    for (auto it = m_displays.begin(); it != m_displays.end(); ++it) {
        delete it.value();
    }
    m_displays.clear();
}

bool RealtimePlot::addSeries(DataPointID id, const QColor& color, const QString& source) {
    if (m_displays.contains(id)) {
        return true;
    }
    
    ValueDisplay* display = new ValueDisplay();
    display->color = color.isValid() ? color : getSourceColorHelper(source);
    display->source = source;
    
    QLabel* label = new QLabel(this);
    QString name = DataPointName(id);
    QString unit = DataPointUnit(id);
    label->setText(QString("%1: -- %2").arg(name, unit));
    label->setStyleSheet(QString(
        "color: #%1; font-size: 14px; padding: 8px; background-color: #2a2a2a; border-radius: 4px;"
    ).arg(display->color.name().mid(1)));
    
    display->label = label;
    m_displays[id] = display;
    
    // Insert before the placeholder (find last widget)
    // For simplicity, just add to scroll area content
    if (m_scrollArea && m_scrollArea->widget()) {
        QVBoxLayout* scrollLayout = qobject_cast<QVBoxLayout*>(m_scrollArea->widget()->layout());
        if (scrollLayout) {
            // Insert before the stretch spacer
            scrollLayout->insertWidget(scrollLayout->count() - 1, label);
        }
    }
    
    qDebug() << "[RealtimePlot] Added display:" << DataPointName(id);
    return true;
}

void RealtimePlot::removeSeries(DataPointID id) {
    auto it = m_displays.find(id);
    if (it != m_displays.end()) {
        if (it.value()->label) {
            it.value()->label->deleteLater();
        }
        delete it.value();
        m_displays.erase(it);
        qDebug() << "[RealtimePlot] Removed display:" << DataPointName(id);
    }
}

bool RealtimePlot::hasSeries(DataPointID id) const {
    return m_displays.contains(id);
}

void RealtimePlot::setTimeWindow(double seconds) {
    m_timeWindow = qMax(1.0, seconds);
}

void RealtimePlot::updateValue(DataPointID id, double value, const QDateTime& timestamp) {
    auto it = m_displays.find(id);
    if (it == m_displays.end()) {
        return;
    }
    
    ValueDisplay* display = it.value();
    display->lastValue = value;
    display->lastTime = timestamp;
    
    QString unit = DataPointUnit(id);
    QString name = DataPointName(id);
    
    // Format value based on data point type
    QString valueStr;
    if (id == DataPointID::RPM) {
        valueStr = QString("%1").arg(static_cast<int>(value));
    } else if (id == DataPointID::ThrottlePosition || id == DataPointID::IntakeTemp) {
        valueStr = QString("%1°").arg(value, 0, 'f', 1);
    } else {
        valueStr = QString("%1").arg(value, 0, 'f', 2);
    }
    
    display->label->setText(QString("%1: %2 %3").arg(name, valueStr, unit));
}

void RealtimePlot::clear() {
    for (auto it = m_displays.begin(); it != m_displays.end(); ++it) {
        if (it.value()->label) {
            it.value()->label->setText(QString("%1: -- %2").arg(DataPointName(it.key()), DataPointUnit(it.key())));
        }
    }
}

void RealtimePlot::setTitle(const QString& title) {
    // Title is already set in constructor for non-Qwt version
    Q_UNUSED(title);
}

void RealtimePlot::setYLabel(const QString& label) {
    Q_UNUSED(label);
}

void RealtimePlot::onRefreshTimer() {
    // No-op for non-Qwt version
}

void RealtimePlot::setPaused(bool paused) {
    m_paused = paused;
    emit pausedChanged(paused);
}

void RealtimePlot::setShowAllHistory(bool showAll) {
    m_showAll = showAll;
}

void RealtimePlot::clearMarkers() {
    // No-op for non-Qwt version
}

// Non-Qwt version doesn't need getSourceColor as a member
// It uses the helper function instead

#endif // ENABLE_QWT_PLOTTING
