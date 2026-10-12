/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef DP_PICKER_DIALOG_H
#define DP_PICKER_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLabel>
#include "DataPointRegistry.h"

/**
 * @brief Data point picker dialog with category grouping and search
 * 
 * Implements the lv_list-style modal picker from OpenDash:
 * - Category-grouped data points (ENGINE/ECU, BMS, VESC/DRIVETRAIN, etc.)
 * - Real-time text search filter
 * - Paging (20 items per page for desktop)
 * - Click-to-select with close-on-selection
 */
class DpPickerDialog : public QDialog {
    Q_OBJECT

public:
    explicit DpPickerDialog(QWidget* parent = nullptr);
    ~DpPickerDialog() override = default;

    /**
     * @brief Set the currently selected data point (highlighted)
     */
    void setSelectedDataPoint(DataPointID id);

    /**
     * @brief Get the selected data point after dialog closes
     */
    DataPointID getSelectedDataPoint() const { return m_selectedId; }

private slots:
    void onSearchTextChanged(const QString& text);
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onNextPage();
    void onPrevPage();

private:
    void setupUI();
    void populateCategories();
    void applyFilter();
    void updatePageLabel();
    int getTotalPages() const;

    // UI components
    QLineEdit* m_searchEdit;
    QTreeWidget* m_treeWidget;
    QLabel* m_pageLabel;
    
    // State
    DataPointID m_selectedId = DataPointID::Invalid;
    int m_currentPage = 0;
    static constexpr int ITEMS_PER_PAGE = 20;
};

#endif // DP_PICKER_DIALOG_H
