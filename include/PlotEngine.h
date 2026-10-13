/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file PlotEngine.h
 * @brief Engine-agnostic plot widget interface.
 *
 * Everything the dashboard needs from "a plot" — declared here so the current
 * backend (RealtimePlot over Qwt) can be swapped for a matplot++-backed engine
 * without touching callers. All value traffic is in canonical units; engines
 * apply UnitConverter display transforms themselves.
 */
#ifndef PLOT_ENGINE_H
#define PLOT_ENGINE_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QDateTime>
#include "UnifiedDataPoint.h"

class PlotEngine : public QWidget {
    Q_OBJECT

public:
    explicit PlotEngine(QWidget* parent = nullptr) : QWidget(parent) {}
    ~PlotEngine() override = default;

    // ── series management ──
    virtual bool addSeries(DataPointID id, const QColor& color = QColor(),
                           const QString& source = QString()) = 0;
    virtual void removeSeries(DataPointID id) = 0;
    virtual bool hasSeries(DataPointID id) const = 0;

    // ── data intake ──
    virtual void updateValue(DataPointID id, double value, const QDateTime& timestamp) = 0;
    virtual void clear() = 0;

    // ── view ──
    virtual void setTimeWindow(double seconds) = 0;   // live scrolling window width
    virtual void setTitle(const QString& title) = 0;
    virtual void setYLabel(const QString& label) = 0;

    // ── review mode ──
    virtual void setPaused(bool paused) = 0;           // freeze axes for pan/zoom review
    virtual bool isPaused() const = 0;
    virtual void setShowAllHistory(bool showAll) = 0;  // "crunch" full session into view
    virtual bool showAllHistory() const = 0;
    virtual void clearMarkers() = 0;

    // ── XY mode: plot series against another DP's value instead of time ──
    virtual void setXAxis(DataPointID id) = 0;         // Invalid = time axis
    virtual DataPointID xAxis() const = 0;

signals:
    /** One-line summary of the values at the last clicked point */
    void pointMarked(const QString& summary);
    void pausedChanged(bool paused);
    void viewChanged();
};

#endif // PLOT_ENGINE_H