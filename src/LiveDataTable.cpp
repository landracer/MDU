/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "LiveDataTable.h"
#include "UnitConverter.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>
#include <QSettings>

LiveDataTable::LiveDataTable(QWidget* parent)
    : QWidget(parent)
    , m_table(nullptr)
    , m_timer(nullptr)
{
    setRefreshInterval(200); // 5 Hz refresh
    setupUI();
    populateTable();
    
    // Connect to UnifiedDataStore signals
    auto& store = UnifiedDataStore::instance();
    connect(&store, &UnifiedDataStore::dataPointUpdated,
            this, &LiveDataTable::onDataPointUpdated);
    
    // Click the Unit column to cycle through convertible units (°C→°F, kPa→PSI→bar, ...)
    connect(m_table, &QTableWidget::cellClicked, this, &LiveDataTable::onCellClicked);
    connect(&UnitConverter::instance(), &UnitConverter::unitsChanged,
            this, &LiveDataTable::onUnitsChanged);
    
    // Start refresh timer
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LiveDataTable::onRefreshTick);
    m_timer->start(200);
}

void LiveDataTable::setRefreshInterval(int ms) {
    if (m_timer) {
        m_timer->setInterval(ms);
    }
}

void LiveDataTable::setDataPointVisible(quint16 dpId, bool visible) {
    // Cast quint16 to DataPointID (the enum class uses uint32_t as underlying type)
    DataPointID id = static_cast<DataPointID>(dpId);
    int row = m_idToRow.value(id, -1);
    if (row >= 0) {
        setTableRowVisible(row, visible);
        m_rowVisibility[row] = visible;
        qDebug() << "[LiveDataTable] Data point 0x" 
                 << QString::number(dpId, 16).toUpper()
                 << " visibility changed to:" << visible;
    }
}

void LiveDataTable::setTableRowVisible(int row, bool visible) {
    if (m_table && row >= 0 && row < m_table->rowCount()) {
        m_table->setRowHidden(row, !visible);
        // Also store the row height for non-hidden rows
        if (!visible) {
            m_table->setRowHeight(row, 0);  // Minimize hidden rows
        } else {
            m_table->setRowHeight(row, 24); // Restore default height
        }
    }
}

void LiveDataTable::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Name", "Value", "Unit", "Source", "Last Update"});
    
    // Configure table appearance - use Stretch for all, let content determine widths
    QHeaderView* header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);  // Name gets remaining space
    
    // Fixed widths via setMinimumSectionSize + stretch factors
    header->setSectionResizeMode(1, QHeaderView::Fixed);    // Value - will resize to content
    header->setSectionResizeMode(2, QHeaderView::Fixed);    // Unit
    header->setSectionResizeMode(3, QHeaderView::Fixed);    // Source
    header->setSectionResizeMode(4, QHeaderView::Fixed);    // Last Update
    
    m_table->horizontalHeader()->setMinimumSectionSize(60);
    
    // Dark theme styling
    m_table->setStyleSheet(
        "QTableWidget { "
        "background-color: #2d2d30; "
        "color: #cccccc; "
        "gridline-color: #3e3e42; "
        "border: 1px solid #3e3e42; "
        "}"
        "QTableWidget::item { "
        "padding: 4px; "
        "}"
        "QHeaderView::section { "
        "background-color: #1e1e1e; "
        "color: #ffffff; "
        "border: 1px solid #3e3e42; "
        "padding: 5px; "
        "font-weight: bold; "
        "}"
    );
    
    layout->addWidget(m_table);
}

void LiveDataTable::populateTable() {
    auto& registry = DataPointRegistry::instance();
    QVector<DataPointID> allPoints = registry.allPointsSorted();
    
    m_table->setRowCount(allPoints.size());
    
    for (int i = 0; i < allPoints.size(); i++) {
        DataPointID id = allPoints[i];
        auto info = registry.getInfo(id);
        
        QTableWidgetItem* nameItem = new QTableWidgetItem(info.name);  // info.name is now QString
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 0, nameItem);
        
        // Value column (placeholder)
        QTableWidgetItem* valueItem = new QTableWidgetItem("--");
        valueItem->setTextAlignment(Qt::AlignCenter);
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 1, valueItem);
        
        // Unit column — shows the selected display unit; click to cycle the family
        auto& conv = UnitConverter::instance();
        QTableWidgetItem* unitItem = new QTableWidgetItem(conv.displayUnit(info.unit));
        unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);
        unitItem->setData(Qt::UserRole, info.unit); // remember the canonical unit
        if (conv.isConvertible(info.unit)) {
            unitItem->setToolTip(QStringLiteral("Click to switch: %1")
                .arg(conv.familyUnits(info.unit).join(QStringLiteral(" → "))));
            unitItem->setForeground(QBrush(QColor("#4ec9b0"))); // hint it's interactive
        }
        m_table->setItem(i, 2, unitItem);
        
        // Source column (placeholder)
        QTableWidgetItem* sourceItem = new QTableWidgetItem("--");
        sourceItem->setTextAlignment(Qt::AlignCenter);
        sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 3, sourceItem);
        
        // Last Update column (placeholder)
        QTableWidgetItem* timeItem = new QTableWidgetItem("--");
        timeItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, 4, timeItem);
        
        // Store row index for this data point
        m_idToRow[id] = i;
        
        // Set initial visibility from settings
        quint16 dpIdU16 = static_cast<quint16>(id);
        bool visible = Settings::instance().isDataPointVisible(dpIdU16);
        qDebug() << "[LiveDataTable] Row" << i << "ID=0x" 
                 << QString("%1").arg(static_cast<uint32_t>(id), 4, 16, QChar('0'))
                 << "visibility:" << visible;
        setTableRowVisible(i, visible);
        m_rowVisibility[i] = visible;
    }
    
    qDebug() << "[LiveDataTable] Populated table with" << allPoints.size() << "data points";
}

