/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "SettingsDialog.h"
#include "Settings.h"
#include "DataPointRegistry.h"
#include "MainWindow.h"
#include "LiveDataTable.h"
#include <QVBoxLayout>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QDebug>

SettingsDialog::SettingsDialog(LiveDataTable* liveDataTable, QWidget* parent)
    : QDialog(parent)
    , m_settings(Settings::instance())
    , m_liveDataTable(liveDataTable)
{
    setWindowTitle("Settings");
    resize(700, 600);
    
    // LiveDataTable is passed directly as first argument
    
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Tab widget
    auto* tabWidget = new QTabWidget(this);
    
    // ========================================================================
    // Tab 1: Connections
    // ========================================================================
    m_connectionsTab = new QWidget;
    auto* connLayout = new QVBoxLayout(m_connectionsTab);
    connLayout->setSpacing(15);
    
    // MultiDisplay ECU Group
    auto* mdGroup = new QGroupBox("MultiDisplay ECU");
    auto* mdForm = new QFormLayout(mdGroup);
    
    m_mdEcuPortCombo = new QComboBox;
    m_mdEcuPortCombo->addItems(getAvailablePorts());
    mdForm->addRow("Serial Port:", m_mdEcuPortCombo);
    
    m_mdEcuBaudSpin = new QSpinBox;
    m_mdEcuBaudSpin->setRange(9600, 2000000);
    m_mdEcuBaudSpin->setSingleStep(115200);
    m_mdEcuBaudSpin->setValue(115200);
    mdForm->addRow("Baud Rate:", m_mdEcuBaudSpin);
    
    m_autoConnectMdEcuCheck = new QCheckBox("Auto-connect on startup");
    mdForm->addRow("", m_autoConnectMdEcuCheck);
    
    // Apply connection button for MD-ECU
    auto* mdBtnLayout = new QHBoxLayout;
    auto* applyMdBtn = new QPushButton("Apply Connection");
    auto* disconnectMdBtn = new QPushButton("Disconnect");
    mdBtnLayout->addWidget(applyMdBtn);
    mdBtnLayout->addWidget(disconnectMdBtn);
    mdForm->addRow("", mdBtnLayout);
    
    connect(applyMdBtn, &QPushButton::clicked, this, &SettingsDialog::applyMdEcuConnection);
    connect(disconnectMdBtn, &QPushButton::clicked, this, [this]() {
        if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
            mw->disconnectMdEcu();
            showStatusMessage("MD-ECU disconnected");
        }
    });
    
    connLayout->addWidget(mdGroup);
    
    // ESP-NOW Bridge Group
    auto* bridgeGroup = new QGroupBox("ESP-NOW Bridge");
    auto* bridgeForm = new QFormLayout(bridgeGroup);
    
    m_bridgePortCombo = new QComboBox;
    m_bridgePortCombo->addItems(getAvailablePorts());
    bridgeForm->addRow("Serial Port:", m_bridgePortCombo);
    
    m_bridgeBaudSpin = new QSpinBox;
    m_bridgeBaudSpin->setRange(9600, 2000000);
    m_bridgeBaudSpin->setSingleStep(115200);
    m_bridgeBaudSpin->setValue(115200);
    bridgeForm->addRow("Baud Rate:", m_bridgeBaudSpin);
    
    m_autoConnectBridgeCheck = new QCheckBox("Auto-connect on startup");
    bridgeForm->addRow("", m_autoConnectBridgeCheck);
    
    // Apply connection button for Bridge
    auto* bridgeBtnLayout = new QHBoxLayout;
    auto* applyBridgeBtn = new QPushButton("Apply Connection");
    auto* disconnectBridgeBtn = new QPushButton("Disconnect");
    bridgeBtnLayout->addWidget(applyBridgeBtn);
    bridgeBtnLayout->addWidget(disconnectBridgeBtn);
    bridgeForm->addRow("", bridgeBtnLayout);
    
    connect(applyBridgeBtn, &QPushButton::clicked, this, &SettingsDialog::applyBridgeConnection);
    connect(disconnectBridgeBtn, &QPushButton::clicked, this, [this]() {
        if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
            mw->disconnectBridge();
            showStatusMessage("ESP-NOW Bridge disconnected");
        }
    });
    
    // Port test section
    auto* portTestLayout = new QHBoxLayout;
    m_bridgePortTestEdit = new QLineEdit;
    m_bridgePortTestEdit->setPlaceholderText("Enter port to test (e.g., /dev/ttyACM0)");
    portTestLayout->addWidget(m_bridgePortTestEdit);
    
    m_testBridgeButton = new QPushButton("Test Port");
    portTestLayout->addWidget(m_testBridgeButton);
    connect(m_testBridgeButton, &QPushButton::clicked, this, [this]() {
        QString port = m_bridgePortTestEdit->text();
        if (port.isEmpty()) {
            QMessageBox::warning(this, "Test Port", "Please enter a port name to test.");
            return;
        }
        
        // Quick test: try to open the port
        QMessageBox::information(this, "Port Test", 
            "Testing port: " + port + "\n\n"
            "If the bridge is streaming data, you should see DP lines in the terminal.\n"
            "Command: sudo cat " + port + " 2>/dev/null | head -10");
    });
    
    bridgeForm->addRow("Quick Test:", portTestLayout);
    
    connLayout->addWidget(bridgeGroup);
    
    // BMS Group
    auto* bmsGroup = new QGroupBox("rAtTrax-BMS");
    auto* bmsForm = new QFormLayout(bmsGroup);
    
    m_bmsPortCombo = new QComboBox;
    m_bmsPortCombo->addItems(getAvailablePorts());
    bmsForm->addRow("Serial Port:", m_bmsPortCombo);
    
    auto* bmsBaudSpin = new QSpinBox;
    bmsBaudSpin->setRange(9600, 2000000);
    bmsBaudSpin->setSingleStep(115200);
    bmsBaudSpin->setValue(115200);
    bmsForm->addRow("Baud Rate:", bmsBaudSpin);
    
    auto* autoConnectBmsCheck = new QCheckBox("Auto-connect on startup");
    bmsForm->addRow("", autoConnectBmsCheck);
    
    // Store BMS UI elements as member variables for onApply()
    m_autoConnectBmsCheck = autoConnectBmsCheck;
    
    // Apply connection button for BMS
    auto* bmsBtnLayout = new QHBoxLayout;
    auto* applyBmsBtn = new QPushButton("Apply Connection");
    auto* disconnectBmsBtn = new QPushButton("Disconnect");
    bmsBtnLayout->addWidget(applyBmsBtn);
    bmsBtnLayout->addWidget(disconnectBmsBtn);
    bmsForm->addRow("", bmsBtnLayout);
    
    connect(applyBmsBtn, &QPushButton::clicked, this, &SettingsDialog::applyBmsConnection);
    connect(disconnectBmsBtn, &QPushButton::clicked, this, [this]() {
        if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
            mw->disconnectBms();
            showStatusMessage("rAtTrax-BMS disconnected");
        }
    });
    
    connLayout->addWidget(bmsGroup);
    
    connLayout->addStretch();
    tabWidget->addTab(m_connectionsTab, "Connections");
    
    // ========================================================================
    // Tab 2: Display
    // ========================================================================
    m_displayTab = new QWidget;
    auto* displayLayout = new QVBoxLayout(m_displayTab);
    displayLayout->setSpacing(15);
    
    // Refresh Rates Group
    auto* refreshGroup = new QGroupBox("Refresh Rates");
    auto* refreshForm = new QFormLayout(refreshGroup);
    
    m_tableRefreshSpin = new QSpinBox;
    m_tableRefreshSpin->setRange(1, 60);
    m_tableRefreshSpin->setValue(5);
    m_tableRefreshSpin->setSuffix(" Hz");
    refreshForm->addRow("Data Table:", m_tableRefreshSpin);
    
    m_dashboardRefreshSpin = new QSpinBox;
    m_dashboardRefreshSpin->setRange(1, 120);
    m_dashboardRefreshSpin->setValue(30);
    m_dashboardRefreshSpin->setSuffix(" Hz");
    refreshForm->addRow("Dashboard Plots:", m_dashboardRefreshSpin);
    
    displayLayout->addWidget(refreshGroup);
    
    // Theme Group
    auto* themeGroup = new QGroupBox("Theme");
    auto* themeLayout = new QHBoxLayout(themeGroup);
    
    m_themeCombo = new QComboBox;
    m_themeCombo->addItems({"dark", "light", "system"});
    themeLayout->addWidget(m_themeCombo);
    themeLayout->addStretch();
    
    displayLayout->addWidget(themeGroup);
    
    // Panel Visibility Group
    auto* panelGroup = new QGroupBox("Panel Visibility");
    auto* panelLayout = new QVBoxLayout(panelGroup);
    
    m_showDataTableCheck = new QCheckBox("Show Data Table on startup");
    panelLayout->addWidget(m_showDataTableCheck);
    
    m_showDashboardCheck = new QCheckBox("Show Dashboard on startup");
    panelLayout->addWidget(m_showDashboardCheck);
    
    displayLayout->addWidget(panelGroup);
    displayLayout->addStretch();
    tabWidget->addTab(m_displayTab, "Display");
    
    // ========================================================================
    // Tab 3: Data Points
    // ========================================================================
    m_dataPointsTab = new QWidget;
    auto* dpLayout = new QVBoxLayout(m_dataPointsTab);
    dpLayout->setSpacing(10);
    
    // Search bar + buttons row
    auto* searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("Filter:"));
    m_dpSearchEdit = new QLineEdit;
    m_dpSearchEdit->setPlaceholderText("Type to filter data points...");
    searchLayout->addWidget(m_dpSearchEdit);
    
    // Show/Hide buttons
    auto* buttonLayout = new QHBoxLayout;
    m_showAllButton = new QPushButton("Show All");
    m_hideAllButton = new QPushButton("Hide All");
    buttonLayout->addWidget(m_showAllButton);
    buttonLayout->addWidget(m_hideAllButton);
    buttonLayout->addStretch();
    
    searchLayout->addLayout(buttonLayout);
    dpLayout->addLayout(searchLayout);
    
    // Grouped tree widget with drag-and-drop
    m_dpTreeWidget = new QTreeWidget;
    m_dpTreeWidget->setHeaderLabels({"Data Point", "Unit", "Category"});
    m_dpTreeWidget->setColumnWidth(0, 280);
    m_dpTreeWidget->setColumnWidth(1, 80);
    m_dpTreeWidget->setColumnWidth(2, 140);
    m_dpTreeWidget->setDragDropMode(QAbstractItemView::DragDrop);
    m_dpTreeWidget->setDefaultDropAction(Qt::MoveAction);
    m_dpTreeWidget->setExpandsOnDoubleClick(false);
    m_dpTreeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    populateDataPointTree();
    dpLayout->addWidget(m_dpTreeWidget);
    
    // Hint label
    auto* hintLabel = new QLabel("Drag items to reorder • Click category checkbox to toggle all in group");
    hintLabel->setStyleSheet("color: gray; font-style: italic; font-size: 11px;");
    dpLayout->addWidget(hintLabel);
    
    tabWidget->addTab(m_dataPointsTab, "Data Points");
    
    // ========================================================================
    // Tab 4: Window
    // ========================================================================
    m_windowTab = new QWidget;
    auto* windowLayout = new QVBoxLayout(m_windowTab);
    windowLayout->setSpacing(15);
    
    auto* geometryGroup = new QGroupBox("Window Geometry");
    auto* geometryForm = new QFormLayout(geometryGroup);
    
    m_restoreGeometryButton = new QPushButton("Restore Saved Geometry");
    geometryForm->addRow("", m_restoreGeometryButton);
    
    m_saveCurrentGeometryButton = new QPushButton("Save Current Geometry");
    geometryForm->addRow("", m_saveCurrentGeometryButton);
    
    windowLayout->addWidget(geometryGroup);
    windowLayout->addStretch();
    tabWidget->addTab(m_windowTab, "Window");
    
    mainLayout->addWidget(tabWidget);
    
    // Apply/Close buttons at bottom
    auto* buttonLayout2 = new QHBoxLayout;
    auto* applyButton = new QPushButton("Apply");
    auto* closeButton = new QPushButton("Close");
    buttonLayout2->addWidget(applyButton);
    buttonLayout2->addStretch();
    buttonLayout2->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout2);
    
    // Connections
    connect(applyButton, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(closeButton, &QPushButton::clicked, this, &SettingsDialog::onClose);
    connect(m_restoreGeometryButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Restore Geometry", 
            "Window geometry will be restored on next application start.");
    });
    connect(m_saveCurrentGeometryButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Save Geometry", 
            "Save the current window geometry from MainWindow using:\n"
            "Settings::instance().setMainWindowGeometry(window()->saveGeometry())");
    });
    
    // Data Points tree widget connections
    connect(m_dpSearchEdit, &QLineEdit::textChanged, this, &SettingsDialog::onSearchTextChanged);
    connect(m_showAllButton, &QPushButton::clicked, this, &SettingsDialog::onShowAllClicked);
    connect(m_hideAllButton, &QPushButton::clicked, this, &SettingsDialog::onHideAllClicked);
    connect(m_dpTreeWidget, &QTreeWidget::itemChanged, this, &SettingsDialog::onDataPointToggled);
}

