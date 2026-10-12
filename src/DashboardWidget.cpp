/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "DashboardWidget.h"
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDebug>
#include <QTabBar>
#include <QStyle>
#include <QPushButton>

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DashboardWidget::addBoostPIDSeries(RealtimePlot* plot) {
    // Add all boost-related series to the BoostPID plot
    QVector<DataPointID> boostPoints = {
        DataPointID::RPM,
        DataPointID::BoostPressure,
        DataPointID::N75DutyCycle,
        DataPointID::ThrottlePosition
    };
    
    for (auto id : boostPoints) {
        // Use the existing plot's addSeries method
        plot->addSeries(id, QColor(), "System");
    }
}

void DashboardWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // Review toolbar: pause/resume, crunch-to-full-history, marker cleanup
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);
    
    QPushButton* pauseButton = new QPushButton("\u23F8 Pause / Review", this);
    pauseButton->setCheckable(true);
    pauseButton->setToolTip(
        "Freeze the plots for review: whole session is compacted into view,\n"
        "mouse wheel zooms, middle-drag pans. Data keeps recording in the\n"
        "background \u2014 resume snaps back to live with nothing lost.");
    toolbar->addWidget(pauseButton);
    
    QPushButton* showAllButton = new QPushButton("\u2194 Show All", this);
    showAllButton->setCheckable(true);
    showAllButton->setToolTip(
        "Crunch the graph: compact the entire session onto the visible axis\n"
        "instead of the scrolling time window (stays live).");
    toolbar->addWidget(showAllButton);
    
    QPushButton* clearMarkersButton = new QPushButton("\u2715 Clear Markers", this);
    clearMarkersButton->setToolTip("Remove all click-markers from every plot\n(right-click a plot removes just the last one)");
    toolbar->addWidget(clearMarkersButton);
    
    toolbar->addStretch();
    
    QLabel* hintLabel = new QLabel("Left-click plot: mark point \u00b7 Right-click: remove last mark", this);
    hintLabel->setStyleSheet("color: #888; font-size: 10px;");
    toolbar->addWidget(hintLabel);
    
    mainLayout->addLayout(toolbar);
    
    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    
    // Create plot tabs by category
    struct TabInfo {
        QString name;
        QString title;
    };
    
    // Tab structure matches original multidisplay app:
    // - Visualization: Shows all engine curves like VisualizationPlot (RPM, Boost, Lambda, EGTs, etc.)
    // - BoostPID: N75 PID configuration and WOT plots
    QVector<TabInfo> tabs = {
        {"Visualization", "Visualization — RPM, Boost, Lambda, EGTs"},
        {"BoostPID", "Boost PID — RPM/Boost & N75 Control"},
        {"GPS_IMU", "GPS & IMU — Speed, Position, Attitude"},
        {"BMS", "Battery / BMS — Pack V/A/SOC, Cells"},
        {"VESC", "VESC / Drivetrain — Motor, Wheel RPMs"},
        {"AllData", "All Data Points — Complete Overview"}
    };
    
    for (const auto& tab : tabs) {
        QWidget* tabWidget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tabWidget);
        layout->setContentsMargins(5, 5, 5, 5);
        
        RealtimePlot* plot = new RealtimePlot(tabWidget);
        if (tab.name == "BoostPID") {
            plot->setTitle("Boost PID — RPM/Boost & Boost/Lambda on WOT");
        } else {
            plot->setTitle(tab.title);
        }
        plot->setTimeWindow(30.0);
        plot->setYLabel("Value");
        
        // RealtimePlot installs its own clickable legend (click an entry to
        // rename the series or change its color) — do not insert another here.
        
        layout->addWidget(plot, 1);
        m_tabs->addTab(tabWidget, tab.name);
        m_plots[tab.name] = plot;
    }
    
    mainLayout->addWidget(m_tabs, 1);
    
    // Status bar at bottom — also shows the values of the last clicked point
    QLabel* statusLabel = new QLabel("Ready — connect hardware to begin");
    statusLabel->setObjectName("dashboardStatus");
    statusLabel->setStyleSheet(
        "QLabel { "
        "background-color: #2b2b2b; "
        "color: #aaaaaa; "
        "padding: 5px; "
        "font-size: 11px; "
        "}"
    );
    mainLayout->addWidget(statusLabel);
    
    // Add initial series to BoostPID tab
    auto boostPlotIt = m_plots.find("BoostPID");
    if (boostPlotIt != m_plots.end()) {
        addBoostPIDSeries(boostPlotIt.value());
    }
    
    // Wire the review controls to every plot
    connect(pauseButton, &QPushButton::toggled, this, [this, pauseButton](bool paused) {
        for (RealtimePlot* plot : m_plots) plot->setPaused(paused);
        pauseButton->setText(paused ? QStringLiteral("\u25B6 Resume Live")
                                    : QStringLiteral("\u23F8 Pause / Review"));
    });
    connect(showAllButton, &QPushButton::toggled, this, [this](bool showAll) {
        for (RealtimePlot* plot : m_plots) plot->setShowAllHistory(showAll);
    });
    connect(clearMarkersButton, &QPushButton::clicked, this, [this]() {
        for (RealtimePlot* plot : m_plots) plot->clearMarkers();
    });
    for (RealtimePlot* plot : m_plots) {
        connect(plot, &RealtimePlot::pointMarked, statusLabel, &QLabel::setText);
    }
}

