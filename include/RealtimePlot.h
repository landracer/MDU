/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file RealtimePlot.h
 * @brief Qwt-backed PlotEngine: multiple color-coded series, scrolling time axis,
 *        pause/zoom review mode, click markers, legend customization.
 *
 * All sample storage and decimation live in the pure PlotDataModel; this class
 * is a render adapter. Non-Qwt builds fall back to a latest-value label display.
 */
#ifndef REALTIMEPLOT_H
#define REALTIMEPLOT_H

#include "PlotEngine.h"
#include "PlotDataModel.h"
#include <QTimer>
#include <QElapsedTimer>
#include <QList>
#include <QLabel>
#include <QScrollArea>

#ifdef ENABLE_QWT_PLOTTING
#include <qwt_plot.h>
class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotPicker;
class QwtPlotPanner;
class QwtPlotMagnifier;
class QwtPlotMarker;
#endif

class RealtimePlot : public PlotEngine {
    Q_OBJECT

public:
    explicit RealtimePlot(QWidget* parent = nullptr);
    ~RealtimePlot() override;

    bool addSeries(DataPointID id, const QColor& color = QColor(),
                   const QString& source = QString()) override;
    void removeSeries(DataPointID id) override;
    bool hasSeries(DataPointID id) const override;

    void updateValue(DataPointID id, double value, const QDateTime& timestamp) override;
    void clear() override;

    void setTimeWindow(double seconds) override;
    double timeWindow() const { return m_timeWindow; }
    void setTitle(const QString& title) override;
    void setYLabel(const QString& label) override;

    void setPaused(bool paused) override;
    bool isPaused() const override { return m_paused; }
    void setShowAllHistory(bool showAll) override;
    bool showAllHistory() const override { return m_showAll; }
    void clearMarkers() override;

    void setXAxis(DataPointID id) override;
    DataPointID xAxis() const override { return m_model.xAxis(); }

#ifdef ENABLE_QWT_PLOTTING
private slots:
    void onRefreshTimer();
    void onLegendClicked(const QVariant& itemInfo, int index);
    void onPickerSelected(const QPointF& pos);
    void removeLastMarker();
#endif

protected:
#ifdef ENABLE_QWT_PLOTTING
    bool eventFilter(QObject* obj, QEvent* event) override;
#endif

private:
    void setupUi();
    QColor nextPaletteColor();
#ifdef ENABLE_QWT_PLOTTING
    void renderPass(bool forceRender = false);
    void applyStoredCustomization(DataPointID id, QwtPlotCurve* curve);
    QString valuesAtTime(double t) const;
#else
    QHash<DataPointID, QLabel*> m_labels;
    QScrollArea* m_scroll = nullptr;
#endif

    PlotDataModel m_model;            // pure storage + algorithms (unit-tested)
    QHash<DataPointID, QColor> m_colors;   // per-series display color (palette-deduped)
    double m_timeWindow = 30.0;
    bool m_paused = false;
    bool m_showAll = false;
    bool m_dataDirty = false;
    int m_nextColorIdx = 0;
    QTimer* m_refreshTimer = nullptr;
    QElapsedTimer m_clock;

#ifdef ENABLE_QWT_PLOTTING
    QwtPlot* m_plot = nullptr;        // internal plot canvas (child widget)
    QHash<DataPointID, QwtPlotCurve*> m_curves;
    QwtPlotPicker* m_picker = nullptr;
    QwtPlotPanner* m_panner = nullptr;
    QwtPlotMagnifier* m_magnifier = nullptr;
    QList<QwtPlotMarker*> m_markers;
    QLabel* m_emptyHint = nullptr;    // shown until the first series arrives
#endif
};

#endif // REALTIMEPLOT_H