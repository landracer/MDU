/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "Settings.h"
#include <QSettings>
#include <QDebug>

// Key prefixes
static const QString KEY_PREFIX = "app/";
static const QString KEY_MD_ECU_PORT = KEY_PREFIX + "mdEcu/port";
static const QString KEY_MD_ECU_BAUD = KEY_PREFIX + "mdEcu/baudRate";
static const QString KEY_BRIDGE_PORT = KEY_PREFIX + "bridge/port";
static const QString KEY_BRIDGE_BAUD = KEY_PREFIX + "bridge/baudRate";
static const QString KEY_BMS_PORT = KEY_PREFIX + "bms/port";
static const QString KEY_BMS_BAUD = KEY_PREFIX + "bms/baudRate";
static const QString KEY_LAST_CSV = KEY_PREFIX + "lastCsvPath";
static const QString KEY_AUTO_MD_ECU = KEY_PREFIX + "autoConnect/mdEcu";
static const QString KEY_AUTO_BRIDGE = KEY_PREFIX + "autoConnect/bridge";
static const QString KEY_AUTO_BMS = KEY_PREFIX + "autoConnect/bms";
static const QString KEY_TABLE_REFRESH = KEY_PREFIX + "display/dataTableRefreshRate";
static const QString KEY_DASHBOARD_REFRESH = KEY_PREFIX + "display/dashboardRefreshRate";
static const QString KEY_THEME = KEY_PREFIX + "display/theme";
static const QString KEY_SHOW_TABLE = KEY_PREFIX + "display/showDataTable";
static const QString KEY_SHOW_DASHBOARD = KEY_PREFIX + "display/showDashboard";
static const QString KEY_WINDOW_GEOMETRY = KEY_PREFIX + "window/geometry";
static const QString KEY_WINDOW_STATE = KEY_PREFIX + "window/state";

// --- N75 Boost Config ---
static const QString KEY_N75_NORMAL_DUTY = KEY_PREFIX + "n75/normalDuty";
static const QString KEY_N75_RACE_DUTY = KEY_PREFIX + "n75/raceDuty";

Settings::Settings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "MultiDisplay-Unified", "settings") {
}

Settings& Settings::instance() {
    static Settings instance;
    return instance;
}

// --- Connection Settings ---

QString Settings::getMdEcuPort() const {
    return m_settings.value(KEY_MD_ECU_PORT, QString()).toString();
}

int Settings::getMdEcuBaudRate() const {
    return m_settings.value(KEY_MD_ECU_BAUD, 115200).toInt();
}

QString Settings::getBridgePort() const {
    return m_settings.value(KEY_BRIDGE_PORT, QString()).toString();
}

int Settings::getBridgeBaudRate() const {
    return m_settings.value(KEY_BRIDGE_BAUD, 115200).toInt();
}

QString Settings::getBmsPort() const {
    return m_settings.value(KEY_BMS_PORT, QString()).toString();
}

int Settings::getBmsBaudRate() const {
    return m_settings.value(KEY_BMS_BAUD, 115200).toInt();
}

QString Settings::getLastCsvPath() const {
    return m_settings.value(KEY_LAST_CSV, QString()).toString();
}

// --- Auto-connect Settings ---

bool Settings::getAutoConnectMdEcu() const {
    return m_settings.value(KEY_AUTO_MD_ECU, false).toBool();
}

bool Settings::getAutoConnectBridge() const {
    return m_settings.value(KEY_AUTO_BRIDGE, false).toBool();
}

bool Settings::getAutoConnectBms() const {
    return m_settings.value(KEY_AUTO_BMS, false).toBool();
}

// --- Display Settings ---

int Settings::getDataTableRefreshRate() const {
    return m_settings.value(KEY_TABLE_REFRESH, 5).toInt();
}

int Settings::getDashboardRefreshRate() const {
    return m_settings.value(KEY_DASHBOARD_REFRESH, 30).toInt();
}

QString Settings::getTheme() const {
    return m_settings.value(KEY_THEME, "dark").toString();
}

bool Settings::getShowDataTable() const {
    return m_settings.value(KEY_SHOW_TABLE, true).toBool();
}

bool Settings::getShowDashboard() const {
    return m_settings.value(KEY_SHOW_DASHBOARD, true).toBool();
}

// --- Data Point Visibility ---

bool Settings::isDataPointVisible(quint16 dpId) const {
    QString key = KEY_PREFIX + "datapoints/0x" + QString::number(dpId, 16);
    return m_settings.value(key, true).toBool();
}

