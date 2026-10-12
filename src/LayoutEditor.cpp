/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "LayoutEditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>
#include <QDebug>

LayoutEditor::LayoutEditor(QWidget* parent)
    : QDialog(parent)
    , m_dpPicker(nullptr)
{
    setWindowTitle("Layout Editor — OpenDash Node Configuration");
    resize(800, 700);
    
    setupUI();
    
    // Disconnect signals during population to prevent callbacks before widgets are ready
    disconnect(m_nodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &LayoutEditor::onNodeChanged);
    disconnect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
               this, &LayoutEditor::onModeChanged);
    
    populateNodes();
    populateModes();
    buildSlotWidgets();
    
    // Reconnect signals now that everything is initialized
    connect(m_nodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LayoutEditor::onNodeChanged);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LayoutEditor::onModeChanged);
}

void LayoutEditor::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // ========================================================================
    // Top bar: Node selector + Mode selector + Actions
    // ========================================================================
    auto* topLayout = new QHBoxLayout();
    
    topLayout->addWidget(new QLabel("Node:"));
    m_nodeCombo = new QComboBox();
    topLayout->addWidget(m_nodeCombo);
    
    topLayout->addWidget(new QLabel("Mode:"));
    m_modeCombo = new QComboBox();
    topLayout->addWidget(m_modeCombo);
    
    topLayout->addStretch();
    
    auto* loadBtn = new QPushButton("Load");
    auto* saveBtn = new QPushButton("Save");
    auto* resetBtn = new QPushButton("Reset");
    auto* pushBtn = new QPushButton("Push to Node");
    
    topLayout->addWidget(loadBtn);
    topLayout->addWidget(saveBtn);
    topLayout->addWidget(resetBtn);
    topLayout->addWidget(pushBtn);
    
    mainLayout->addLayout(topLayout);
    
    // ========================================================================
    // Arc Gauge Configuration
    // ========================================================================
    auto* arcGroup = new QGroupBox("Arc Gauge (Center Display)");
    auto* arcForm = new QFormLayout(arcGroup);
    
    m_arcConfigWidget = new QWidget();
    auto* arcLayout = new QHBoxLayout(m_arcConfigWidget);
    
    m_arcDpLabel = new QLabel("Data Point:");
    m_arcDpButton = new QPushButton("Select...");
    m_arcMinSpin = new QSpinBox();
    m_arcMinSpin->setRange(0, 100000);
    m_arcMinSpin->setValue(0);
    m_arcMaxSpin = new QSpinBox();
    m_arcMaxSpin->setRange(0, 100000);
    m_arcMaxSpin->setValue(8000);
    
    arcLayout->addWidget(m_arcDpLabel);
    arcLayout->addWidget(m_arcDpButton);
    arcLayout->addStretch();
    arcLayout->addWidget(new QLabel("Min:"));
    arcLayout->addWidget(m_arcMinSpin);
    arcLayout->addWidget(new QLabel("Max:"));
    arcLayout->addWidget(m_arcMaxSpin);
    
    arcForm->addRow("", m_arcConfigWidget);
    mainLayout->addWidget(arcGroup);
    
    // ========================================================================
    // Slot Configurations (scrollable area)
    // ========================================================================
    auto* slotsGroup = new QGroupBox("Display Slots");
    auto* slotsMainLayout = new QVBoxLayout(slotsGroup);
    
    m_slotsContainer = new QWidget();
    auto* slotLayout = new QVBoxLayout(m_slotsContainer);
    slotLayout->setSpacing(8);
    
    // Create 16 slot widgets in a grid
    for (int i = 0; i < MAX_SLOTS; i++) {
        auto* slotWidget = new QWidget();
        auto* slotHLayout = new QHBoxLayout(slotWidget);
        slotHLayout->setContentsMargins(5, 2, 5, 2);
        
        auto* slotLabel = new QLabel(QString("Slot %1:").arg(i + 1));
        slotLabel->setFixedWidth(70);
        m_slotLabels.append(slotLabel);
        
        auto* slotButton = new QPushButton("Select...");
        slotButton->setMinimumWidth(150);
        m_slotButtons.append(slotButton);
        
        slotHLayout->addWidget(slotLabel);
        slotHLayout->addWidget(slotButton);
        slotHLayout->addStretch();
        
        slotLayout->addWidget(slotWidget);
    }
    
    slotLayout->addStretch();
    
    // Scroll area for slots
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(m_slotsContainer);
    
    slotsMainLayout->addWidget(scrollArea);
    mainLayout->addWidget(slotsGroup, 1);
    
    // ========================================================================
    // Status bar
    // ========================================================================
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: gray; font-size: 11px;");
    mainLayout->addWidget(m_statusLabel);
    
    // Connect signals
    connect(m_nodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LayoutEditor::onNodeChanged);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LayoutEditor::onModeChanged);
    connect(m_arcDpButton, &QPushButton::clicked, this, &LayoutEditor::onArcDpClicked);
    connect(resetBtn, &QPushButton::clicked, this, &LayoutEditor::onResetLayout);
    connect(pushBtn, &QPushButton::clicked, this, &LayoutEditor::onPushToNode);
    connect(saveBtn, &QPushButton::clicked, this, &LayoutEditor::onSaveLayout);
    connect(loadBtn, &QPushButton::clicked, this, &LayoutEditor::onLoadLayout);
    
    for (int i = 0; i < MAX_SLOTS; i++) {
        connect(m_slotButtons[i], &QPushButton::clicked, [this, i]() {
            onSlotDpClicked(i);
        });
    }
}