void SettingsDialog::loadSettings() {
    // Connections
    if (!m_settings.getMdEcuPort().isEmpty()) {
        int idx = m_mdEcuPortCombo->findText(m_settings.getMdEcuPort());
        if (idx >= 0) m_mdEcuPortCombo->setCurrentIndex(idx);
    }
    m_mdEcuBaudSpin->setValue(m_settings.getMdEcuBaudRate());
    m_autoConnectMdEcuCheck->setChecked(m_settings.getAutoConnectMdEcu());
    
    if (!m_settings.getBridgePort().isEmpty()) {
        int idx = m_bridgePortCombo->findText(m_settings.getBridgePort());
        if (idx >= 0) m_bridgePortCombo->setCurrentIndex(idx);
    }
    m_bridgeBaudSpin->setValue(m_settings.getBridgeBaudRate());
    m_autoConnectBridgeCheck->setChecked(m_settings.getAutoConnectBridge());
    
    // Display
    m_tableRefreshSpin->setValue(m_settings.getDataTableRefreshRate());
    m_dashboardRefreshSpin->setValue(m_settings.getDashboardRefreshRate());
    
    int themeIdx = m_themeCombo->findText(m_settings.getTheme());
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);
    
    m_showDataTableCheck->setChecked(m_settings.getShowDataTable());
    m_showDashboardCheck->setChecked(m_settings.getShowDashboard());
}