QHash<quint16, bool> Settings::getAllDataPointVisibility() const {
    QHash<quint16, bool> visibility;
    m_settings.beginGroup(KEY_PREFIX + "datapoints");
    auto keys = m_settings.allKeys();
    for (const auto& key : keys) {
        quint16 dpId = key.mid(2).toUInt(nullptr, 16); // Parse "0xHHHH"
        visibility[dpId] = m_settings.value(key, true).toBool();
    }
    m_settings.endGroup();
    return visibility;
}

// --- Window Settings ---

QByteArray Settings::getMainWindowGeometry() const {
    return m_settings.value(KEY_WINDOW_GEOMETRY, QByteArray()).toByteArray();
}

QByteArray Settings::getMainWindowState() const {
    return m_settings.value(KEY_WINDOW_STATE, QByteArray()).toByteArray();
}

// --- Setters ---

void Settings::setMdEcuPort(const QString& port) {
    m_settings.setValue(KEY_MD_ECU_PORT, port);
}

void Settings::setMdEcuBaudRate(int baud) {
    m_settings.setValue(KEY_MD_ECU_BAUD, baud);
}

void Settings::setBridgePort(const QString& port) {
    m_settings.setValue(KEY_BRIDGE_PORT, port);
}

void Settings::setBridgeBaudRate(int baud) {
    m_settings.setValue(KEY_BRIDGE_BAUD, baud);
}

void Settings::setBmsPort(const QString& port) {
    m_settings.setValue(KEY_BMS_PORT, port);
}

void Settings::setLastCsvPath(const QString& path) {
    m_settings.setValue(KEY_LAST_CSV, path);
}

void Settings::setAutoConnectMdEcu(bool enabled) {
    m_settings.setValue(KEY_AUTO_MD_ECU, enabled);
}

void Settings::setAutoConnectBridge(bool enabled) {
    m_settings.setValue(KEY_AUTO_BRIDGE, enabled);
}

void Settings::setAutoConnectBms(bool enabled) {
    m_settings.setValue(KEY_AUTO_BMS, enabled);
}

void Settings::setDataTableRefreshRate(int rate) {
    m_settings.setValue(KEY_TABLE_REFRESH, qBound(1, rate, 60));
}

void Settings::setDashboardRefreshRate(int rate) {
    m_settings.setValue(KEY_DASHBOARD_REFRESH, qBound(1, rate, 120));
}

void Settings::setTheme(const QString& theme) {
    m_settings.setValue(KEY_THEME, theme);
}

void Settings::setShowDataTable(bool visible) {
    m_settings.setValue(KEY_SHOW_TABLE, visible);
}

void Settings::setShowDashboard(bool visible) {
    m_settings.setValue(KEY_SHOW_DASHBOARD, visible);
}

void Settings::setDataPointVisible(quint16 dpId, bool visible) {
    QString key = KEY_PREFIX + "datapoints/0x" + QString::number(dpId, 16);
    m_settings.setValue(key, visible);
}

void Settings::setMainWindowGeometry(const QByteArray& geometry) {
    m_settings.setValue(KEY_WINDOW_GEOMETRY, geometry);
}

void Settings::setMainWindowState(const QByteArray& state) {
    m_settings.setValue(KEY_WINDOW_STATE, state);
}

// --- N75 Boost Config ---
int Settings::getN75NormalDutyCycle() const {
    return m_settings.value(KEY_N75_NORMAL_DUTY, 128).toInt();
}

int Settings::getN75RaceDutyCycle() const {
    return m_settings.value(KEY_N75_RACE_DUTY, 255).toInt();
}

void Settings::setN75NormalDutyCycle(int value) {
    m_settings.setValue(KEY_N75_NORMAL_DUTY, qBound(0, value, 255));
}

void Settings::setN75RaceDutyCycle(int value) {
    m_settings.setValue(KEY_N75_RACE_DUTY, qBound(0, value, 255));
}

// --- Gear Settings ---
QString Settings::getGearRatio(int gear) const {
    if (gear < 1 || gear > 6) return QString();
    QString key = KEY_PREFIX + "gear/ratio" + QString::number(gear);
    bool ok = false;
    double value = m_settings.value(key, QString("0.0")).toDouble(&ok);
    return ok ? QString::number(value, 'f', 4) : QString();
}

void Settings::setGearRatio(int gear, double value) {
    if (gear < 1 || gear > 6) return;
    QString key = KEY_PREFIX + "gear/ratio" + QString::number(gear);
    m_settings.setValue(key, QString::number(value, 'f', 4));
}

void Settings::resetToDefaults() {
    m_settings.clear();
}

void Settings::save() const {
    m_settings.sync();
}
