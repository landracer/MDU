/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef LAYOUT_EDITOR_H
#define LAYOUT_EDITOR_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "DpPickerDialog.h"

/**
 * @brief OpenDash per-node layout editor dialog
 * 
 * Implements the layout configuration system from mergecombo.md:
 * - Node selector (CENTER, LEFT, RIGHT, GPS, POD1-8, etc.)
 * - Display mode selector (ENGINE, GPS, MD, OBD, BMS, RELAY, CONFIG)
 * - Arc gauge configuration with min/max range
 * - Slot-based data point assignment (up to 16 slots per mode)
 * - Push layouts to nodes via ESP-NOW bridge
 * - Save/load from Settings persistence
 */
class LayoutEditor : public QDialog {
    Q_OBJECT

public:
    explicit LayoutEditor(QWidget* parent = nullptr);
    ~LayoutEditor() override = default;

private slots:
    void onNodeChanged(int index);
    void onModeChanged(int index);
    void onArcDpClicked();
    void onSlotDpClicked(int slotIndex);
    void onResetLayout();
    void onPushToNode();
    void onSaveLayout();
    void onLoadLayout();

private:
    void setupUI();
    void populateNodes();
    void populateModes();
    void buildSlotWidgets();
    QString getCommandString() const;
    void showStatusMessage(const QString& msg, int timeout = 0);
    
    // Node configuration
    struct SlotConfig {
        DataPointID dpId = DataPointID::Invalid;
        bool hasValue = false;
    };
    
    void loadSavedLayout();
    
    // Guard to prevent callbacks during construction
    bool m_constructing = true;

    // UI components
    QComboBox* m_nodeCombo;
    QComboBox* m_modeCombo;
    
    // Arc gauge configuration
    QWidget* m_arcConfigWidget;
    QLabel* m_arcDpLabel;
    QPushButton* m_arcDpButton;
    QSpinBox* m_arcMinSpin;
    QSpinBox* m_arcMaxSpin;
    
    // Slot configurations (up to 16 slots)
    static constexpr int MAX_SLOTS = 16;
    QWidget* m_slotsContainer;
    QVector<QPushButton*> m_slotButtons;
    QVector<QLabel*> m_slotLabels;
    
    // Current layout state
    QString m_currentNode;
    int m_currentMode = 0;
    QVector<SlotConfig> m_slots;
    
    // DP picker dialog (reused)
    DpPickerDialog* m_dpPicker;
    
    // Status label
    QLabel* m_statusLabel;
};

#endif // LAYOUT_EDITOR_H
