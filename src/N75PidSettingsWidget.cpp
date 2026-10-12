/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "N75PidSettingsWidget.h"
#include <QLabel>
#include <QDoubleValidator>

N75PidSettingsWidget::N75PidSettingsWidget(QWidget* parent)
    : QWidget(parent) {
    setupUI();
}

void N75PidSettingsWidget::setupUI() {
    auto* layout = new QGridLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(10, 10, 10, 10);

    int row = 0;

    // Group title: Acceleration PID
    QLabel* accelTitle = new QLabel("<b>Acceleration PID (aKp, aKi, aKd)</b>", this);
    accelTitle->setStyleSheet("font-weight: bold; color: #007bff;");
    layout->addWidget(accelTitle, row++, 0, 1, -1);

    // Acceleration PID parameters
    QLabel* aKpLabel = new QLabel("aKp:", this);
    m_aKp = new QDoubleSpinBox(this);
    m_aKp->setRange(0.0, 100.0);
    m_aKp->setSingleStep(0.1);
    m_aKp->setValue(2.5);

    QLabel* aKiLabel = new QLabel("aKi:", this);
    m_aKi = new QDoubleSpinBox(this);
    m_aKi->setRange(0.0, 10.0);
    m_aKi->setSingleStep(0.01);
    m_aKi->setValue(0.5);

    QLabel* aKdLabel = new QLabel("aKd:", this);
    m_aKd = new QDoubleSpinBox(this);
    m_aKd->setRange(0.0, 50.0);
    m_aKd->setSingleStep(0.1);
    m_aKd->setValue(5.0);

    layout->addWidget(aKpLabel, row, 0);
    layout->addWidget(m_aKp, row++, 1);
    layout->addWidget(aKiLabel, row, 0);
    layout->addWidget(m_aKi, row++, 1);
    layout->addWidget(aKdLabel, row, 0);
    layout->addWidget(m_aKd, row++, 1);

    // Spacer
    QLabel* spacer = new QLabel("", this);
    layout->addWidget(spacer, row++, 0, 1, -1);

    // Group title: Control PID
    QLabel* controlTitle = new QLabel("<b>Control/Compression PID (cKp, cKi, cKd)</b>", this);
    controlTitle->setStyleSheet("font-weight: bold; color: #28a745;");
    layout->addWidget(controlTitle, row++, 0, 1, -1);

    // Control PID parameters
    QLabel* cKpLabel = new QLabel("cKp:", this);
    m_cKp = new QDoubleSpinBox(this);
    m_cKp->setRange(0.0, 100.0);
    m_cKp->setSingleStep(0.1);
    m_cKp->setValue(2.5);

    QLabel* cKiLabel = new QLabel("cKi:", this);
    m_cKi = new QDoubleSpinBox(this);
    m_cKi->setRange(0.0, 10.0);
    m_cKi->setSingleStep(0.01);
    m_cKi->setValue(0.5);

    QLabel* cKdLabel = new QLabel("cKd:", this);
    m_cKd = new QDoubleSpinBox(this);
    m_cKd->setRange(0.0, 50.0);
    m_cKd->setSingleStep(0.1);
    m_cKd->setValue(5.0);

    layout->addWidget(cKpLabel, row, 0);
    layout->addWidget(m_cKp, row++, 1);
    layout->addWidget(cKiLabel, row, 0);
    layout->addWidget(m_cKi, row++, 1);
    layout->addWidget(cKdLabel, row, 0);
    layout->addWidget(m_cKd, row++, 1);

    // Spacer
    QLabel* spacer2 = new QLabel("", this);
    layout->addWidget(spacer2, row++, 0, 1, -1);

    // Group title: Aftertreatment
    QLabel* afterTitle = new QLabel("<b>Aftertreatment (aAT, cAT)</b>", this);
    afterTitle->setStyleSheet("font-weight: bold; color: #dc3545;");
    layout->addWidget(afterTitle, row++, 0, 1, -1);

    // Aftertreatment parameters
    QLabel* aATLabel = new QLabel("aAT:", this);
    m_aAT = new QDoubleSpinBox(this);
    m_aAT->setRange(0.0, 5.0);
    m_aAT->setSingleStep(0.01);
    m_aAT->setValue(1.0);

    QLabel* cATLabel = new QLabel("cAT:", this);
    m_cAT = new QDoubleSpinBox(this);
    m_cAT->setRange(0.0, 5.0);
    m_cAT->setSingleStep(0.01);
    m_cAT->setValue(1.0);

    layout->addWidget(aATLabel, row, 0);
    layout->addWidget(m_aAT, row++, 1);
    layout->addWidget(cATLabel, row, 0);
    layout->addWidget(m_cAT, row++, 1);

    // Spacer
    QLabel* spacer3 = new QLabel("", this);
    layout->addWidget(spacer3, row++, 0, 1, -1);

    // Enable checkbox and Max boost
    m_enable = new QCheckBox("Enable N75 Control", this);
    m_enable->setChecked(true);

    QLabel* maxBoostLabel = new QLabel("Max Boost:", this);
    m_maxBoost = new QDoubleSpinBox(this);
    m_maxBoost->setRange(0.0, 3.0);
    m_maxBoost->setSingleStep(0.1);
    m_maxBoost->setValue(1.5);

    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(m_enable);
    bottomLayout->addStretch();
    bottomLayout->addWidget(maxBoostLabel);
    bottomLayout->addWidget(m_maxBoost);

    layout->addLayout(bottomLayout, row++, 0, 1, -1);

    // Connect signals
    connect(m_aKp, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_aKi, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_aKd, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_cKp, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_cKi, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_cKd, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_aAT, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_cAT, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
    connect(m_enable, &QCheckBox::stateChanged, this, &N75PidSettingsWidget::settingsChanged);
    connect(m_maxBoost, &QDoubleSpinBox::valueChanged, this, &N75PidSettingsWidget::onSpinBoxValueChanged);
}

void N75PidSettingsWidget::onSpinBoxValueChanged(double) {
    emit settingsChanged();
}