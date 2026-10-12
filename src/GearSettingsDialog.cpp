/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "GearSettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDoubleValidator>
#include <QDebug>
#include <QDialogButtonBox>

// Predefined transmission profiles
const GearSettingsDialog::TransmissionProfile GearSettingsDialog::s_profiles[] = {
    // Corrado 02A ATB
    {"02A ATB (Corrado)", {13.0341, 7.26225, 4.64025, 3.34995, 2.74275, 2.74275}},
    // Corrado AYN
    {"02A AYN (Corrado)", {13.918152, 7.802712, 4.95498, 3.577164, 2.785, 2.785}},
    // Corrado CBA
    {"02A CBA (Corrado)", {13.816146, 7.745526, 4.97352, 3.536319, 2.812233, 2.812233}},
    // R32 FMP custom
    {"02M FMP (R32 Custom)", {10.870, 6.758, 4.757, 3.555, 2.897, 2.424}},
    // Corrado 02A ATB with 5th gear from ASD
    {"02A ATB + ASD 5th", {13.0341, 7.26225, 4.64025, 3.34995, 2.4702, 2.4702}},
    // Golf 3 CCM (VR6)
    {"02A CCM (VR6)", {11.1837, 6.588216, 4.432812, 3.504226, 2.839982, 2.839982}},
    // Golf 3 CSR (VR6 Syncro)
    {"02A CSR (VR6 Syncro)", {12.2525, 6.99998, 4.97874, 3.50844, 2.84934, 2.84934}},
    // S3
    {"02M DQB (S3)", {14.364, 8.862, 6.0018, 4.5696, 3.6373, 3.023}},
    // Golf3 16V
    {"02A CDA (Golf3 16V)", {11.052, 7.1617, 4.8187, 3.7907, 3.0834, 3.0834}},
};

GearSettingsDialog::GearSettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Gearbox Settings");
    resize(500, 400);
    
    setupUI();
}

GearSettingsDialog::~GearSettingsDialog() = default;

void GearSettingsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Transmission profile selector
    QLabel* profileLabel = new QLabel("Transmission Profile:", this);
    QComboBox* profileCombo = new QComboBox(this);
    
    for (const auto& profile : s_profiles) {
        profileCombo->addItem(profile.name);
    }
    
    connect(profileCombo, &QComboBox::currentIndexChanged,
            this, &GearSettingsDialog::loadDefaultTransmission);
    
    QHBoxLayout* profileLayout = new QHBoxLayout();
    profileLayout->addWidget(profileLabel);
    profileLayout->addWidget(profileCombo);
    profileLayout->addStretch();
    mainLayout->addLayout(profileLayout);

    // Gear ratio spinboxes
    QLabel* gearTitle = new QLabel("<b>Gear Ratios (RPM / Wheel Speed)</b>", this);
    gearTitle->setStyleSheet("font-weight: bold; color: #007bff;");
    mainLayout->addWidget(gearTitle);

    const char* GEAR_NAMES[] = {"1st Gear:", "2nd Gear:", "3rd Gear:",
                                "4th Gear:", "5th Gear:", "6th Gear:"};

    for (int i = 0; i < 6; ++i) {
        QHBoxLayout* gearLayout = new QHBoxLayout();
        
        QLabel* label = new QLabel(GEAR_NAMES[i], this);
        m_gearSpinboxes[i] = new QDoubleSpinBox(this);
        m_gearSpinboxes[i]->setRange(1.0, 30.0);
        m_gearSpinboxes[i]->setDecimals(4);
        m_gearSpinboxes[i]->setSingleStep(0.01);
        m_gearSpinboxes[i]->setValue(s_profiles[0].ratios[i]); // Default to first profile
        
        gearLayout->addWidget(label);
        gearLayout->addWidget(m_gearSpinboxes[i]);
        gearLayout->addStretch();
        
        mainLayout->addLayout(gearLayout);
    }

    // Description
    QLabel* infoLabel = new QLabel(
        "<b>Gear Ratio Definition:</b> Transmission ratio = Engine RPM / Wheel Speed<br>"
        "Lower ratios provide higher top speed but slower acceleration.<br>"
        "Higher ratios provide better acceleration but lower top speed."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #666; font-size: 11px;");
    mainLayout->addWidget(infoLabel);

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GearSettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &GearSettingsDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void GearSettingsDialog::loadDefaultTransmission(int index) {
    if (index < 0 || index >= static_cast<int>(std::size(s_profiles))) return;
    
    for (int i = 0; i < 6; ++i) {
        m_gearSpinboxes[i]->setValue(s_profiles[index].ratios[i]);
    }
}

void GearSettingsDialog::onAccepted() {
    // Emit the gear ratios changed signal
    QVector<double> ratios;
    for (int i = 0; i < 6; ++i) {
        ratios << m_gearSpinboxes[i]->value();
    }
    emit gearRatiosChanged(ratios);
    
    // Also emit writeGearboxData signal for compatibility
    emit writeGearboxData(m_gearSpinboxes[0]->value(), m_gearSpinboxes[1]->value(),
                          m_gearSpinboxes[2]->value(), m_gearSpinboxes[3]->value(),
                          m_gearSpinboxes[4]->value(), m_gearSpinboxes[5]->value(),
                          99);
}

double GearSettingsDialog::getGearRatio(int gear) const {
    if (gear < 0 || gear >= 6) return 0.0;
    return m_gearSpinboxes[gear] ? m_gearSpinboxes[gear]->value() : 0.0;
}

void GearSettingsDialog::setGearRatio(int gear, double value) {
    if (gear < 0 || gear >= 6) return;
    if (m_gearSpinboxes[gear]) {
        m_gearSpinboxes[gear]->setValue(value);
    }
}

QVector<double> GearSettingsDialog::getGearRatios() const {
    QVector<double> ratios;
    for (int i = 0; i < 6; ++i) {
        if (m_gearSpinboxes[i]) {
            ratios << m_gearSpinboxes[i]->value();
        }
    }
    return ratios;
}