void LayoutEditor::populateNodes() {
    // OpenDash node types from mergecombo.md
    m_nodeCombo->addItems({
        "CENTER",     // Main dashboard
        "LEFT",       // Left gauge pod
        "RIGHT",      // Right gauge pod
        "GPS",        // GPS/telemetry node
        "POD1",       // Expansion pods
        "POD2",
        "POD3",
        "POD4",
        "POD5",
        "POD6",
        "POD7",
        "POD8",
        "RELAY_4CH",  // Relay controller
        "RELAY_8CH_A",
        "MOS_4CH_A"
    });
    
    m_currentNode = m_nodeCombo->currentText();
}

void LayoutEditor::populateModes() {
    // Display modes from mergecombo.md
    m_modeCombo->addItems({
        "ENGINE",  // Mode 0: RPM, boost, lambda, etc.
        "GPS",     // Mode 1: GPS speed, lat/lon, altitude
        "MD",      // Mode 2: MultiDisplay ECU overlay
        "OBD",     // Mode 3: OBD2-specific view
        "BMS",     // Mode 4: Battery management
        "RELAY",   // Mode 5: Relay/MOS controller status
        "CONFIG"   // Mode 6: System configuration
    });
}

void LayoutEditor::buildSlotWidgets() {
    m_slots.resize(MAX_SLOTS);
    
    // Reset all slot buttons to default state
    for (int i = 0; i < MAX_SLOTS; i++) {
        m_slotButtons[i]->setText("Select...");
        m_slotButtons[i]->setStyleSheet("");
        m_slotLabels[i]->setText(QString("Slot %1:").arg(i + 1));
        m_slots[i] = {DataPointID::Invalid, false};
    }
}

void LayoutEditor::onNodeChanged(int index) {
    if (m_constructing || index < 0) return;
    
    m_currentNode = m_nodeCombo->currentText();
    loadSavedLayout();
}

void LayoutEditor::onModeChanged(int index) {
    if (m_constructing || index < 0) return;
    
    m_currentMode = index;
    loadSavedLayout();
}

void LayoutEditor::onArcDpClicked() {
    if (!m_dpPicker) {
        m_dpPicker = new DpPickerDialog(this);
    }
    
    // Set current selection if any
    auto& registry = DataPointRegistry::instance();
    // Arc gauge defaults to RPM (0x0100)
    DataPointID currentArcId = DataPointID::RPM;
    m_dpPicker->setSelectedDataPoint(currentArcId);
    
    if (m_dpPicker->exec() == QDialog::Accepted) {
        DataPointID selectedId = m_dpPicker->getSelectedDataPoint();
        if (selectedId != DataPointID::Invalid) {
            auto info = DataPointRegistry::instance().getInfo(selectedId);
            m_arcDpLabel->setText(QString("Data Point: %1").arg(info.name));
            m_arcDpLabel->setStyleSheet("font-weight: bold;");
            
            // Set sensible min/max based on data point type
            if (selectedId == DataPointID::RPM) {
                m_arcMinSpin->setValue(0);
                m_arcMaxSpin->setValue(8000);
            } else if (selectedId == DataPointID::VehicleSpeed || selectedId == DataPointID::GPSSpeed) {
                m_arcMinSpin->setValue(0);
                m_arcMaxSpin->setValue(300);
            } else if (selectedId == DataPointID::BMSSOC) {
                m_arcMinSpin->setValue(0);
                m_arcMaxSpin->setValue(100);
            }
            
            showStatusMessage(QString("Arc gauge set to %1 (%2)").arg(info.name).arg(QString("0x%1").arg(static_cast<uint32_t>(selectedId), 4, 16, QChar('0'))));
        }
    }
}

