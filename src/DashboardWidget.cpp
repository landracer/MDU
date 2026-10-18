/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file DashboardWidget.cpp
 * @brief Tabbed PlotEngine dashboard with review toolbar and XY-mode selector.
 */
#include "DashboardWidget.h"
#include "RealtimePlot.h"
#include "DpPickerDialog.h"
#include "UnifiedDataStore.h"
#include <QPushButton>
#include <QDebug>
#include <QTabBar>
#include <QStyle>
#include <QDateTime>

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DashboardWidget::addBoostPIDSeries(PlotEngine* plot) {
    // All boost-related series on the BoostPID plot
    const QVector<DataPointID> boostPoints = {
        DataPointID::RPM, DataPointID::BoostPressure,
        DataPointID::N75DutyCycle, DataPointID::ThrottlePosition
    };
    for (auto id : boostPoints) plot->addSeries(id, QColor(), "System");
}

void DashboardWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // Review toolbar: pause/resume, crunch-to-full-history, marker cleanup, X axis
    QHBoxLayout* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    QPushButton* pauseButton = new QPushButton("⏸ Pause / Review", this);
    pauseButton->setCheckable(true);
    pauseButton->setToolTip(
        "Freeze the plots: whatever you are zoomed into stays frozen.\n"
        "Mouse wheel zooms and middle-drag pans on every plot at all times;\n"
        "zooming while live pins the view to that window (auto-follow pauses).\n"
        "Data keeps recording in the background — Resume Live snaps back with\n"
        "nothing lost.");
    toolbar->addWidget(pauseButton);

    QPushButton* showAllButton = new QPushButton("↔ Show All", this);
    showAllButton->setCheckable(true);
    showAllButton->setToolTip(
        "Crunch the graph: compact the entire session onto the visible axis\n"
        "instead of the scrolling time window (stays live). Wheel-zoom then\n"
        "drills into any subsection; the pinned window holds until you toggle\n"
        "this off or press Resume Live.");
    toolbar->addWidget(showAllButton);

    QPushButton* clearMarkersButton = new QPushButton("\u2715 Clear Markers", this);
    clearMarkersButton->setToolTip("Remove all click-markers from every plot\n(right-click a plot removes just the last one)");
    toolbar->addWidget(clearMarkersButton);

    // XY mode: swap every plot's X axis from time to any other series' value
    toolbar->addWidget(new QLabel("X axis:", this));
    m_xAxisCombo = new QComboBox(this);
    m_xAxisCombo->addItem("Time", QVariant(static_cast<uint32_t>(DataPointID::Invalid)));
    m_xAxisCombo->setToolTip("Plot every series against this data point instead of time");
    toolbar->addWidget(m_xAxisCombo);

    // Manual control: pick any data point and graph it on the visible tab
    QPushButton* addSeriesButton = new QPushButton("\uFF0B Add Series\u2026", this);
    addSeriesButton->setToolTip(
        "Choose a data point and plot it on the currently visible tab.\n"
        "(Series are normally added automatically to their category tab\n"
        "as soon as their data arrives \u2014 check the other tabs too.)");
    connect(addSeriesButton, &QPushButton::clicked,
            this, &DashboardWidget::addSeriesViaPicker);
    toolbar->addWidget(addSeriesButton);

    toolbar->addStretch();

    QLabel* hintLabel = new QLabel("Wheel: zoom (pins view) · Middle-drag: pan · Left-click plot: mark point · Right-click: remove last mark", this);
    hintLabel->setStyleSheet("color: #888; font-size: 10px;");
    toolbar->addWidget(hintLabel);

    mainLayout->addLayout(toolbar);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);

    struct TabInfo { QString name; QString title; };
    const QVector<TabInfo> tabs = {
        {"Visualization", "Visualization \u2014 RPM, Boost, Lambda, EGTs"},
        {"BoostPID", "Boost PID \u2014 RPM/Boost & Boost/Lambda on WOT"},
        {"GPS_IMU", "GPS & IMU \u2014 Speed, Position, Attitude"},
        {"BMS", "Battery / BMS \u2014 Pack V/A/SOC, Cells"},
        {"VESC", "VESC / Drivetrain \u2014 Motor, Wheel RPMs"},
        {"AllData", "All Data Points \u2014 Complete Overview"}
    };

    for (const auto& tab : tabs) {
        QWidget* tabWidget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tabWidget);
        layout->setContentsMargins(5, 5, 5, 5);

        PlotEngine* plot = new RealtimePlot(tabWidget);
        plot->setTitle(tab.title);
        plot->setTimeWindow(30.0);
        plot->setYLabel("Value");
        // RealtimePlot installs its own clickable legend — do not add another.

        layout->addWidget(plot, 1);
        m_tabs->addTab(tabWidget, tab.name);
        m_plots[tab.name] = plot;
    }

    mainLayout->addWidget(m_tabs, 1);

    // Status bar at bottom — also shows the values of the last clicked point
    QLabel* statusLabel = new QLabel("Ready \u2014 connect hardware to begin");
    statusLabel->setObjectName("dashboardStatus");
    statusLabel->setStyleSheet(
        "QLabel { background-color: #2b2b2b; color: #aaaaaa; padding: 5px; font-size: 11px; }");
    mainLayout->addWidget(statusLabel);

    // Add initial series to BoostPID tab
    auto boostPlotIt = m_plots.find("BoostPID");
    if (boostPlotIt != m_plots.end()) addBoostPIDSeries(boostPlotIt.value());

    // Wire the review controls to every plot
    connect(pauseButton, &QPushButton::toggled, this, [this, pauseButton](bool paused) {
        for (PlotEngine* plot : m_plots) plot->setPaused(paused);
        pauseButton->setText(paused ? QStringLiteral("\u25B6 Resume Live")
                                    : QStringLiteral("\u23F8 Pause / Review"));
    });
    connect(showAllButton, &QPushButton::toggled, this, [this](bool showAll) {
        for (PlotEngine* plot : m_plots) plot->setShowAllHistory(showAll);
    });
    connect(clearMarkersButton, &QPushButton::clicked, this, [this]() {
        for (PlotEngine* plot : m_plots) plot->clearMarkers();
    });
    for (PlotEngine* plot : m_plots)
        connect(plot, &PlotEngine::pointMarked, statusLabel, &QLabel::setText);

    // X-axis selector drives XY mode on every plot
    connect(m_xAxisCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        const uint32_t raw = m_xAxisCombo->currentData().toUInt();
        setXAxisForAll(static_cast<DataPointID>(raw));
    });
}

