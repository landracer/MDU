/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "DpPickerDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>

DpPickerDialog::DpPickerDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Select Data Point");
    resize(600, 500);
    
    setupUI();
    populateCategories();
}

void DpPickerDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);
    
    // Search bar at top
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Search:"));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Type to filter data points...");
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);
    
    // Tree widget for category-grouped data points
    m_treeWidget = new QTreeWidget();
    m_treeWidget->setHeaderLabels({"Data Point", "ID", "Unit"});
    m_treeWidget->setColumnWidth(0, 250);
    m_treeWidget->setColumnWidth(1, 80);
    m_treeWidget->setColumnWidth(2, 80);
    m_treeWidget->setExpandsOnDoubleClick(false);
    m_treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_treeWidget);
    
    // Paging controls at bottom
    auto* pageLayout = new QHBoxLayout();
    auto* prevButton = new QPushButton("◄ Prev");
    m_pageLabel = new QLabel("Page 1/1");
    auto* nextButton = new QPushButton("Next ►");
    pageLayout->addWidget(prevButton);
    pageLayout->addWidget(m_pageLabel);
    pageLayout->addStretch();
    mainLayout->addLayout(pageLayout);
    
    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &DpPickerDialog::onSearchTextChanged);
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &DpPickerDialog::onItemClicked);
    connect(prevButton, &QPushButton::clicked,
            this, &DpPickerDialog::onPrevPage);
    connect(nextButton, &QPushButton::clicked,
            this, &DpPickerDialog::onNextPage);
}

void DpPickerDialog::populateCategories() {
    auto& registry = DataPointRegistry::instance();
    
    // Get all registered data point IDs
    QVector<DataPointID> allPoints = registry.allPoints();
    
    // Group by category string
    QHash<QString, QVector<std::pair<DataPointID, DataPointRegistry::PointInfo>>> categorized;
    for (auto id : allPoints) {
        if (!registry.isValid(id)) continue;
        auto info = registry.getInfo(id);
        categorized[info.category].append({id, info});
    }
    
    // Sort each category by DP ID
    for (auto& points : categorized) {
        std::sort(points.begin(), points.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    
    // Create tree items for each category
    for (auto it = categorized.begin(); it != categorized.end(); ++it) {
        const QString& catName = it.key();
        auto& points = it.value();
        
        // Create category header item
        auto* catItem = new QTreeWidgetItem(m_treeWidget);
        catItem->setText(0, catName);
        catItem->setText(2, QString("%1 items").arg(points.size()));
        catItem->setFlags(catItem->flags() | Qt::ItemIsUserTristate | Qt::ItemIsUserCheckable);
        catItem->setCheckState(0, Qt::Checked); // All visible by default
        catItem->setFont(0, QFont()); // Bold font for category
        
        // Store points in category item for paging
        catItem->setData(0, Qt::UserRole, QVariant::fromValue(points));
    }
    
    applyFilter();
}

void DpPickerDialog::applyFilter() {
    QString searchText = m_searchEdit->text().toLower();
    m_currentPage = 0;
    
    // Clear existing items (but keep category headers)
    for (int c = 0; c < m_treeWidget->topLevelItemCount(); c++) {
        auto* catItem = m_treeWidget->topLevelItem(c);
        
        // Remove child items
        while (catItem->childCount() > 0) {
            delete catItem->takeChild(0);
        }
        
        auto points = catItem->data(0, Qt::UserRole).value<QVector<std::pair<DataPointID, DataPointRegistry::PointInfo>>>();
        
        // Filter and show first page
        int visibleCount = 0;
        for (const auto& [id, info] : points) {
            QString hexId = QString("0x%1").arg(static_cast<uint32_t>(id), 4, 16, QChar('0')).toLower();
            
            bool matchesSearch = searchText.isEmpty() || 
                                info.name.toLower().contains(searchText) ||
                                hexId.contains(searchText) ||
                                info.unit.toLower().contains(searchText);
            
            if (matchesSearch && visibleCount < ITEMS_PER_PAGE) {
                auto* childItem = new QTreeWidgetItem(catItem);
                childItem->setText(0, info.name);
                childItem->setText(1, hexId);
                childItem->setText(2, info.unit);
                childItem->setData(0, Qt::UserRole, QVariant::fromValue(id));
                visibleCount++;
            }
        }
        
        // Update category header with visible count
        catItem->setText(2, QString("%1/%2 items").arg(visibleCount).arg(points.size()));
    }
    
    updatePageLabel();
}

void DpPickerDialog::onSearchTextChanged(const QString& text) {
    applyFilter();
}

void DpPickerDialog::onItemClicked(QTreeWidgetItem* item, int column) {
    // If clicking category header, toggle visibility (don't select)
    if (item->childCount() > 0 || m_treeWidget->topLevelItemCount() > 0 && 
        m_treeWidget->topLevelItem(0) == item) {
        return;
    }
    
    // Get selected data point ID
    DataPointID id = item->data(0, Qt::UserRole).value<DataPointID>();
    if (id != DataPointID::Invalid) {
        m_selectedId = id;
        accept();
    }
}

void DpPickerDialog::onNextPage() {
    int totalPages = getTotalPages();
    if (m_currentPage < totalPages - 1) {
        m_currentPage++;
        applyFilter();
    }
}

void DpPickerDialog::onPrevPage() {
    if (m_currentPage > 0) {
        m_currentPage--;
        applyFilter();
    }
}

void DpPickerDialog::setSelectedDataPoint(DataPointID id) {
    m_selectedId = id;
    
    // Highlight the matching item in the tree
    for (int c = 0; c < m_treeWidget->topLevelItemCount(); c++) {
        auto* catItem = m_treeWidget->topLevelItem(c);
        for (int i = 0; i < catItem->childCount(); i++) {
            auto childId = catItem->child(i)->data(0, Qt::UserRole).value<DataPointID>();
            if (childId == id) {
                m_treeWidget->setCurrentItem(catItem->child(i));
                m_treeWidget->scrollToItem(catItem->child(i));
                return;
            }
        }
    }
}

int DpPickerDialog::getTotalPages() const {
    int totalVisible = 0;
    for (int c = 0; c < m_treeWidget->topLevelItemCount(); c++) {
        auto* catItem = m_treeWidget->topLevelItem(c);
        totalVisible += catItem->childCount();
    }
    return qMax(1, (totalVisible + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
}

void DpPickerDialog::updatePageLabel() {
    int totalPages = getTotalPages();
    m_pageLabel->setText(QString("Page %1/%2").arg(m_currentPage + 1).arg(totalPages));
}
