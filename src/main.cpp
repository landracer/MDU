/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include <QApplication>
#include <QDebug>
#include <QCommandLineParser>
#include <QMessageBox>
#include "MainWindow.h"
#include "UnifiedDataStore.h"
#include "TestHarness.h"

// APP_VERSION is injected by CMake from project(... VERSION ...) — single source of truth.

/**
 * @brief Main entry point for multidisplay-unified
 * 
 * Cross-platform Qt6 application that unifies 4 hardware systems:
 * - MultiDisplay ECU (ATmega2560) via USB serial
 * - rAtTrax-BMS (ESP32 + BQ76952 + VESC + GPS) via USB/Bluetooth
 * - OpenDash Mesh (ESP32-S3 ESP-NOW) via USB bridge
 * - MDLogger (ATmega328P CSV files) via file import
 * 
 * Usage:
 *   multidisplay-unified --md-ecu /dev/ttyUSB0 --bms /dev/ttyUSB1 --bridge /dev/ttyUSB2
 *   multidisplay-unified --csv MD0001.CSV
 *   multidisplay-unified --test          (run test harness + simulated data)
 *   multidisplay-unified --unit-test     (run unit tests only)
 *   multidisplay-unified --help
 */
/**
 * @brief Message handler that always routes Qt log messages to the console.
 *
 * Qt 6 builds on systemd distros otherwise silently route qDebug/qInfo output
 * to journald whenever stderr is not a TTY (e.g. output piped or redirected),
 * which made the test-suite and parser diagnostics invisible in terminals.
 */
static void consoleMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    Q_UNUSED(type);
    Q_UNUSED(context);
    fputs((message + "\n").toUtf8().constData(), stderr);
    fflush(stderr);
}

int main(int argc, char* argv[]) {
    qInstallMessageHandler(consoleMessageHandler);

    QApplication app(argc, argv);
    app.setApplicationName("MultiDisplay Unified");
    app.setApplicationVersion(QStringLiteral(APP_VERSION));
    app.setOrganizationName("multidisplay");
    
    // Command line parser for hardware configuration
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Unified dashboard for MultiDisplay ECU, rAtTrax-BMS, OpenDash Mesh, and MDLogger"
    );
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Hardware connection options
    QCommandLineOption mdEcuOption(QStringList() << "m" << "md-ecu",
        "MultiDisplay ECU serial port (e.g., /dev/ttyUSB0 or COM3)", "port");
    parser.addOption(mdEcuOption);
    
    QCommandLineOption bmsOption(QStringList() << "b" << "bms",
        "rAtTrax-BMS serial port (e.g., /dev/ttyUSB1)", "port");
    parser.addOption(bmsOption);
    
    QCommandLineOption bridgeOption(QStringList() << "g" << "bridge",
        "OpenDash ESP-NOW Bridge serial port (e.g., /dev/ttyUSB2)", "port");
    parser.addOption(bridgeOption);
    
    QCommandLineOption csvOption(QStringList() << "c" << "csv",
        "MDLogger CSV file to import (e.g., MD0001.CSV)", "file");
    parser.addOption(csvOption);
    
    QCommandLineOption testOption("test",
        "Run in test mode with simulated data");
    parser.addOption(testOption);
    
    QCommandLineOption unitTestOption("unit-test",
        "Run unit tests only and exit");
    parser.addOption(unitTestOption);
    
    parser.process(app);
    
    qDebug() << "========================================";
    qDebug() << "MultiDisplay Unified v" << APP_VERSION;
    qDebug() << "========================================";
    
    // Run unit tests if requested
    if (parser.isSet(unitTestOption)) {
        TestHarness harness;
        bool allPassed = harness.runAllTests();
        return allPassed ? 0 : 1;
    }
    
    // Get the singleton data store
    UnifiedDataStore& datastore = UnifiedDataStore::instance();
    
    // Create main window
    MainWindow mainWindow;
    
    // Connect hardware based on command line options
    if (parser.isSet(mdEcuOption)) {
        QString port = parser.value(mdEcuOption);
        qDebug() << "[Main] Connecting MultiDisplay ECU to:" << port;
        mainWindow.connectMdEcu(port);
    }
    
    if (parser.isSet(bmsOption)) {
        QString port = parser.value(bmsOption);
        qDebug() << "[Main] Connecting rAtTrax-BMS to:" << port;
        mainWindow.connectBMS(port);
    }
    
    if (parser.isSet(bridgeOption)) {
        QString port = parser.value(bridgeOption);
        qDebug() << "[Main] Connecting ESP-NOW Bridge to:" << port;
        mainWindow.connectBridge(port);
    }
    
    if (parser.isSet(csvOption)) {
        QString filePath = parser.value(csvOption);
        qDebug() << "[Main] Opening MDLogger CSV:" << filePath;
        mainWindow.openCsvFile(filePath);
    }
    
    if (parser.isSet(testOption)) {
        qDebug() << "[Main] Running in TEST mode with simulated data";
        mainWindow.startTestMode();
    }
    
    mainWindow.show();
    
    qDebug() << "========================================";
    qDebug() << "Application started successfully";
    qDebug() << "========================================";
    
    return app.exec();
}
