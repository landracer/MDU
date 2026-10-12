/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QString>
#include <QHash>
#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>

class Settings;
class DataPointRegistry;
class MainWindow;
class LiveDataTable;

/**
 * @brief Application settings dialog
 * 
 * Organized into tabs:
 *   1. Connections — Serial port configuration for each hardware source
 *   2. Display — Refresh rates, theme, panel visibility
 *   3. Data Points — Toggle individual data point visibility
 *   4. Window — Restore saved geometry/state
 */
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(LiveDataTable* liveDataTable, QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onApply();
    void onResetToDefaults();
    void onClose();
    void onDataPointToggled(QTreeWidgetItem* item, int column);
    void onCategoryToggled(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onShowAllClicked();
    void onHideAllClicked();
    void applyMdEcuConnection();
    void applyBridgeConnection();
    void applyBmsConnection();

private:
    void setupUI();
    void loadSettings();
    void populateDataPointList();
    void populateDataPointTree();
    QStringList getAvailablePorts() const;
    void showStatusMessage(const QString& message, int timeout = 3000);

    // UI Components — Connections Tab
    QWidget* m_connectionsTab;
    QComboBox* m_mdEcuPortCombo;
    QSpinBox* m_mdEcuBaudSpin;
    QCheckBox* m_autoConnectMdEcuCheck;
    QComboBox* m_bridgePortCombo;
    QSpinBox* m_bridgeBaudSpin;
    QCheckBox* m_autoConnectBridgeCheck;
    QLineEdit* m_bridgePortTestEdit;
    QPushButton* m_testBridgeButton;
    QComboBox* m_bmsPortCombo;
    QCheckBox* m_autoConnectBmsCheck;

    // UI Components — Display Tab
    QWidget* m_displayTab;
    QSpinBox* m_tableRefreshSpin;
    QSpinBox* m_dashboardRefreshSpin;
    QComboBox* m_themeCombo;
    QCheckBox* m_showDataTableCheck;
    QCheckBox* m_showDashboardCheck;

    // UI Components — Data Points Tab
    QWidget* m_dataPointsTab;
    QLineEdit* m_dpSearchEdit;
    QTreeWidget* m_dpTreeWidget;
    QPushButton* m_showAllButton;
    QPushButton* m_hideAllButton;
    QHash<QString, QTreeWidgetItem*> m_categoryItems; // category name -> tree item

    // UI Components — Window Tab
    QWidget* m_windowTab;
    QPushButton* m_restoreGeometryButton;
    QPushButton* m_saveCurrentGeometryButton;

    // Settings reference
    Settings& m_settings;
    
    // LiveDataTable pointer for immediate visibility updates
    LiveDataTable* m_liveDataTable = nullptr;
    
    // Flag to prevent recursive toggling during batch updates
    bool m_updatingTree = false;
};


#endif // SETTINGS_DIALOG_H
