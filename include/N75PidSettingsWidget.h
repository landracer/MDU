/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef N75PIDSETTINGSWIDGET_H
#define N75PIDSETTINGSWIDGET_H

#include <QWidget>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGridLayout>

/**
 * @brief Widget for configuring N75 PID parameters and settings
 * 
 * Displays:
 * - aKp, aKi, aKd: Acceleration PID parameters
 * - cKp, cKi, cKd: Control/Compression PID parameters  
 * - aAT, cAT: Aftertreatment / Boost control parameters
 * - Enable checkbox for N75 control
 * - Max boost setting (in bar)
 */
class N75PidSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit N75PidSettingsWidget(QWidget* parent = nullptr);

    // Acceleration PID parameters
    double getAKp() const { return m_aKp->value(); }
    void setAKp(double value) { m_aKp->setValue(value); }

    double getAKi() const { return m_aKi->value(); }
    void setAKi(double value) { m_aKi->setValue(value); }

    double getAKd() const { return m_aKd->value(); }
    void setAKd(double value) { m_aKd->setValue(value); }

    // Control PID parameters
    double getCKp() const { return m_cKp->value(); }
    void setCKp(double value) { m_cKp->setValue(value); }

    double getCKi() const { return m_cKi->value(); }
    void setCKi(double value) { m_cKi->setValue(value); }

    double getCKd() const { return m_cKd->value(); }
    void setCKd(double value) { m_cKd->setValue(value); }

    // Aftertreatment parameters
    double getAAT() const { return m_aAT->value(); }
    void setAAT(double value) { m_aAT->setValue(value); }

    double getCAT() const { return m_cAT->value(); }
    void setCAT(double value) { m_cAT->setValue(value); }

    // Enable and max boost
    bool isEnabled() const { return m_enable->isChecked(); }
    void setEnabled(bool enabled) { m_enable->setChecked(enabled); }

    double getMaxBoost() const { return m_maxBoost->value(); }
    void setMaxBoost(double value) { m_maxBoost->setValue(value); }

signals:
    void settingsChanged();

private slots:
    void onSpinBoxValueChanged(double);

private:
    void setupUI();

    QDoubleSpinBox* m_aKp = nullptr;
    QDoubleSpinBox* m_aKi = nullptr;
    QDoubleSpinBox* m_aKd = nullptr;

    QDoubleSpinBox* m_cKp = nullptr;
    QDoubleSpinBox* m_cKi = nullptr;
    QDoubleSpinBox* m_cKd = nullptr;

    QDoubleSpinBox* m_aAT = nullptr;
    QDoubleSpinBox* m_cAT = nullptr;

    QCheckBox* m_enable = nullptr;
    QDoubleSpinBox* m_maxBoost = nullptr;
};

#endif // N75PIDSETTINGSWIDGET_H