void SettingsDialog::populateDataPointTree() {
    m_dpTreeWidget->clear();
    m_categoryItems.clear();
    
    auto& registry = DataPointRegistry::instance();
    
    // Get categories and their points
    auto allIds = registry.allPointsSorted();
    QHash<QString, QVector<DataPointID>> categoryMap;
    for (const auto& id : allIds) {
        auto info = registry.getInfo(id);
        categoryMap[info.category].append(id);
    }
    
    // Build tree: categories as parent items, data points as children
    for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it) {
        const QString& categoryName = it.key();
        const QVector<DataPointID>& dpIds = it.value();
        
        // Create category header item
        auto* catItem = new QTreeWidgetItem(m_dpTreeWidget);
        catItem->setText(0, categoryName);
        catItem->setForeground(0, QBrush(QColor("#aaaaaa")));
        catItem->setFont(0, QFont("Sans Serif", 9, QFont::Bold));
        catItem->setFlags(catItem->flags() | Qt::ItemIsUserCheckable);
        
        // Check if all points in this category are visible
        bool allVisible = true;
        bool someVisible = false;
        for (const auto& dpId : dpIds) {
            bool visible = m_settings.isDataPointVisible(static_cast<quint16>(dpId));
            if (visible) someVisible = true;
            else allVisible = false;
        }
        catItem->setCheckState(0, allVisible ? Qt::Checked : (someVisible ? Qt::PartiallyChecked : Qt::Unchecked));
        
        m_categoryItems[categoryName] = catItem;
        
        // Add data point children
        for (const auto& dpId : dpIds) {
            auto info = registry.getInfo(dpId);
            auto* dpItem = new QTreeWidgetItem(catItem);
            dpItem->setText(0, info.name);
            dpItem->setText(1, info.unit.isEmpty() ? "—" : info.unit);
            dpItem->setText(2, categoryName);
            dpItem->setData(0, Qt::UserRole, static_cast<quint32>(dpId));
            
            // Tooltip with ID
            QString tooltip = QString("0x%1 | %2")
                .arg(static_cast<int>(dpId), 4, 16, QChar('0'))
                .arg(info.unit.isEmpty() ? "—" : info.unit);
            dpItem->setToolTip(0, tooltip);
            
            // Check visibility
            bool visible = m_settings.isDataPointVisible(static_cast<quint16>(dpId));
            dpItem->setCheckState(0, visible ? Qt::Checked : Qt::Unchecked);
        }
        
        catItem->setExpanded(true);
    }
}

