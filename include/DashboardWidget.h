/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include "RealtimePlot.h"
#include <QHash>

/**
 * @brief Dashboard widget with multiple real-time plot panels
 * 
 * Organizes plots into tabs by category:
 * - Engine/ECU: RPM, Boost, Lambda, EGTs
 * - GPS/IMU: Speed, Lat/Lon, Pitch/Roll/Yaw
 * - BMS: Pack V/A/SOC, Cell Voltages
 * - VESC: Motor V/A/RPM, Wheel RPMs
 * 
 * Each tab contains one or more RealtimePlot instances.
 */
class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);
    ~DashboardWidget() override = default;

    /**
     * @brief Add a data series to the appropriate plot
     */
    void addSeries(DataPointID id, const QString& source = QString());

    /**
     * @brief Update a data point value (called by UnifiedDataStore)
     */
    void updateValue(DataPointID id, double value, const QDateTime& timestamp);

    /**
     * @brief Set the time window for all plots
     */
    void setTimeWindow(double seconds);

    /**
     * @brief Clear all plots
     */
    void clearAll();

private:
    void setupUI();
    RealtimePlot* getOrCreatePlot(const QString& category, const QString& title);

    QTabWidget* m_tabs;
    
    // Plot instances by category
    QHash<QString, RealtimePlot*> m_plots;
    
    // Series-to-plot mapping
    QHash<DataPointID, QString> m_seriesToPlot;
    
private:
    void addBoostPIDSeries(RealtimePlot* plot);
};

#endif // DASHBOARDWIDGET_H