void LiveDataTable::onRefreshTick() {
    auto& store = UnifiedDataStore::instance();
    QVector<DataPointID> validPoints = store.getValidPoints();
    
    qDebug() << "[LiveDataTable] Refresh tick - found" << validPoints.size() << "valid points";
    
    for (DataPointID id : validPoints) {
        int row = m_idToRow.value(id, -1);
        uint32_t val = static_cast<uint32_t>(id);
        
        
        if (row >= 0 && !m_table->isRowHidden(row)) {
            updateRow(row, id);
        }
    }
}

void LiveDataTable::onDataPointUpdated(DataPointID id, double value, const QString& source) {
    uint32_t val = static_cast<uint32_t>(id);
    
    
    int row = m_idToRow.value(id, -1);
    if (row >= 0 && !m_table->isRowHidden(row)) {
        updateRow(row, id);
    }
}

void LiveDataTable::onCellClicked(int row, int column) {
    if (column != 2) return; // only the Unit column cycles
    
    QTableWidgetItem* unitItem = m_table->item(row, 2);
    if (!unitItem) return;
    
    const QString canonical = unitItem->data(Qt::UserRole).toString();
    auto& conv = UnitConverter::instance();
    if (!conv.isConvertible(canonical)) return;
    
    const QString newUnit = conv.cycleUnit(canonical);
    qDebug() << "[LiveDataTable] Unit family" << canonical << "switched to" << newUnit;
    // onUnitsChanged refreshes every affected row
}

void LiveDataTable::onUnitsChanged(const QString& canonicalUnit) {
    auto& conv = UnitConverter::instance();
    const QString newUnit = conv.displayUnit(canonicalUnit);
    
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem* unitItem = m_table->item(row, 2);
        if (!unitItem || unitItem->data(Qt::UserRole).toString() != canonicalUnit) continue;
        
        unitItem->setText(newUnit);
        
        // Re-render the value in the new unit immediately
        for (auto it = m_idToRow.constBegin(); it != m_idToRow.constEnd(); ++it) {
            if (it.value() == row) {
                if (!m_table->isRowHidden(row)) updateRow(row, it.key());
                break;
            }
        }
    }
}

void LiveDataTable::updateRow(int row, DataPointID id) {
    auto& store = UnifiedDataStore::instance();
    auto& registry = DataPointRegistry::instance();
    auto& conv = UnitConverter::instance();
    
    bool valid = false;
    double value = store.getDataPoint(id, &valid);
    QString source = store.getSource(id);
    QDateTime timestamp = store.getTimestamp(id);
    
    
    if (!valid) return;
    
    // Convert to the user's selected display unit (Unit column shows which)
    auto info = registry.getInfo(id);
    value = conv.toDisplay(info.unit, value);
    
    // Value column shows the number only; the Unit column carries the unit
    QString formattedValue;
    if (id == DataPointID::RPM || id == DataPointID::MDRPM) {
        formattedValue = QString::number(static_cast<qint64>(value));
    } else if (qAbs(value) >= 1000.0) {
        formattedValue = QString("%1").arg(value, 0, 'f', 0);
    } else if (qAbs(value) >= 1.0) {
        formattedValue = QString("%1").arg(value, 0, 'f', 2);
    } else {
        formattedValue = QString("%1").arg(value, 0, 'f', 4);
    }
    
    QTableWidgetItem* valueItem = m_table->item(row, 1);
    if (valueItem) {
        valueItem->setText(formattedValue);
        
        // Color coding based on value ranges
        QColor textColor = QColor("#cccccc"); // Default gray
        uint32_t val = static_cast<uint32_t>(id);
        
        // Red for critical values
        if (val >= 0x0400 && val <= 0x04FF) { // BMS range
            if (value < 3.0 || value > 4.2) textColor = QColor("#ff4444");
            else if (value < 3.2 || value > 4.1) textColor = QColor("#ffaa00");
        }
        
        // Green for normal ranges
        if (val >= 0x0100 && val <= 0x01FF) { // Engine range
            if (id == DataPointID::RPM && value > 8000) textColor = QColor("#ff4444");
            else if (id == DataPointID::BoostPressure && value > 200) textColor = QColor("#ff4444");
        }
        
        valueItem->setForeground(QBrush(textColor));
    }
    
    // Update source column
    QTableWidgetItem* sourceItem = m_table->item(row, 3);
    if (sourceItem) {
        sourceItem->setText(source);
    }
    
    // Update timestamp column
    QTableWidgetItem* timeItem = m_table->item(row, 4);
    if (timeItem) {
        timeItem->setText(timestamp.toString("hh:mm:ss.zzz"));
    }
}