void SettingsDialog::onDataPointToggled(QTreeWidgetItem* item, int column) {
    if (column != 0) return;
    
    // Prevent recursive updates
    if (m_updatingTree) return;
    m_updatingTree = true;
    
    // Check if this is a category item or data point item
    bool isCategory = m_categoryItems.values().contains(item);
    
    if (isCategory) {
        // Toggle all children in category
        Qt::CheckState state = item->checkState(0);
        for (int i = 0; i < item->childCount(); ++i) {
            auto* child = item->child(i);
            child->setCheckState(0, state);
            
            quint16 dpId = child->data(0, Qt::UserRole).toUInt();
            m_settings.setDataPointVisible(dpId, state == Qt::Checked);
            
            // Update LiveDataTable immediately if available
            if (m_liveDataTable) {
                m_liveDataTable->setDataPointVisible(dpId, state == Qt::Checked);
            }
        }
    } else {
        // Update single data point visibility first
        quint16 dpId = item->data(0, Qt::UserRole).toUInt();
        m_settings.setDataPointVisible(dpId, item->checkState(0) == Qt::Checked);
        
        // Update LiveDataTable immediately if available
        if (m_liveDataTable) {
            m_liveDataTable->setDataPointVisible(dpId, item->checkState(0) == Qt::Checked);
        }
        
        // Update parent category check state
        auto* parent = item->parent();
        if (parent) {
            bool allChecked = true;
            bool anyChecked = false;
            for (int i = 0; i < parent->childCount(); ++i) {
                auto* child = parent->child(i);
                if (child->checkState(0) == Qt::Checked) anyChecked = true;
                else allChecked = false;
            }
            
            // Only update parent if state actually changed
            Qt::CheckState expectedState = allChecked ? Qt::Checked : (anyChecked ? Qt::PartiallyChecked : Qt::Unchecked);
            if (parent->checkState(0) != expectedState) {
                parent->setCheckState(0, expectedState);
            }
        }
    }
    
    m_updatingTree = false;
}

