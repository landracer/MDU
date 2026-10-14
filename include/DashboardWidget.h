/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file DashboardWidget.h
 * @brief Tabbed plot dashboard; routes data points to engine-agnostic plots.
 *
 * Tabs by category (Engine/ECU, GPS/IMU, BMS, VESC, ...). Each tab hosts one
 * PlotEngine instance (currently the Qwt-backed RealtimePlot), so the whole
 * dashboard can be re-backed by a different plotting engine without callers
 * changing. Includes the global X-axis selector enabling XY-mode plotting.
 */
#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QHash>
#include "PlotEngine.h"

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    /** Add a data series to the plot for this point's category */
    void addSeries(DataPointID id, const QString& source = QString());

    /** Forward one sample to the owning plot */
    void updateValue(DataPointID id, double value, const QDateTime& timestamp);

    /** Set the scrolling time window for all plots */
    void setTimeWindow(double seconds);

    /** Plot every series against another DP's value (Invalid = time on X) */
    void setXAxisForAll(DataPointID id);

    /** Clear all plots */
    void clearAll();

    /** The plot instance backing a category tab (nullptr if none) */
    PlotEngine* plotFor(const QString& category) const;

private slots:
    /** Open the DP picker and add the chosen series to the visible tab */
    void addSeriesViaPicker();


private:
    void setupUI();
    PlotEngine* getOrCreatePlot(const QString& category, const QString& title);
    void addBoostPIDSeries(PlotEngine* plot);

    QTabWidget* m_tabs = nullptr;
    QComboBox* m_xAxisCombo = nullptr;

    // Plot instances by category
    QHash<QString, PlotEngine*> m_plots;

    // Series-to-plot mapping
    QHash<DataPointID, QString> m_seriesToPlot;
};

#endif // DASHBOARDWIDGET_H