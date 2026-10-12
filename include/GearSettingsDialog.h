/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef GEARSETTINGSDIALOG_H
#define GEARSETTINGSDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>

/**
 * @brief Dialog for configuring gear ratio settings
 * 
 * Similar to the original Multidisplay app's GearSettingsDialog,
 * this dialog allows users to configure:
 * - 6 gear ratios with 4 decimal precision
 * - Predefined transmission profiles
 */
class GearSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GearSettingsDialog(QWidget* parent = nullptr);
    ~GearSettingsDialog() override;

    // Get/set gear ratios (transmission ratio = RPM / wheelSpeed)
    double getGearRatio(int gear) const;  // gear: 0-5 for gears 1-6
    void setGearRatio(int gear, double value);

    // Get all gear ratios at once
    QVector<double> getGearRatios() const;

signals:
    void gearRatiosChanged(const QVector<double>& ratios);
    void writeGearboxData(double g1, double g2, double g3, 
                          double g4, double g5, double g6, quint8 serial);

private slots:
    void onAccepted();
    void loadDefaultTransmission(int index);

private:
    void setupUI();

    QDoubleSpinBox* m_gearSpinboxes[6] = {nullptr};

    // Predefined transmission profiles
    struct TransmissionProfile {
        QString name;
        double ratios[6];
    };
    
    static const TransmissionProfile s_profiles[];
};

#endif // GEARSETTINGSDIALOG_H