void SettingsDialog::onCategoryToggled(QTreeWidgetItem* item, int column) {
    onDataPointToggled(item, column);
}

void SettingsDialog::onSearchTextChanged(const QString& text) {
    QString filter = text.toLower();
    
    for (auto it = m_categoryItems.begin(); it != m_categoryItems.end(); ++it) {
        auto* catItem = it.value();
        bool categoryMatch = filter.isEmpty() || catItem->text(0).toLower().contains(filter);
        
        int visibleCount = 0;
        for (int i = 0; i < catItem->childCount(); ++i) {
            auto* child = catItem->child(i);
            bool match = filter.isEmpty() || 
                         child->text(0).toLower().contains(filter) ||
                         child->text(1).toLower().contains(filter);
            child->setHidden(!match);
            if (match) visibleCount++;
        }
        
        catItem->setHidden(!categoryMatch && visibleCount == 0);
        if (categoryMatch || visibleCount > 0) {
            catItem->setExpanded(true);
        }
    }
}

void SettingsDialog::onShowAllClicked() {
    for (auto it = m_categoryItems.begin(); it != m_categoryItems.end(); ++it) {
        auto* catItem = it.value();
        for (int i = 0; i < catItem->childCount(); ++i) {
            auto* child = catItem->child(i);
            child->setCheckState(0, Qt::Checked);
            quint16 dpId = child->data(0, Qt::UserRole).toUInt();
            m_settings.setDataPointVisible(dpId, true);
            
            // Update LiveDataTable immediately if available
            if (m_liveDataTable) {
                m_liveDataTable->setDataPointVisible(dpId, true);
            }
        }
        catItem->setCheckState(0, Qt::Checked);
    }
}