void DashboardWidget::addSeries(DataPointID id, const QString& source) {
    // Route by ID range to the matching tab
    const uint32_t val = static_cast<uint32_t>(id);
    QString category;
    if (id == DataPointID::RPM || id == DataPointID::BoostPressure ||
        id == DataPointID::N75DutyCycle || id == DataPointID::ThrottlePosition) {
        category = "BoostPID";
    } else if (val >= 0x0100 && val <= 0x01FF) {
        category = "Visualization";
    } else if ((val >= 0x0200 && val <= 0x02FF) || (val >= 0x0300 && val <= 0x03FF)) {
        category = "GPS_IMU";
    } else if ((val >= 0x0400 && val <= 0x04FF) || (val >= 0x1410 && val <= 0x141F)) {
        category = "BMS";
    } else if (val >= 0x0600 && val <= 0x06FF) {
        category = "VESC";
    } else {
        category = "AllData";
    }

    PlotEngine* plot = getOrCreatePlot(category, DataPointName(id));
    if (!plot) return;

    // Color is allocated by the engine's palette (user-overridable via legend click)
    plot->addSeries(id, QColor(), source);
    m_seriesToPlot[id] = category;
    qDebug() << "[Dashboard] Added series:" << DataPointName(id)
             << "to tab:" << category << "from source:" << source;

    // Offer this series as an XY-mode X axis choice
    if (m_xAxisCombo->findData(QVariant(static_cast<uint32_t>(id))) < 0)
        m_xAxisCombo->addItem(DataPointName(id), QVariant(static_cast<uint32_t>(id)));
}

