/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef N75TABLEWIDGET_H
#define N75TABLEWIDGET_H

#include <QTableWidget>

/**
 * @brief Widget displaying N75 gear map table (duty/setpoint rows × 16 RPM bins)
 * 
 * The table has 12 rows:
 * - Rows 0, 2, 4, 6, 8, 10: Duty cycle values for gears 1-6 (0-255)
 * - Rows 1, 3, 5, 7, 9, 11: Setpoint values for gears 1-6 (-0.5 to 2.5 bar)
 * 
 * Columns represent RPM bins:
 * - Column 0: 0 RPM
 * - Columns 1-14: 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000, 12000, 13000, 14000, 15000 RPM
 * - Column 15: 16000+ RPM
 */
class N75TableWidget : public QTableWidget {
    Q_OBJECT

public:
    /**
     * @brief Construct a new N75 Table Widget
     * 
     * @param gearIndex Gear index (0-5) for low or high mode selection
     * @param isHighMode true if this table shows high boost mode, false for low
     * @param parent Parent widget
     */
    N75TableWidget(int gearIndex = 0, bool isHighMode = false, QWidget* parent = nullptr);

    /**
     * @brief Set the duty cycle value for a specific gear and RPM bin
     * @param gear Gear index (0-5)
     * @param rpmBin RPM bin index (0-15)
     * @param value Duty cycle value (0-255)
     */
    void setDutyValue(int gear, int rpmBin, quint8 value);

    /**
     * @brief Get the duty cycle value for a specific gear and RPM bin
     * @return Duty cycle value (0-255), or 0 if out of range
     */
    quint8 getDutyValue(int gear, int rpmBin) const;

    /**
     * @brief Set the setpoint value for a specific gear and RPM bin
     * @param gear Gear index (0-5)
     * @param rpmBin RPM bin index (0-15)
     * @param value Setpoint in bar (-0.5 to 2.5)
     */
    void setSetpointValue(int gear, int rpmBin, double value);

    /**
     * @brief Get the setpoint value for a specific gear and RPM bin
     * @return Setpoint in bar (-0.5 to 2.5), or 0 if out of range
     */
    double getSetpointValue(int gear, int rpmBin) const;

    /**
     * @brief Fill table with duty cycle data from vector
     */
    void fillDutyData(const QVector<quint8>& data);

    /**
     * @brief Fill table with setpoint data from vector
     */
    void fillSetpointData(const QVector<double>& data);

private:
    void setupUI();

    int m_gearIndex;
    bool m_isHighMode;
};

#endif // N75TABLEWIDGET_H