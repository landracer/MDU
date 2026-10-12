/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include "UnifiedDataStore.h"
#include "MdSerialComUnified.h"
#include "RAtTraxBMS.h"
#include "OpenDashBridge.h"
#include "MDLoggerCSV.h"
#include "DashboardWidget.h"
#include "LiveDataTable.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "LayoutEditor.h"
#include "BoostConfigDialog.h"
#include "GearSettingsDialog.h"

/**
 * @brief Main application window
 * 
 * Ties together all communication channels and provides the UI shell.
 * Displays real-time data from all connected hardware systems.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    /**
     * @brief Connect MultiDisplay ECU to specified port
     */
    void connectMdEcu(const QString& portName);

    /**
     * @brief Connect rAtTrax-BMS to specified port
     */
    void connectBMS(const QString& portName);

    /**
     * @brief Connect OpenDash ESP-NOW Bridge to specified port
     */
    void connectBridge(const QString& portName);

    /**
     * @brief Disconnect MultiDisplay ECU
     */
    void disconnectMdEcu();

    /**
     * @brief Disconnect rAtTrax-BMS
     */
    void disconnectBms();

    /**
     * @brief Disconnect ESP-NOW Bridge
     */
    void disconnectBridge();

    /**
     * @brief Open MDLogger CSV file for import
     */
    void openCsvFile(const QString& filePath);

    /**
     * @brief Start test mode with simulated data
     */
    void startTestMode();

private slots:
    /**
     * @brief Handle data point updates from UnifiedDataStore
     */
    void onDataPointUpdated(DataPointID id, double value, const QString& source);

    /**
     * @brief Update status bar with connection states
     */
    void updateStatusBar();

    /**
     * @brief Generate simulated test data
     */
    void generateTestData();

    // File menu actions
    void onOpenCsvFile();
    void onCloseConnection();
    void onExit();

    // Edit menu actions
    void onPreferences();

     // Config menu actions
     void onLayoutEditor();
     void onBoostConfig();
     void onGearSettings();

     // OTA menu actions
    void onOtaManager();

    // View menu actions
    void onToggleDataTable();
    void onToggleDashboard();
    void onToggleFullscreen();

    // Help menu actions
    void onAbout();
    void onDocumentation();

    // Settings
    void loadSettings();
    void saveWindowState();

public:
    /**
     * @brief Get the LiveDataTable instance
     */
    LiveDataTable* getLiveDataTable() const { return m_dataTable; }

private:
    void setupUI();
    void setupConnections();
    void setupMenuBar();

    // Communication channels
    MdSerialComUnified* m_mdEcu;
    RAtTraxBMS* m_bms;
    OpenDashBridge* m_bridge;
    MDLoggerCSV* m_csvParser;

    // UI components
    DashboardWidget* m_dashboard;
    LiveDataTable* m_dataTable;

    // Test mode timer
    QTimer* m_testTimer;
    
    // Status tracking
    bool m_mdConnected = false;
    bool m_bmsConnected = false;
    bool m_bridgeConnected = false;
};

#endif // MAINWINDOW_H