void DashboardWidget::updateValue(DataPointID id, double value, const QDateTime& timestamp) {
    auto it = m_seriesToPlot.find(id);
    if (it == m_seriesToPlot.end()) {
        addSeries(id);                       // auto-add series for unknown data points
        it = m_seriesToPlot.find(id);
        if (it == m_seriesToPlot.end()) {
            qWarning() << "[Dashboard] Still couldn't find series after addSeries!";
            return;
        }
    }
    auto plotIt = m_plots.find(it.value());
    if (plotIt != m_plots.end())
        plotIt.value()->updateValue(id, value, timestamp);
    else
        qWarning() << "[Dashboard] Plot not found for category:" << it.value();
}

void DashboardWidget::setTimeWindow(double seconds) {
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
        it.value()->setTimeWindow(seconds);
}

void DashboardWidget::setXAxisForAll(DataPointID id) {
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
        it.value()->setXAxis(id);
}

void DashboardWidget::clearAll() {
    for (auto it = m_plots.begin(); it != m_plots.end(); ++it)
        it.value()->clear();
}

PlotEngine* DashboardWidget::getOrCreatePlot(const QString& category, const QString& title) {
    auto it = m_plots.find(category);
    if (it != m_plots.end()) return it.value();

    // Create a new tab (shouldn't normally happen)
    qDebug() << "[Dashboard] Creating new plot for category:" << category;
    QWidget* tabWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tabWidget);
    layout->setContentsMargins(5, 5, 5, 5);

    PlotEngine* plot = new RealtimePlot(tabWidget);
    plot->setTitle(title);
    plot->setTimeWindow(30.0);

    layout->addWidget(plot, 1);
    m_tabs->addTab(tabWidget, category);
    m_plots[category] = plot;

    // Wire into the status label like the plots created in setupUI
    if (QLabel* statusLabel = findChild<QLabel*>("dashboardStatus"))
        connect(plot, &PlotEngine::pointMarked, statusLabel, &QLabel::setText);

    return plot;
}

PlotEngine* DashboardWidget::plotFor(const QString& category) const
{
    return m_plots.value(category, nullptr);
}

void DashboardWidget::addSeriesViaPicker()
{
    const QString category = m_tabs->tabText(m_tabs->currentIndex());
    PlotEngine* plot = plotFor(category);
    if (!plot) return;

    DpPickerDialog picker(this);
    if (picker.exec() != QDialog::Accepted) return;

    const DataPointID id = picker.getSelectedDataPoint();
    if (id == DataPointID::Invalid) return;

    // Explicit user choice overrides the automatic category routing
    m_seriesToPlot[id] = category;
    if (!plot->hasSeries(id))
        plot->addSeries(id, QColor(), QStringLiteral("user"));

    // Seed the curve with the latest known value so the graph isn't empty
    bool valid = false;
    const double v = UnifiedDataStore::instance().getDataPoint(id, &valid);
    if (valid)
        plot->updateValue(id, v, QDateTime::currentDateTime());

    // Offer it as an XY-mode X axis choice too
    if (m_xAxisCombo->findData(QVariant(static_cast<uint32_t>(id))) < 0)
        m_xAxisCombo->addItem(DataPointName(id), QVariant(static_cast<uint32_t>(id)));

    if (QLabel* statusLabel = findChild<QLabel*>("dashboardStatus"))
        statusLabel->setText(QStringLiteral("Added %1 to '%2'")
                                 .arg(DataPointName(id), category));
    qDebug() << "[Dashboard] User added series:" << DataPointName(id)
             << "to tab:" << category;
}
