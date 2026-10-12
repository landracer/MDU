/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef BOOSTCONFIGDIALOG_H
#define BOOSTCONFIGDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QComboBox>

class N75TableWidget;
class N75PidSettingsWidget;

/**
 * @brief Dialog for configuring N75 boost control settings
 * 
 * Similar to the original Multidisplay app's V2N75SetupDialog,
 * this dialog allows users to configure:
 * - Normal/Race mode duty cycle selection
 * - Gear-specific maps (duty/setpoint) for 6 gears × 16 RPM bins
 * - PID parameters (aKp, aKi, aKd, cKp, cKi, cKd, aAT, cAT)
 * - Enable/max boost settings
 */
class BoostConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit BoostConfigDialog(QWidget* parent = nullptr);
    ~BoostConfigDialog() override;

    /**
     * @brief Get the current mode (0=Normal/Low, 1=Race/High)
     */
    int getCurrentMode() const;

    /**
     * @brief Set the current mode (0=Normal/Low, 1=Race/High)
     */
    void setCurrentMode(int mode);

    // N75 Table access methods
    void setDutyValue(int gear, int rpmBin, quint8 value);
    quint8 getDutyValue(int gear, int rpmBin) const;
    void setSetpointValue(int gear, int rpmBin, double value);
    double getSetpointValue(int gear, int rpmBin) const;

    // PID Settings access methods
    double getAKp() const;
    void setAKp(double value);
    double getAKi() const;
    void setAKi(double value);
    double getAKd() const;
    void setAKd(double value);

    double getCKp() const;
    void setCKp(double value);
    double getCKi() const;
    void setCKi(double value);
    double getCKd() const;
    void setCKd(double value);

    double getAAT() const;
    void setAAT(double value);
    double getCAT() const;
    void setCAT(double value);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    double getMaxBoost() const;
    void setMaxBoost(double value);

    // Duty Cycle access (backward compatibility)
    int getNormalDutyCycle() const { return m_normalDuty; }
    void setNormalDutyCycle(int value) { m_normalDuty = qBound(0, value, 255); }
    int getRaceDutyCycle() const { return m_raceDuty; }
    void setRaceDutyCycle(int value) { m_raceDuty = qBound(0, value, 255); }

private slots:
    void onModeChanged(int index);
    void onNormalDutyChanged(int value);
    void onRaceDutyChanged(int value);

private:
    void setupUI();

    QComboBox* m_modeCombo = nullptr;

    N75TableWidget* m_lowTable = nullptr;
    N75TableWidget* m_highTable = nullptr;
    N75PidSettingsWidget* m_pidSettings = nullptr;

    int m_normalDuty = 128;  // Default: 50% duty cycle
    int m_raceDuty = 255;    // Default: Full boost

    bool m_lowModeVisible = true;
};

#endif // BOOSTCONFIGDIALOG_H