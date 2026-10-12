/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QVariant>
#include <QHash>
#include <QSettings>

/**
 * @brief Centralized persistent settings manager
 * 
 * Uses Qt QSettings for cross-platform persistence:
 *   Linux: ~/.config/MultiDisplay-Unified/settings.conf
 *   Windows: HKEY_CURRENT_USER\Software\MultiDisplay-Unified
 *   macOS: ~/Library/Preferences/MultiDisplay-Unified.plist
 */
class Settings {
public:
    static Settings& instance();

    // --- Connection Settings ---
    QString getMdEcuPort() const;
    int getMdEcuBaudRate() const;
    void setMdEcuBaudRate(int baud);
    QString getBridgePort() const;
    int getBridgeBaudRate() const;
    void setBridgeBaudRate(int baud);
    QString getBmsPort() const;
    int getBmsBaudRate() const;
    QString getLastCsvPath() const;

    // --- Auto-connect Settings ---
    bool getAutoConnectMdEcu() const;
    bool getAutoConnectBridge() const;
    bool getAutoConnectBms() const;

    // --- Display Settings ---
    int getDataTableRefreshRate() const;  // Hz
    int getDashboardRefreshRate() const;  // Hz
    QString getTheme() const;             // "dark", "light", "system"
    bool getShowDataTable() const;
    bool getShowDashboard() const;

    // --- Data Point Visibility ---
    bool isDataPointVisible(quint16 dpId) const;
    QHash<quint16, bool> getAllDataPointVisibility() const;

    // --- Window Settings ---
    QByteArray getMainWindowGeometry() const;
    QByteArray getMainWindowState() const;

    // --- Setters ---
    void setMdEcuPort(const QString& port);
    void setBridgePort(const QString& port);
    void setBmsPort(const QString& port);
    void setLastCsvPath(const QString& path);
    void setAutoConnectMdEcu(bool enabled);
    void setAutoConnectBridge(bool enabled);
    void setAutoConnectBms(bool enabled);
    void setDataTableRefreshRate(int rate);
    void setDashboardRefreshRate(int rate);
    void setTheme(const QString& theme);
    void setShowDataTable(bool visible);
    void setShowDashboard(bool visible);
    void setDataPointVisible(quint16 dpId, bool visible);
    void setMainWindowGeometry(const QByteArray& geometry);
    void setMainWindowState(const QByteArray& state);
    
    // --- N75 Boost Config ---
    int getN75NormalDutyCycle() const;
    int getN75RaceDutyCycle() const;
    void setN75NormalDutyCycle(int value);
    void setN75RaceDutyCycle(int value);

    // --- Gear Settings ---
    QString getGearRatio(int gear) const;  // gear: 1-6
    void setGearRatio(int gear, double value);  // gear: 1-6

    // --- Utility ---
    void resetToDefaults();
    void save() const;  // Explicit save (QSettings auto-saves on destruction too)

private:
    Settings();
    ~Settings() = default;
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    mutable QSettings m_settings;
};

#endif // SETTINGS_H