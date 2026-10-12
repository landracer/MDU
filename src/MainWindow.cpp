/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QWidget>
#include <QLabel>
#include <QTextEdit>
#include <QStatusBar>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

// ============================================================================
// Includes for BoostConfigDialog and GearSettingsDialog
// ============================================================================

#include "BoostConfigDialog.h"
#include "GearSettingsDialog.h"
#include <QSettings>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mdEcu(nullptr)
    , m_bms(nullptr)
    , m_bridge(nullptr)
    , m_csvParser(nullptr)
    , m_dashboard(nullptr)
    , m_testTimer(nullptr)
{
    setWindowTitle("MultiDisplay Unified v0.1.0");
    resize(1600, 900);
    
    setupUI();
    setupConnections();
    setupMenuBar();
    loadSettings();
    
    // Initialize status bar
    statusBar()->showMessage("Ready — connect hardware via command line options");
}

void MainWindow::loadSettings() {
    auto& settings = Settings::instance();
    
    // Apply display settings
    if (m_dashboard) {
        m_dashboard->setVisible(settings.getShowDashboard());
    }
    if (m_dataTable) {
        m_dataTable->setVisible(settings.getShowDataTable());
    }
    
    // Restore window geometry if saved
    QByteArray geo = settings.getMainWindowGeometry();
    if (!geo.isEmpty()) {
        restoreGeometry(geo);
    } else {
        resize(1600, 900);  // Default size
    }
    
    // Auto-connect if enabled
    if (!settings.getBridgePort().isEmpty() && settings.getAutoConnectBridge()) {
        connectBridge(settings.getBridgePort());
    }
    if (!settings.getMdEcuPort().isEmpty() && settings.getAutoConnectMdEcu()) {
        connectMdEcu(settings.getMdEcuPort());
    }
    
    statusBar()->showMessage("Settings loaded", 2000);
}

void MainWindow::setupUI() {
    // Central widget with layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // Header bar
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* headerLabel = new QLabel("MultiDisplay Unified Dashboard v0.1.0");
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; padding: 8px; background-color: #1e1e1e;");
    headerLayout->addWidget(headerLabel);
    
    headerLayout->addStretch();
    
    // Status indicators
    QLabel* mdStatus = new QLabel("MD-ECU: Disconnected");
    mdStatus->setObjectName("mdStatus");
    mdStatus->setStyleSheet("color: #ff4444; font-weight: bold; padding: 8px; background-color: #1e1e1e;");
    headerLayout->addWidget(mdStatus);
    
    QLabel* bmsStatus = new QLabel("BMS: Disconnected");
    bmsStatus->setObjectName("bmsStatus");
    bmsStatus->setStyleSheet("color: #ff4444; font-weight: bold; padding: 8px; background-color: #1e1e1e;");
    headerLayout->addWidget(bmsStatus);
    
    QLabel* bridgeStatus = new QLabel("ESP-NOW: Disconnected");
    bridgeStatus->setObjectName("bridgeStatus");
    bridgeStatus->setStyleSheet("color: #ff4444; font-weight: bold; padding: 8px; background-color: #1e1e1e;");
    headerLayout->addWidget(bridgeStatus);
    
    mainLayout->addLayout(headerLayout);
    
    // Splitter — draggable resize between Dashboard and Data Table
    auto* splitter = new QSplitter(Qt::Vertical, this);
    
    // Dashboard with real-time plots
    m_dashboard = new DashboardWidget(this);
    splitter->addWidget(m_dashboard);
    
    // Live data table — shows all valid data points in real-time
    m_dataTable = new LiveDataTable(this);
    splitter->addWidget(m_dataTable);
    
    // Set initial sizes: Dashboard 60%, Data Table 40%
    splitter->setSizes({540, 360});
    splitter->setStretchFactor(0, 1);  // Dashboard gets extra space on resize
    splitter->setStretchFactor(1, 0);  // Data Table keeps fixed minimum
    
    mainLayout->addWidget(splitter);
    
    // Bottom status bar
    QLabel* infoLabel = new QLabel(
        "Connect hardware: --md-ecu /dev/ttyUSB0 | --bms /dev/ttyUSB1 | --bridge /dev/ttyUSB2 | --csv-file MD0001.CSV | --test"
    );
    infoLabel->setStyleSheet(
        "QLabel { "
        "background-color: #2b2b2b; "
        "color: #888888; "
        "padding: 5px; "
        "font-size: 11px; "
        "font-family: monospace; "
        "}"
    );
    infoLabel->setWordWrap(true);
    mainLayout->addWidget(infoLabel);
}

