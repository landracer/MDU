/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "BoostConfigDialog.h"
#include "Settings.h"
#include "N75TableWidget.h"
#include "N75PidSettingsWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QTabWidget>
#include <QDialogButtonBox>

BoostConfigDialog::BoostConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("N75 Boost Config");
    resize(800, 600);
    
    setupUI();
    
    // Load settings if available
    auto& settings = Settings::instance();
    m_normalDuty = settings.getN75NormalDutyCycle();
    m_raceDuty = settings.getN75RaceDutyCycle();
}

BoostConfigDialog::~BoostConfigDialog() = default;

void BoostConfigDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Mode selector (Low/Normal vs High/Race)
    QHBoxLayout* modeLayout = new QHBoxLayout();
    QLabel* modeLabel = new QLabel("Boost Mode:");
    m_modeCombo = new QComboBox();
    m_modeCombo->addItem("Low / Normal");
    m_modeCombo->addItem("High / Race");
    connect(m_modeCombo, &QComboBox::currentIndexChanged, this, &BoostConfigDialog::onModeChanged);
    
    modeLayout->addWidget(modeLabel);
    modeLayout->addWidget(m_modeCombo);
    modeLayout->addStretch();
    mainLayout->addLayout(modeLayout);

    // Tab widget for tables and PID settings
    QTabWidget* tabWidget = new QTabWidget(this);

    // N75 Table tab - shows either low or high mode table based on selection
    QWidget* tableTab = new QWidget();
    QVBoxLayout* tableLayout = new QVBoxLayout(tableTab);
    tableLayout->setContentsMargins(5, 5, 5, 5);
    tableLayout->setSpacing(10);

    QLabel* tableInfoLabel = new QLabel("Configure gear-specific N75 maps:\n"
                                        "Rows: Duty (even) / Setpoint (odd)\n"
                                        "Columns: RPM bins (0-16000+)");
    tableInfoLabel->setWordWrap(true);
    tableInfoLabel->setStyleSheet("color: #666; font-size: 11px;");
    tableLayout->addWidget(tableInfoLabel);

    // N75 Low Table
    m_lowTable = new N75TableWidget(0, false, this);
    tableLayout->addWidget(m_lowTable);

    // N75 High Table (hidden by default)
    m_highTable = new N75TableWidget(0, true, this);
    m_highTable->hide();
    tableLayout->addWidget(m_highTable);

    tabWidget->addTab(tableTab, "Gear Maps");

    // PID Settings tab
    QWidget* pidTab = new QWidget();
    QVBoxLayout* pidLayout = new QVBoxLayout(pidTab);
    pidLayout->setContentsMargins(5, 5, 5, 5);

    m_pidSettings = new N75PidSettingsWidget(this);
    pidLayout->addWidget(m_pidSettings);

    tabWidget->addTab(pidTab, "PID Parameters");

    mainLayout->addWidget(tabWidget, 1); // Take up remaining space

    // Duty cycle summary (display for current mode)
    QHBoxLayout* dutyLayout = new QHBoxLayout();
    QLabel* dutyLabel = new QLabel("Current Mode Duty Cycle:");
    QSpinBox* currentDutySpin = new QSpinBox();
    currentDutySpin->setRange(0, 255);
    if (m_modeCombo && m_modeCombo->currentIndex() == 0) {
        currentDutySpin->setValue(m_normalDuty);
    } else {
        currentDutySpin->setValue(m_raceDuty);
    }
    connect(currentDutySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this, currentDutySpin](int value) {
                if (m_modeCombo && m_modeCombo->currentIndex() == 0) {
                    m_normalDuty = qBound(0, value, 255);
                } else {
                    m_raceDuty = qBound(0, value, 255);
                }
            });
    
    dutyLayout->addWidget(dutyLabel);
    dutyLayout->addWidget(currentDutySpin);
    dutyLayout->addStretch();
    mainLayout->addLayout(dutyLayout);

    // Description
    QLabel* infoLabel = new QLabel(
        "<b>Boost Control Configuration</b><br>"
        "• Duty Cycle: 0-255 (0% to 100% PWM for N75 valve)<br>"
        "• Setpoint: -0.5 to 2.5 bar target boost pressure<br>"
        "• PID Parameters control response characteristics"
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #333; font-size: 12px;");
    mainLayout->addWidget(infoLabel);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &BoostConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BoostConfigDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Connect signals from PID settings
    if (m_pidSettings) {
        connect(m_pidSettings, &N75PidSettingsWidget::settingsChanged, 
                this, []() { qDebug() << "PID settings changed"; });
    }
}

// ============================================================================
// Public Method Implementations
// ============================================================================

int BoostConfigDialog::getCurrentMode() const {
    return m_modeCombo ? m_modeCombo->currentIndex() : 0;
}

void BoostConfigDialog::setCurrentMode(int mode) {
    if (m_modeCombo && mode >= 0 && mode <= 1) {
        m_modeCombo->setCurrentIndex(mode);
        onModeChanged(mode);
    }
}