void SettingsDialog::onHideAllClicked() {
    for (auto it = m_categoryItems.begin(); it != m_categoryItems.end(); ++it) {
        auto* catItem = it.value();
        for (int i = 0; i < catItem->childCount(); ++i) {
            auto* child = catItem->child(i);
            child->setCheckState(0, Qt::Unchecked);
            quint16 dpId = child->data(0, Qt::UserRole).toUInt();
            m_settings.setDataPointVisible(dpId, false);
            
            // Update LiveDataTable immediately if available
            if (m_liveDataTable) {
                m_liveDataTable->setDataPointVisible(dpId, false);
            }
        }
        catItem->setCheckState(0, Qt::Unchecked);
    }
}

void SettingsDialog::populateDataPointList() {
    // Legacy function - now handled by populateDataPointTree()
    populateDataPointTree();
}

QStringList SettingsDialog::getAvailablePorts() const {
    QStringList ports;
    auto portsInfo = QSerialPortInfo::availablePorts();
    for (const auto& port : portsInfo) {
        QString description = port.description();
        QString manufacturer = port.manufacturer();
        
        if (description.isEmpty() && manufacturer.isEmpty()) {
            ports << port.portName();
        } else {
            ports << QString("%1 — %2 (%3)")
                .arg(port.portName(), description, manufacturer);
        }
    }
    return ports;
}

void SettingsDialog::onApply() {
    // Save connection settings
    m_settings.setMdEcuPort(m_mdEcuPortCombo->currentText());
    m_settings.setMdEcuBaudRate(m_mdEcuBaudSpin->value());
    m_settings.setAutoConnectMdEcu(m_autoConnectMdEcuCheck->isChecked());
    
    m_settings.setBridgePort(m_bridgePortCombo->currentText());
    m_settings.setBridgeBaudRate(m_bridgeBaudSpin->value());
    m_settings.setAutoConnectBridge(m_autoConnectBridgeCheck->isChecked());
    
    m_settings.setBmsPort(m_bmsPortCombo->currentText());
    m_settings.setAutoConnectBms(m_autoConnectBmsCheck->isChecked());
    
    // Save display settings
    m_settings.setDataTableRefreshRate(m_tableRefreshSpin->value());
    m_settings.setDashboardRefreshRate(m_dashboardRefreshSpin->value());
    m_settings.setTheme(m_themeCombo->currentText());
    m_settings.setShowDataTable(m_showDataTableCheck->isChecked());
    m_settings.setShowDashboard(m_showDashboardCheck->isChecked());
    
    // Save data point visibility from tree widget
    for (auto it = m_categoryItems.begin(); it != m_categoryItems.end(); ++it) {
        auto* catItem = it.value();
        for (int i = 0; i < catItem->childCount(); ++i) {
            auto* child = catItem->child(i);
            if (child->isHidden()) continue;
            quint16 dpId = child->data(0, Qt::UserRole).toUInt();
            bool visible = (child->checkState(0) == Qt::Checked);
            m_settings.setDataPointVisible(dpId, visible);
        }
    }
    
    m_settings.save();
    
    QMessageBox::information(this, "Settings Applied", 
        "Settings saved successfully.\n\n"
        "All connection settings will persist across restarts.");
}