void MainWindow::setupConnections() {
    // Connect all communication channels to UnifiedDataStore
    auto& datastore = UnifiedDataStore::instance();
    
    connect(&datastore, &UnifiedDataStore::dataPointUpdated,
            this, &MainWindow::onDataPointUpdated);
    
    // Update status bar periodically
    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    statusTimer->start(1000); // Every second
}

void MainWindow::connectMdEcu(const QString& portName) {
    if (m_mdEcu) {
        delete m_mdEcu;
    }
    
    m_mdEcu = new MdSerialComUnified(portName, this);
    connect(m_mdEcu, &MdSerialComUnified::frameParsed,
            this, [this](const QHash<DataPointID, double>& values) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            UnifiedDataStore::instance().setDataPoint(it.key(), it.value(), "MD-ECU");
        }
    });
    
    connect(m_mdEcu, &MdSerialComUnified::connectionStateChanged,
            this, [this](bool connected) {
        m_mdConnected = connected;
        if (connected) {
            statusBar()->showMessage("MD-ECU connected to " + m_mdEcu->portName());
        } else {
            statusBar()->showMessage("MD-ECU disconnected");
        }
    });
    
    connect(m_mdEcu, &MdSerialComUnified::errorOccurred,
            this, [this](const QString& error) {
        statusBar()->showMessage("MD-ECU Error: " + error, 5000);
    });
    
    if (m_mdEcu->open()) {
        m_mdConnected = true;
        statusBar()->showMessage("MD-ECU connected to " + portName);
    } else {
        statusBar()->showMessage("Failed to connect MD-ECU to " + portName, 5000);
    }
}

void MainWindow::connectBMS(const QString& portName) {
    if (m_bms) {
        delete m_bms;
    }
    
    m_bms = new RAtTraxBMS(portName, this);
    connect(m_bms, &RAtTraxBMS::frameParsed,
            this, [this](const QHash<DataPointID, double>& values) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            UnifiedDataStore::instance().setDataPoint(it.key(), it.value(), "rAtTrax-BMS");
        }
    });
    
    connect(m_bms, &RAtTraxBMS::connectionStateChanged,
            this, [this](bool connected) {
        m_bmsConnected = connected;
        if (connected) {
            statusBar()->showMessage("rAtTrax-BMS connected to " + m_bms->portName());
        } else {
            statusBar()->showMessage("rAtTrax-BMS disconnected");
        }
    });
    
    connect(m_bms, &RAtTraxBMS::batteryCritical,
            this, [this](double soc) {
        statusBar()->showMessage("BATTERY CRITICAL: " + QString::number(soc, 'f', 1) + "%", 10000);
    });
    
    connect(m_bms, &RAtTraxBMS::thermalWarning,
            this, [this](double temp) {
        statusBar()->showMessage("THERMAL WARNING: " + QString::number(temp, 'f', 1) + "°C", 10000);
    });
    
    connect(m_bms, &RAtTraxBMS::faultDetected,
            this, [this](uint32_t fault) {
        statusBar()->showMessage("VESC FAULT: 0x" + QString::number(fault, 16), 10000);
    });
    
    if (m_bms->open()) {
        m_bmsConnected = true;
        statusBar()->showMessage("rAtTrax-BMS connected to " + portName);
    } else {
        statusBar()->showMessage("Failed to connect rAtTrax-BMS to " + portName, 5000);
    }
}

