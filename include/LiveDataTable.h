/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef LiveDataTable_H
#define LiveDataTable_H

#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include "UnifiedDataStore.h"
#include "DataPointRegistry.h"
#include "Settings.h"

// Forward declare SettingsDialog to avoid circular dependency
class SettingsDialog;

/**
 * @brief Real-time data table showing current values from UnifiedDataStore
 * 
 * Displays all valid data points in a scrollable table with columns:
 * - Name, Value, Unit, Source, Last Update
 * Auto-refreshes at ~5 Hz to show latest values without overwhelming the UI.
 */
class LiveDataTable : public QWidget {
    Q_OBJECT

public:
    explicit LiveDataTable(QWidget* parent = nullptr);
    ~LiveDataTable() override = default;

    /**
     * @brief Set refresh interval in milliseconds (default 200ms = 5 Hz)
     */
    void setRefreshInterval(int ms);

    /**
     * @brief Toggle visibility of a specific data point row
     */
    Q_INVOKABLE void setDataPointVisible(quint16 dpId, bool visible);

signals:
    void cellClicked(DataPointID id, double value);
    
private slots:
    void onRefreshTick();
    void onDataPointUpdated(DataPointID id, double value, const QString& source);
    void onCellClicked(int row, int column);
    void onUnitsChanged(const QString& canonicalUnit);

private:
    void setupUI();
    void populateTable();
    void updateRow(int row, DataPointID id);
    void setTableRowVisible(int row, bool visible);

    QTableWidget* m_table;
    QTimer* m_timer;
    
    // Row index for each data point ID
    QHash<DataPointID, int> m_idToRow;
    
    // Visibility state for each row (tracked separately from table visibility)
    QHash<int, bool> m_rowVisibility;
};

#endif // LiveDataTable_H