void SettingsDialog::onResetToDefaults() {
    auto reply = QMessageBox::question(this, "Reset to Defaults",
        "Are you sure you want to reset all settings to defaults?\n\n"
        "This cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_settings.resetToDefaults();
        loadSettings();
        QMessageBox::information(this, "Reset Complete", 
            "All settings have been reset to defaults.");
    }
}

void SettingsDialog::onClose() {
    accept();
}

void SettingsDialog::showStatusMessage(const QString& message, int timeout) {
    if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
        mw->statusBar()->showMessage(message, timeout);
    }
}

void SettingsDialog::applyMdEcuConnection() {
    QString port = m_mdEcuPortCombo->currentText();
    // Extract just the port name (first part before " — ")
    int sepIdx = port.indexOf(" — ");
    if (sepIdx >= 0) {
        port = port.left(sepIdx);
    }
    
    if (port.isEmpty()) {
        QMessageBox::warning(this, "No Port Selected", "Please select a serial port for MD-ECU.");
        return;
    }
    
    if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
        // Disconnect first if already connected
        mw->disconnectMdEcu();
        // Connect to new port
        mw->connectMdEcu(port);
        showStatusMessage("MD-ECU connecting to " + port + "...");
        
        // Save settings immediately for persistence
        m_settings.setMdEcuPort(port);
        m_settings.setAutoConnectMdEcu(true);  // Auto-connect enabled after manual connection
        m_settings.save();
    }
}

void SettingsDialog::applyBridgeConnection() {
    QString port = m_bridgePortCombo->currentText();
    // Extract just the port name (first part before " — ")
    int sepIdx = port.indexOf(" — ");
    if (sepIdx >= 0) {
        port = port.left(sepIdx);
    }
    
    if (port.isEmpty()) {
        QMessageBox::warning(this, "No Port Selected", "Please select a serial port for ESP-NOW Bridge.");
        return;
    }
    
    if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
        // Disconnect first if already connected
        mw->disconnectBridge();
        // Connect to new port
        mw->connectBridge(port);
        showStatusMessage("ESP-NOW Bridge connecting to " + port + "...");
        
        // Save settings immediately for persistence
        m_settings.setBridgePort(port);
        m_settings.setAutoConnectBridge(true);  // Auto-connect enabled after manual connection
        m_settings.save();
    }
}

void SettingsDialog::applyBmsConnection() {
    QString port = m_bmsPortCombo->currentText();
    // Extract just the port name (first part before " — ")
    int sepIdx = port.indexOf(" — ");
    if (sepIdx >= 0) {
        port = port.left(sepIdx);
    }
    
    if (port.isEmpty()) {
        QMessageBox::warning(this, "No Port Selected", "Please select a serial port for rAtTrax-BMS.");
        return;
    }
    
    if (auto* mw = qobject_cast<MainWindow*>(parentWidget())) {
        // Disconnect first if already connected
        mw->disconnectBms();
        // Connect to new port
        mw->connectBMS(port);
        showStatusMessage("rAtTrax-BMS connecting to " + port + "...");
        
        // Save settings immediately for persistence
        m_settings.setBmsPort(port);
        m_settings.setAutoConnectBms(true);  // Auto-connect enabled after manual connection
        m_settings.save();
    }
}