void MainWindow::connectBridge(const QString& portName) {
    if (m_bridge) {
        delete m_bridge;
    }
    
    m_bridge = new OpenDashBridge(portName, this);
    connect(m_bridge, &OpenDashBridge::frameParsed,
            this, [this](const QHash<DataPointID, double>& values) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            UnifiedDataStore::instance().setDataPoint(it.key(), it.value(), "ESP-NOW-Bridge");
        }
    });
    
    connect(m_bridge, &OpenDashBridge::connectionStateChanged,
            this, [this](bool connected) {
        m_bridgeConnected = connected;
        if (connected) {
            statusBar()->showMessage("ESP-NOW Bridge connected to " + m_bridge->portName());
        } else {
            statusBar()->showMessage("ESP-NOW Bridge disconnected");
        }
    });
    
    connect(m_bridge, &OpenDashBridge::errorOccurred,
            this, [this](const QString& error) {
        statusBar()->showMessage("Bridge Error: " + error, 5000);
    });
    
    if (m_bridge->open()) {
        m_bridgeConnected = true;
        statusBar()->showMessage("ESP-NOW Bridge connected to " + portName);
    } else {
        statusBar()->showMessage("Failed to connect ESP-NOW Bridge to " + portName, 5000);
    }
}

void MainWindow::disconnectMdEcu() {
    if (m_mdEcu) {
        m_mdEcu->close();
        m_mdConnected = false;
        statusBar()->showMessage("MD-ECU disconnected");
    }
}

void MainWindow::disconnectBms() {
    if (m_bms) {
        m_bms->close();
        m_bmsConnected = false;
        statusBar()->showMessage("rAtTrax-BMS disconnected");
    }
}

void MainWindow::disconnectBridge() {
    if (m_bridge) {
        m_bridge->close();
        m_bridgeConnected = false;
        statusBar()->showMessage("ESP-NOW Bridge disconnected");
    }
}

void MainWindow::openCsvFile(const QString& filePath) {
    if (m_csvParser) {
        delete m_csvParser;
    }
    
    m_csvParser = new MDLoggerCSV(this);
    
    connect(m_csvParser, &MDLoggerCSV::lineParsed,
            this, [this](const QHash<DataPointID, double>& values) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            UnifiedDataStore::instance().setDataPoint(it.key(), it.value(), "MDLogger-CSV");
        }
    });
    
    connect(m_csvParser, &MDLoggerCSV::fileOpened,
            this, [this](const QString& path, int rows) {
        statusBar()->showMessage("MDLogger CSV opened: " + path + " (" + QString::number(rows) + " rows)");
    });
    
    connect(m_csvParser, &MDLoggerCSV::parsingComplete,
            this, [this]() {
        statusBar()->showMessage("CSV parsing complete");
    });
    
    if (m_csvParser->openFile(filePath)) {
        m_csvParser->startParsing();
        statusBar()->showMessage("Opening CSV: " + filePath);
    } else {
        statusBar()->showMessage("Failed to open CSV: " + filePath, 5000);
    }
}

void MainWindow::startTestMode() {
    if (m_testTimer) {
        delete m_testTimer;
    }
    
    m_testTimer = new QTimer(this);
    connect(m_testTimer, &QTimer::timeout, this, &MainWindow::generateTestData);
    m_testTimer->start(33); // ~30 Hz
    
    statusBar()->showMessage("TEST MODE — Simulated data at 30Hz");
}

void MainWindow::onDataPointUpdated(DataPointID id, double value, const QString& source) {
    // Update dashboard with the new value
    if (m_dashboard) {
        m_dashboard->updateValue(id, value, QDateTime::currentDateTime());
    }
}

