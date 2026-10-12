/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "N75TableWidget.h"
#include <QHeaderView>
#include <QDoubleValidator>
#include <QSpinBox>

// RPM bin labels (0-15)
static const char* RPM_LABELS[] = {
    "0", "2k", "3k", "4k", "5k", "6k", "7k", "8k",
    "9k", "10k", "11k", "12k", "13k", "14k", "15k", "16k+"
};

// For compatibility with the old API where a single table shows all gears
// gearIndex and isHighMode are kept for API but now the widget always shows all 12 rows
N75TableWidget::N75TableWidget(int gearIndex, bool isHighMode, QWidget* parent)
    : QTableWidget(parent), m_gearIndex(gearIndex), m_isHighMode(isHighMode) {
    Q_UNUSED(gearIndex);
    Q_UNUSED(isHighMode);
    setupUI();
}

void N75TableWidget::setupUI() {
    // 12 rows (6 gears × 2 for duty/setpoint), 16 columns (RPM bins)
    setRowCount(12);
    setColumnCount(16);

    // Set column headers (RPM bins)
    QStringList columnLabels;
    for (int i = 0; i < 16; ++i) {
        columnLabels << RPM_LABELS[i];
    }
    setHorizontalHeaderLabels(columnLabels);

    // Set row headers (Gear + Duty/Setpoint labels)
    QStringList rowLabels;
    const char* GEAR_NAMES[] = {"G1 D", "G1 S", "G2 D", "G2 S",
                                "G3 D", "G3 S", "G4 D", "G4 S",
                                "G5 D", "G5 S", "G6 D", "G6 S"};
    for (int i = 0; i < 12; ++i) {
        rowLabels << GEAR_NAMES[i];
    }
    setVerticalHeaderLabels(rowLabels);

    // Configure cell widgets
    for (int row = 0; row < 12; ++row) {
        for (int col = 0; col < 16; ++col) {
            QWidget* editor;
            if (row % 2 == 0) {
                // Duty cycle rows (0, 2, 4, ...): 0-255
                QSpinBox* spinBox = new QSpinBox(this);
                spinBox->setRange(0, 255);
                spinBox->setValue(128); // Default: 50%
                editor = spinBox;
            } else {
                // Setpoint rows (1, 3, 5, ...): -0.5 to 2.5 bar
                QDoubleSpinBox* doubleSpin = new QDoubleSpinBox(this);
                doubleSpin->setRange(-0.5, 2.5);
                doubleSpin->setSingleStep(0.01);
                doubleSpin->setValue(1.0); // Default: 1.0 bar
                editor = doubleSpin;
            }
            setCellWidget(row, col, editor);
        }
    }

    // Make column headers fixed size
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // Auto-fill with default values
    for (int gear = 0; gear < 6; ++gear) {
        for (int bin = 0; bin < 16; ++bin) {
            setDutyValue(gear, bin, 128);     // Default duty: 50%
            setSetpointValue(gear, bin, 1.0); // Default setpoint: 1.0 bar
        }
    }

    // Make vertical header wider to show labels
    verticalHeader()->setDefaultSectionSize(25);
}

void N75TableWidget::setDutyValue(int gear, int rpmBin, quint8 value) {
    if (gear < 0 || gear >= 6 || rpmBin < 0 || rpmBin >= 16) return;

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(cellWidget(gear * 2, rpmBin));
    if (spinBox) {
        spinBox->setValue(value);
    }
}

quint8 N75TableWidget::getDutyValue(int gear, int rpmBin) const {
    if (gear < 0 || gear >= 6 || rpmBin < 0 || rpmBin >= 16) return 0;

    QSpinBox* spinBox = qobject_cast<QSpinBox*>(cellWidget(gear * 2, rpmBin));
    return spinBox ? static_cast<quint8>(spinBox->value()) : 0;
}

void N75TableWidget::setSetpointValue(int gear, int rpmBin, double value) {
    if (gear < 0 || gear >= 6 || rpmBin < 0 || rpmBin >= 16) return;

    QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(cellWidget(gear * 2 + 1, rpmBin));
    if (spinBox) {
        spinBox->setValue(value);
    }
}

double N75TableWidget::getSetpointValue(int gear, int rpmBin) const {
    if (gear < 0 || gear >= 6 || rpmBin < 0 || rpmBin >= 16) return 0.0;

    QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(cellWidget(gear * 2 + 1, rpmBin));
    return spinBox ? spinBox->value() : 0.0;
}

void N75TableWidget::fillDutyData(const QVector<quint8>& data) {
    if (data.size() != 16) return;

    int row = m_gearIndex * 2; // Duty is always in even rows
    for (int i = 0; i < 16; ++i) {
        setDutyValue(m_gearIndex, i, data.at(i));
    }
}

void N75TableWidget::fillSetpointData(const QVector<double>& data) {
    if (data.size() != 16) return;

    int row = m_gearIndex * 2 + 1; // Setpoint is always in odd rows
    for (int i = 0; i < 16; ++i) {
        setSetpointValue(m_gearIndex, i, data.at(i));
    }
}