// N75 Table access methods
void BoostConfigDialog::setDutyValue(int gear, int rpmBin, quint8 value) {
    if (!m_lowTable || !m_highTable) return;
    N75TableWidget* table = m_lowModeVisible ? m_lowTable : m_highTable;
    table->setDutyValue(gear, rpmBin, value);
}

quint8 BoostConfigDialog::getDutyValue(int gear, int rpmBin) const {
    if (!m_lowTable || !m_highTable) return 0;
    const N75TableWidget* table = m_lowModeVisible ? m_lowTable : m_highTable;
    return table->getDutyValue(gear, rpmBin);
}

void BoostConfigDialog::setSetpointValue(int gear, int rpmBin, double value) {
    if (!m_lowTable || !m_highTable) return;
    N75TableWidget* table = m_lowModeVisible ? m_lowTable : m_highTable;
    table->setSetpointValue(gear, rpmBin, value);
}

double BoostConfigDialog::getSetpointValue(int gear, int rpmBin) const {
    if (!m_lowTable || !m_highTable) return 0.0;
    const N75TableWidget* table = m_lowModeVisible ? m_lowTable : m_highTable;
    return table->getSetpointValue(gear, rpmBin);
}

// PID Settings access methods
double BoostConfigDialog::getAKp() const { 
    return m_pidSettings ? m_pidSettings->getAKp() : 2.5; 
}
void BoostConfigDialog::setAKp(double value) { 
    if (m_pidSettings) m_pidSettings->setAKp(value); 
}

double BoostConfigDialog::getAKi() const { 
    return m_pidSettings ? m_pidSettings->getAKi() : 0.5; 
}
void BoostConfigDialog::setAKi(double value) { 
    if (m_pidSettings) m_pidSettings->setAKi(value); 
}

double BoostConfigDialog::getAKd() const { 
    return m_pidSettings ? m_pidSettings->getAKd() : 5.0; 
}
void BoostConfigDialog::setAKd(double value) { 
    if (m_pidSettings) m_pidSettings->setAKd(value); 
}

double BoostConfigDialog::getCKp() const { 
    return m_pidSettings ? m_pidSettings->getCKp() : 2.5; 
}
void BoostConfigDialog::setCKp(double value) { 
    if (m_pidSettings) m_pidSettings->setCKp(value); 
}

double BoostConfigDialog::getCKi() const { 
    return m_pidSettings ? m_pidSettings->getCKi() : 0.5; 
}
void BoostConfigDialog::setCKi(double value) { 
    if (m_pidSettings) m_pidSettings->setCKi(value); 
}

double BoostConfigDialog::getCKd() const { 
    return m_pidSettings ? m_pidSettings->getCKd() : 5.0; 
}
void BoostConfigDialog::setCKd(double value) { 
    if (m_pidSettings) m_pidSettings->setCKd(value); 
}

double BoostConfigDialog::getAAT() const { 
    return m_pidSettings ? m_pidSettings->getAAT() : 1.0; 
}
void BoostConfigDialog::setAAT(double value) { 
    if (m_pidSettings) m_pidSettings->setAAT(value); 
}

double BoostConfigDialog::getCAT() const { 
    return m_pidSettings ? m_pidSettings->getCAT() : 1.0; 
}
void BoostConfigDialog::setCAT(double value) { 
    if (m_pidSettings) m_pidSettings->setCAT(value); 
}

bool BoostConfigDialog::isEnabled() const { 
    return m_pidSettings ? m_pidSettings->isEnabled() : true; 
}
void BoostConfigDialog::setEnabled(bool enabled) { 
    if (m_pidSettings) m_pidSettings->setEnabled(enabled); 
}

double BoostConfigDialog::getMaxBoost() const { 
    return m_pidSettings ? m_pidSettings->getMaxBoost() : 1.5; 
}
void BoostConfigDialog::setMaxBoost(double value) { 
    if (m_pidSettings) m_pidSettings->setMaxBoost(value); 
}

// ============================================================================
// Private Slots
// ============================================================================

void BoostConfigDialog::onModeChanged(int index) {
    bool isHighMode = (index == 1);

    // Show/hide tables based on selected mode
    if (m_lowTable && m_highTable) {
        if (isHighMode) {
            m_lowTable->hide();
            m_highTable->show();
        } else {
            m_lowTable->show();
            m_highTable->hide();
        }
    }

    // Update duty cycle display to show current mode's value
    QSpinBox* dutySpin = findChild<QSpinBox*>();
    if (dutySpin) {
        int currentValue = isHighMode ? m_raceDuty : m_normalDuty;
        dutySpin->setValue(currentValue);
    }
}

void BoostConfigDialog::onNormalDutyChanged(int value) {
    Q_UNUSED(value);
    // This slot is kept for compatibility but now uses current mode logic
}

void BoostConfigDialog::onRaceDutyChanged(int value) {
    Q_UNUSED(value);
    // This slot is kept for compatibility but now uses current mode logic
}