void LayoutEditor::onSlotDpClicked(int slotIndex) {
    if (!m_dpPicker) {
        m_dpPicker = new DpPickerDialog(this);
    }
    
    // Set current selection for this slot
    DataPointID currentId = m_slots[slotIndex].hasValue ? m_slots[slotIndex].dpId : DataPointID::Invalid;
    if (currentId != DataPointID::Invalid) {
        m_dpPicker->setSelectedDataPoint(currentId);
    }
    
    if (m_dpPicker->exec() == QDialog::Accepted) {
        DataPointID selectedId = m_dpPicker->getSelectedDataPoint();
        if (selectedId != DataPointID::Invalid) {
            auto info = DataPointRegistry::instance().getInfo(selectedId);
            m_slotButtons[slotIndex]->setText(info.name);
            m_slotLabels[slotIndex]->setText(QString("Slot %1: %2").arg(slotIndex + 1).arg(info.name));
            
            // Color code the button based on category
            QString category = info.category;
            QString color;
            if (category.contains("ENGINE")) color = "color: #4CAF50;";
            else if (category.contains("BMS")) color = "color: #2196F3;";
            else if (category.contains("VESC")) color = "color: #FF9800;";
            else if (category.contains("GPS")) color = "color: #9C27B0;";
            else if (category.contains("IMU")) color = "color: #F44336;";
            else color = "color: #ffffff;";
            
            m_slotButtons[slotIndex]->setStyleSheet(color + "font-weight: bold;");
            
            m_slots[slotIndex] = {selectedId, true};
            showStatusMessage(QString("Slot %1 set to %2 (%3)").arg(slotIndex + 1).arg(info.name).arg(QString("0x%1").arg(static_cast<uint32_t>(selectedId), 4, 16, QChar('0'))));
        }
    }
}

void LayoutEditor::onResetLayout() {
    // Reset all slots to default (ENGINE mode layout)
    m_modeCombo->setCurrentIndex(0); // ENGINE mode
    
    // Default ENGINE mode layout from mergecombo.md
    QVector<DataPointID> defaultEngineSlots = {
        DataPointID::CoolantTemp,  // Slot 1
        DataPointID::OilPressure,  // Slot 2
        DataPointID::BoostPressure, // Slot 3
        DataPointID::Lambda,       // Slot 4
        DataPointID::Gear,         // Slot 5
        DataPointID::VehicleSpeed  // Slot 6
    };
    
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (i < defaultEngineSlots.size() && defaultEngineSlots[i] != DataPointID::Invalid) {
            m_slots[i] = {defaultEngineSlots[i], true};
            auto info = DataPointRegistry::instance().getInfo(defaultEngineSlots[i]);
            m_slotButtons[i]->setText(info.name);
            m_slotLabels[i]->setText(QString("Slot %1: %2").arg(i + 1).arg(info.name));
        } else {
            m_slots[i] = {DataPointID::Invalid, false};
            m_slotButtons[i]->setText("Select...");
            m_slotLabels[i]->setText(QString("Slot %1:").arg(i + 1));
        }
    }
    
    // Reset arc gauge to RPM
    m_arcDpLabel->setText("Data Point: Engine RPM");
    m_arcMinSpin->setValue(0);
    m_arcMaxSpin->setValue(8000);
    
    showStatusMessage("Layout reset to default ENGINE mode");
}

void LayoutEditor::onPushToNode() {
    // Build command string per mergecombo.md spec
    QString cmd = getCommandString();
    
    if (cmd.isEmpty()) {
        QMessageBox::warning(this, "Push Failed", 
            "No layout configured. Select data points for at least some slots before pushing.");
        return;
    }
    
    // Show command for now (later will send via OpenDashBridge)
    QMessageBox::information(this, "Push Command", 
        QString("Command to send via ESP-NOW bridge:\n\n%1\n\n"
                "Target: %2\n"
                "Mode: %3\n\n"
                "Note: This requires the OpenDash Bridge to be connected.\n"
                "The command will be sent as:\nLAYOUT:%4")
        .arg(cmd).arg(m_currentNode).arg(m_modeCombo->currentText()).arg(cmd));
    
    showStatusMessage(QString("Pushed layout to %1 (mode %2)").arg(m_currentNode, 
        QString::number(m_currentMode)));
}