void DashboardWidget::addSeries(DataPointID id, const QString& source) {
    // Determine which tab this data point belongs to
    uint32_t val = static_cast<uint32_t>(id);
    QString category;
    
    // BoostPID tab for N75 control and WOT plots
    if (id == DataPointID::RPM || id == DataPointID::BoostPressure || 
        id == DataPointID::N75DutyCycle || id == DataPointID::ThrottlePosition) {
        category = "BoostPID";
    } else if (val >= 0x0100 && val <= 0x01FF) {
        // All engine data points go to Visualization tab for visualization
        category = "Visualization";
    } else if (val >= 0x0200 && val <= 0x02FF || val >= 0x0300 && val <= 0x03FF) {
        category = "GPS_IMU";
    } else if ((val >= 0x0400 && val <= 0x04FF) || (val >= 0x1410 && val <= 0x141F)) {
        category = "BMS";
    } else if (val >= 0x0600 && val <= 0x06FF) {
        category = "VESC";
    } else {
        category = "AllData";
    }
    
    // Handle BoostPID tab data points
    if (category == "BoostPID") {
        // Get the existing BoostPID plot
        auto it = m_plots.find("BoostPID");
        if (it != m_plots.end()) {
            RealtimePlot* plot = it.value();
            plot->addSeries(id, QColor(), source);
            m_seriesToPlot[id] = "BoostPID";
            qDebug() << "[Dashboard] Added BoostPID series:" << DataPointName(id) 
                     << "from source:" << source;
            return;
        }
    }
    
    // Get or create plot for this category
    RealtimePlot* plot = getOrCreatePlot(category, DataPointName(id));
    if (!plot) return;
    
    // Color is allocated by RealtimePlot from its palette so every stream on
    // a plot gets its own unique color (user can override via legend click).
    plot->addSeries(id, QColor(), source);
    m_seriesToPlot[id] = category;
    
    qDebug() << "[Dashboard] Added series:" << DataPointName(id) 
             << "to tab:" << category << "from source:" << source;
}

void DashboardWidget::updateValue(DataPointID id, double value, const QDateTime& timestamp) {
    auto it = m_seriesToPlot.find(id);
    if (it == m_seriesToPlot.end()) {
        // Auto-add series for unknown data points
        addSeries(id);
        it = m_seriesToPlot.find(id);
        if (it == m_seriesToPlot.end()) {
            qWarning() << "[Dashboard] Still couldn't find series after addSeries!";
            return;
        }
    }
    
    auto plotIt = m_plots.find(it.value());
    if (plotIt != m_plots.end()) {
        plotIt.value()->updateValue(id, value, timestamp);
    } else {
        qWarning() << "[Dashboard] Plot not found for category:" << it.value();
    }
}

void DashboardWidget::setTimeWindow(double seconds) {
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it) {
        it.value()->setTimeWindow(seconds);
    }
}

void DashboardWidget::clearAll() {
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it) {
        it.value()->clear();
    }
}

RealtimePlot* DashboardWidget::getOrCreatePlot(const QString& category, const QString& title) {
    auto it = m_plots.find(category);
    if (it != m_plots.end()) {
        return it.value();
    }
    
    // Create new plot tab (shouldn't normally happen)
    qDebug() << "[Dashboard] Creating new plot for category:" << category;
    QWidget* tabWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tabWidget);
    layout->setContentsMargins(5, 5, 5, 5);
    
    RealtimePlot* plot = new RealtimePlot(tabWidget);
    plot->setTitle(title);
    plot->setTimeWindow(30.0);
    
    layout->addWidget(plot, 1);
    m_tabs->addTab(tabWidget, category);
    m_plots[category] = plot;
    
    // Wire into the review controls like the plots created in setupUI
    if (QLabel* statusLabel = findChild<QLabel*>("dashboardStatus")) {
        connect(plot, &RealtimePlot::pointMarked, statusLabel, &QLabel::setText);
    }
    
    return plot;
}