void MainWindow::updateStatusBar() {
    // Update status labels
    QLabel* mdLabel = findChild<QLabel*>("mdStatus");
    QLabel* bmsLabel = findChild<QLabel*>("bmsStatus");
    QLabel* bridgeLabel = findChild<QLabel*>("bridgeStatus");
    
    if (mdLabel) {
        if (m_mdConnected) {
            mdLabel->setText("MD-ECU: Connected");
            mdLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            mdLabel->setText("MD-ECU: Disconnected");
            mdLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }
    
    if (bmsLabel) {
        if (m_bmsConnected) {
            bmsLabel->setText("BMS: Connected");
            bmsLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            bmsLabel->setText("BMS: Disconnected");
            bmsLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }
    
    if (bridgeLabel) {
        if (m_bridgeConnected) {
            bridgeLabel->setText("ESP-NOW: Connected");
            bridgeLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            bridgeLabel->setText("ESP-NOW: Disconnected");
            bridgeLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }
}

void MainWindow::generateTestData() {
    // Generate realistic test data for all systems
    auto& datastore = UnifiedDataStore::instance();
    
    // Engine RPM (simulated: idle to redline)
    static double testRPM = 800.0;
    testRPM += QRandomGenerator::global()->bounded(-100, 100);
    testRPM = qBound(800.0, testRPM, 7500.0);
    datastore.setDataPoint(DataPointID::RPM, testRPM, "TEST");
    
    // Boost pressure
    double boost = (testRPM - 800.0) / 6700.0 * 150.0 + QRandomGenerator::global()->bounded(-5, 5);
    datastore.setDataPoint(DataPointID::BoostPressure, qBound(0.0, boost, 250.0), "TEST");
    
    // Battery voltage
    datastore.setDataPoint(DataPointID::BatteryVoltage, 12.6 + QRandomGenerator::global()->bounded(-20, 20) / 100.0, "TEST");
    
    // Vehicle speed
    double speed = testRPM / 7500.0 * 200.0 + QRandomGenerator::global()->bounded(-10, 10);
    datastore.setDataPoint(DataPointID::VehicleSpeed, qBound(0.0, speed, 300.0), "TEST");
    
    // BMS simulation
    datastore.setDataPoint(DataPointID::BMSPackVoltage, 48.0 + QRandomGenerator::global()->bounded(-50, 50) / 100.0, "TEST");
    datastore.setDataPoint(DataPointID::BMSSOC, 75.0 + QRandomGenerator::global()->bounded(-50, 50) / 100.0, "TEST");
    
    // VESC simulation
    datastore.setDataPoint(DataPointID::VESCRPM, testRPM * 2.0 + QRandomGenerator::global()->bounded(-100, 100), "TEST");
    datastore.setDataPoint(DataPointID::VESCInputVoltage, 48.0 + QRandomGenerator::global()->bounded(-100, 100) / 100.0, "TEST");
    
    // Wheel RPMs
    double wheelRPM = testRPM / 4.0;
    datastore.setDataPoint(DataPointID::WheelRPMFL, wheelRPM + QRandomGenerator::global()->bounded(-20, 20), "TEST");
    datastore.setDataPoint(DataPointID::WheelRPMFR, wheelRPM + QRandomGenerator::global()->bounded(-20, 20), "TEST");
    datastore.setDataPoint(DataPointID::WheelRPMRL, wheelRPM + QRandomGenerator::global()->bounded(-20, 20), "TEST");
    datastore.setDataPoint(DataPointID::WheelRPMRR, wheelRPM + QRandomGenerator::global()->bounded(-20, 20), "TEST");
    
    // IMU simulation
    datastore.setDataPoint(DataPointID::IMUPitch, QRandomGenerator::global()->bounded(-50, 50) / 10.0, "TEST");
    datastore.setDataPoint(DataPointID::IMURoll, QRandomGenerator::global()->bounded(-50, 50) / 10.0, "TEST");
}

// ============================================================================
// Menu System Implementation
// ============================================================================

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    
    // --- File Menu ---
    QMenu* fileMenu = menuBar->addMenu("&File");
    
    QAction* openCsvAction = new QAction("Open MDLogger CSV...", this);
    openCsvAction->setShortcut(QKeySequence::Open);
    openCsvAction->setStatusTip("Open an MDLogger CSV file for playback");
    connect(openCsvAction, &QAction::triggered, this, &MainWindow::onOpenCsvFile);
    fileMenu->addAction(openCsvAction);
    
    fileMenu->addSeparator();
    
    QAction* closeAction = new QAction("Close Connection", this);
    closeAction->setShortcut(QKeySequence::Close);
    closeAction->setStatusTip("Close current hardware connection");
    connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseConnection);
    fileMenu->addAction(closeAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Quit the application");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);
    fileMenu->addAction(exitAction);
    
    // --- Edit Menu ---
    QMenu* editMenu = menuBar->addMenu("&Edit");
    
    QAction* preferencesAction = new QAction("Preferences...", this);
    preferencesAction->setShortcut(QKeySequence::Preferences);
    preferencesAction->setStatusTip("Open application preferences");
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferences);
    editMenu->addAction(preferencesAction);
    
     // --- Config Menu ---
     QMenu* configMenu = menuBar->addMenu("&Config");
     
     QAction* layoutEditorAction = new QAction("Layout Editor (OpenDash)...", this);
     layoutEditorAction->setStatusTip("Configure per-node display layouts for OpenDash mesh");
     connect(layoutEditorAction, &QAction::triggered, this, &MainWindow::onLayoutEditor);
     configMenu->addAction(layoutEditorAction);
     
     QAction* boostConfigAction = new QAction("Boost Map Config (N75)...", this);
     boostConfigAction->setStatusTip("Configure N75 PID boost control maps");
     connect(boostConfigAction, &QAction::triggered, this, &MainWindow::onBoostConfig);
     configMenu->addAction(boostConfigAction);
     
     QAction* gearSettingsAction = new QAction("Gearbox Settings...", this);
     gearSettingsAction->setStatusTip("Configure transmission gear ratios and profiles");
     connect(gearSettingsAction, &QAction::triggered, this, &MainWindow::onGearSettings);
     configMenu->addAction(gearSettingsAction);
    
    // --- OTA Menu ---
    QMenu* otaMenu = menuBar->addMenu("&OTA");
    
    QAction* otaManagerAction = new QAction("Firmware Manager...", this);
    otaManagerAction->setStatusTip("Manage firmware updates for OpenDash nodes and hardware");
    connect(otaManagerAction, &QAction::triggered, this, &MainWindow::onOtaManager);
    otaMenu->addAction(otaManagerAction);
    
    // --- View Menu ---
    QMenu* viewMenu = menuBar->addMenu("&View");
    
    QAction* toggleDataTableAction = new QAction("Toggle Data Table", this);
    toggleDataTableAction->setCheckable(true);
    toggleDataTableAction->setChecked(true);
    toggleDataTableAction->setStatusTip("Show/hide the live data table");
    connect(toggleDataTableAction, &QAction::triggered, this, [this](bool checked) {
        if (m_dataTable) {
            m_dataTable->setVisible(checked);
        }
    });
    viewMenu->addAction(toggleDataTableAction);
    
    QAction* toggleDashboardAction = new QAction("Toggle Dashboard", this);
    toggleDashboardAction->setCheckable(true);
    toggleDashboardAction->setChecked(true);
    toggleDashboardAction->setStatusTip("Show/hide the dashboard widget");
    connect(toggleDashboardAction, &QAction::triggered, this, [this](bool checked) {
        if (m_dashboard) {
            m_dashboard->setVisible(checked);
        }
    });
    viewMenu->addAction(toggleDashboardAction);
    
    viewMenu->addSeparator();
    
    QAction* fullscreenAction = new QAction("Fullscreen", this);
    fullscreenAction->setShortcut(QKeySequence::FullScreen);
    fullscreenAction->setStatusTip("Toggle fullscreen mode");
    connect(fullscreenAction, &QAction::triggered, this, &MainWindow::onToggleFullscreen);
    viewMenu->addAction(fullscreenAction);
    
    // --- Help Menu ---
    QMenu* helpMenu = menuBar->addMenu("&Help");
    
    QAction* documentationAction = new QAction("Documentation", this);
    documentationAction->setStatusTip("Open the project documentation");
    connect(documentationAction, &QAction::triggered, this, &MainWindow::onDocumentation);
    helpMenu->addAction(documentationAction);
    
    helpMenu->addSeparator();
    
    QAction* aboutAction = new QAction("About", this);
    aboutAction->setStatusTip("About MultiDisplay Unified");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    helpMenu->addAction(aboutAction);
}

// --- File Menu Slots ---

void MainWindow::onOpenCsvFile() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open MDLogger CSV File",
        "",
        "CSV Files (*.csv);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        openCsvFile(filePath);
    }
}