void LayoutEditor::onSaveLayout() {
    if (!m_nodeCombo || !m_modeCombo) return;
    
    QSettings settings("MultidisplayUnified", "LayoutEditor");
    
    QString key = QString("node_%1_mode_%2").arg(m_currentNode).arg(QString::number(m_currentMode));
    
    // Save arc gauge config
    settings.setValue(key + "/arcDpId", static_cast<int>(DataPointID::RPM));
    settings.setValue(key + "/arcMin", m_arcMinSpin->value());
    settings.setValue(key + "/arcMax", m_arcMaxSpin->value());
    
    // Save slot configs (up to 16 slots)
    settings.setValue(key + "/slotCount", MAX_SLOTS);
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (m_slots[i].hasValue) {
            settings.setValue(key + QString("/slot%1").arg(i), static_cast<int>(m_slots[i].dpId));
        } else {
            settings.setValue(key + QString("/slot%1").arg(i), -1);
        }
    }
    
    showStatusMessage(QString("Layout saved for %1 mode %2").arg(m_currentNode).arg(m_modeCombo->currentText()));
}

void LayoutEditor::onLoadLayout() {
    if (!m_nodeCombo || !m_modeCombo) return;
    
    QSettings settings("MultidisplayUnified", "LayoutEditor");
    
    QString key = QString("node_%1_mode_%2").arg(m_currentNode).arg(QString::number(m_currentMode));
    
    // Load arc gauge config
    int arcDpId = settings.value(key + "/arcDpId", static_cast<int>(DataPointID::RPM)).toInt();
    m_arcMinSpin->setValue(settings.value(key + "/arcMin", 0).toInt());
    m_arcMaxSpin->setValue(settings.value(key + "/arcMax", 8000).toInt());
    
    // Load slot configs
    int slotCount = settings.value(key + "/slotCount", MAX_SLOTS).toInt();
    for (int i = 0; i < qMin(slotCount, MAX_SLOTS); i++) {
        if (!m_slotButtons[i] || !m_slotLabels[i]) continue;
        int dpIdVal = settings.value(key + QString("/slot%1").arg(i), -1).toInt();
        if (dpIdVal >= 0) {
            DataPointID dpId = static_cast<DataPointID>(static_cast<uint32_t>(dpIdVal));
            if (DataPointRegistry::instance().isValid(dpId)) {
                m_slots[i] = {dpId, true};
                auto info = DataPointRegistry::instance().getInfo(dpId);
                m_slotButtons[i]->setText(info.name);
                m_slotLabels[i]->setText(QString("Slot %1: %2").arg(i + 1).arg(info.name));
            }
        } else {
            m_slots[i] = {DataPointID::Invalid, false};
            m_slotButtons[i]->setText("Select...");
            m_slotLabels[i]->setText(QString("Slot %1:").arg(i + 1));
        }
    }
}

void LayoutEditor::loadSavedLayout() {
    // Auto-load saved layout when node/mode changes
    onLoadLayout();
}

QString LayoutEditor::getCommandString() const {
    // Build LAYOUT command per mergecombo.md spec:
    // LAYOUT:<node>:<mode>:<arc_dp>:<min>:<max>:<slot_dps...>
    
    QString cmd = QString("LAYOUT:%1:%2:")
        .arg(m_currentNode).arg(QString::number(m_currentMode));
    
    // Arc gauge DP ID (default to RPM if not set)
    cmd += "0x0100:";  // RPM
    cmd += QString("%1:%2:").arg(m_arcMinSpin->value()).arg(m_arcMaxSpin->value());
    
    // Slot DP IDs
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (m_slots[i].hasValue) {
            cmd += QString("0x%1").arg(static_cast<uint32_t>(m_slots[i].dpId), 4, 16, QChar('0'));
        } else {
            cmd += "0x0000";  // Empty slot
        }
        if (i < MAX_SLOTS - 1) cmd += ":";
    }
    
    return cmd;
}

void LayoutEditor::showStatusMessage(const QString& msg, int timeout) {
    m_statusLabel->setText(msg);
    if (timeout > 0) {
        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this]() {
            m_statusLabel->setText("Ready");
        });
        timer->start(timeout);
    }
}
