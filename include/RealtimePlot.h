/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef REALTIMEPLOT_H
#define REALTIMEPLOT_H

#include <QWidget>
#include <QColor>
#include <QHash>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QDateTime>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include "UnifiedDataPoint.h"

#ifdef ENABLE_QWT_PLOTTING
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
class QwtPlotPicker;
class QwtPlotPanner;
class QwtPlotMagnifier;
class QwtPlotMarker;
#endif

/**
 * @brief Real-time scrolling plot for unified data points
 * 
 * Displays multiple data series on a single plot with automatic scaling,
 * time-axis scrolling, and color-coded traces. Designed for 30Hz+ update rates.
 * 
 * When Qwt is available: Full-featured plotting with Qwt.
 * When Qwt is unavailable: Simple value display widget showing latest readings.
 */

#ifdef ENABLE_QWT_PLOTTING
class RealtimePlot : public QwtPlot
#else
class RealtimePlot : public QWidget
#endif
{
    Q_OBJECT

public:
    explicit RealtimePlot(QWidget* parent = nullptr);
    ~RealtimePlot() override;

    /**
     * @brief Add a data series to the plot
     */
    bool addSeries(DataPointID id, const QColor& color = Qt::white, const QString& source = QString());

    /**
     * @brief Remove a data series from the plot
     */
    void removeSeries(DataPointID id);

    /**
     * @brief Check if a series is plotted
     */
    bool hasSeries(DataPointID id) const;

    /**
     * @brief Set the time window (scroll range) in seconds
     */
    void setTimeWindow(double seconds);

    /**
     * @brief Get current time window
     */
    double timeWindow() const { return m_timeWindow; }

    /**
     * @brief Update with a new data point value
     */
    void updateValue(DataPointID id, double value, const QDateTime& timestamp);

    /**
     * @brief Clear all data and reset axes
     */
    void clear();

    /**
     * @brief Set plot title
     */
    void setTitle(const QString& title);

    /**
     * @brief Set Y-axis label
     */
    void setYLabel(const QString& label);

    /**
     * @brief Pause/resume review mode.
     *
     * Paused: axes freeze, full session history is loaded into the curves,
     * wheel zoom + middle-drag pan are enabled. Data intake continues in the
     * background — resuming snaps back to the live view with nothing lost.
     */
    void setPaused(bool paused);
    bool isPaused() const { return m_paused; }

    /**
     * @brief "Crunch the graph": show the whole session compacted into the
     * visible axis (x = 0..now) instead of the scrolling time window.
     */
    void setShowAllHistory(bool showAll);
    bool showAllHistory() const { return m_showAll; }

    /**
     * @brief Remove all click-markers from the plot
     */
    void clearMarkers();

signals:
    void viewChanged();
    void pausedChanged(bool paused);
    /** One-line summary of the values at the last clicked point */
    void pointMarked(const QString& summary);

private slots:
    void onRefreshTimer();
#ifdef ENABLE_QWT_PLOTTING
    void onLegendClicked(const QVariant& itemInfo, int index);
    void onPickerSelected(const QPointF& pos);
#endif

protected:
#ifdef ENABLE_QWT_PLOTTING
    bool eventFilter(QObject* obj, QEvent* event) override;
#endif

private:
#ifdef ENABLE_QWT_PLOTTING
    struct PlotSeries {
        QwtPlotCurve* curve = nullptr;
        QVector<QPointF> points;   // x = seconds since plot start (monotonic)
        QColor color;
        QString unit;              // canonical unit (values stored canonical)
    };

    QHash<DataPointID, PlotSeries*> m_series;
    QwtPlotGrid* m_grid = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QElapsedTimer m_clock;
    bool m_dataDirty = false;
    int m_nextColorIdx = 0;

    QwtPlotPicker* m_picker = nullptr;
    QwtPlotPanner* m_panner = nullptr;
    QwtPlotMagnifier* m_magnifier = nullptr;
    QList<QwtPlotMarker*> m_markers;
#else
    // Non-Qwt: Simple value display
    struct ValueDisplay {
        QLabel* label = nullptr;
        double lastValue = 0.0;
        QDateTime lastTime;
        QColor color;
        QString source;
    };

    QHash<DataPointID, ValueDisplay*> m_displays;
    QVBoxLayout* m_layout = nullptr;
    QScrollArea* m_scrollArea = nullptr;
#endif

    double m_timeWindow = 30.0;
    bool m_paused = false;
    bool m_showAll = false;

#ifdef ENABLE_QWT_PLOTTING
    void setupUI();
    QColor nextPaletteColor();
    void applyStoredCustomization(DataPointID id, PlotSeries* series);
    void rebuildSamplesFull();
    void removeLastMarker();
    QString valuesAtTime(double t) const;
#endif
};

#endif // REALTIMEPLOT_H