void MainWindow::onCloseConnection() {
    // Close all active connections
    if (m_mdEcu) {
        m_mdEcu->close();
        m_mdConnected = false;
    }
    
    if (m_bms) {
        m_bms->close();
        m_bmsConnected = false;
    }
    
    if (m_bridge) {
        m_bridge->close();
        m_bridgeConnected = false;
    }
    
    statusBar()->showMessage("All connections closed", 3000);
    updateStatusBar();
}

void MainWindow::onExit() {
    close();
}

// --- Edit Menu Slots ---

void MainWindow::onPreferences() {
    if (m_dataTable) {
        SettingsDialog dialog(m_dataTable, this);
        dialog.exec();
    }
}

// --- Config Menu Slots ---

void MainWindow::onLayoutEditor() {
    LayoutEditor dialog(this);
    dialog.exec();
}

void MainWindow::onBoostConfig() {
    BoostConfigDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Save settings
        auto& settings = Settings::instance();
        settings.setN75NormalDutyCycle(dialog.getNormalDutyCycle());
        settings.setN75RaceDutyCycle(dialog.getRaceDutyCycle());
        
        statusBar()->showMessage(
            QString("N75 Boost Config: Normal=%1, Race=%2")
                .arg(dialog.getNormalDutyCycle())
                .arg(dialog.getRaceDutyCycle()),
            3000
        );
    }
}

void MainWindow::onGearSettings() {
    GearSettingsDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Save gear ratios using Settings class methods
        auto& settings = Settings::instance();
        QVector<double> ratios = dialog.getGearRatios();
        
        for (int i = 0; i < 6; ++i) {
            settings.setGearRatio(i + 1, ratios[i]);
        }
        
        statusBar()->showMessage(
            "Gearbox Settings saved - Transmission ratios updated",
            3000
        );
    }
}

// --- OTA Menu Slots ---

void MainWindow::onOtaManager() {
    QMessageBox::information(
        this,
        "Firmware Manager",
        "OTA Firmware Manager coming soon.\n\n"
        "Features:\n"
        "- Scan for connected nodes\n"
        "- Select firmware binary\n"
        "- Trigger BLE OTA updates\n"
        "- Monitor flash progress"
    );
}

// --- View Menu Slots ---

void MainWindow::onToggleDataTable() {
    if (m_dataTable) {
        m_dataTable->setVisible(!m_dataTable->isVisible());
    }
}

void MainWindow::onToggleDashboard() {
    if (m_dashboard) {
        m_dashboard->setVisible(!m_dashboard->isVisible());
    }
}

void MainWindow::onToggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

// --- Help Menu Slots ---

void MainWindow::onDocumentation() {
    QMessageBox::information(
        this,
        "Documentation",
        "Project Documentation coming soon.\n\n"
        "Available resources:\n"
        "- mergecombo.md (Integration Plan)\n"
        "- ESP32-Bridge-Implementation-Guide.md\n"
        "- Protocol specifications\n"
        "- API documentation"
    );
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this,
        "About MultiDisplay Unified",
        "<h2>MultiDisplay Unified v0.1.0</h2>"
        "<p>A unified Qt6 dashboard for integrating:\n"
        "<ul>"
        "<li>MultiDisplay ECU (ATmega2560)</li>"
        "<li>rAtTrax-BMS (ESP32 + BQ76952)</li>"
        "<li>OpenDash Mesh (ESP32-S3 fleet)</li>"
        "<li>MDLogger (ATmega328P CSV logging)</li>"
        "</ul></p>"
        "<p>Supports ESP-NOW bridge protocol, real-time plotting with Qwt 6.3, and live data tables.</p>"
        "<p><i>Built with Qt6 and CMake on Linux</i></p>"
    );
}

void MainWindow::saveWindowState() {
    Settings::instance().setMainWindowGeometry(saveGeometry());
}
