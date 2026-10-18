/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef TESTHARNESS_H
#define TESTHARNESS_H

#include <QObject>
#include "UnifiedDataStore.h"
#include "DataPointRegistry.h"
#include "Utilities.h"

/**
 * @brief Test harness for core infrastructure
 * 
 * Runs automated tests on:
 * - UnifiedDataStore thread safety
 * - DataPointRegistry metadata
 * - Utility functions (endianness, checksums, etc.)
 * 
 * Usage: Run from command line with --test flag or call manually
 */
class TestHarness : public QObject {
    Q_OBJECT

public:
    explicit TestHarness(QObject* parent = nullptr);
    ~TestHarness() override = default;

    /**
     * @brief Run all tests
     * @return true if all tests passed
     */
    bool runAllTests();

private slots:
    // DataStore tests
    void testSingleton();
    void testDataPointSetGet();
    void testDataPointValidity();
    void testDataPointTimestamps();
    
    // Registry tests
    void testRegistryValidation();
    void testRegistryMetadata();
    void testRegistryConversion();
    void testCategoryIndexing();
    
    // Utility tests
    void testEndianness();
    void testChecksums();
    void testClampLerp();
    void testAngleConversions();
    void testUnitConverter();

    // MD-ECU frame-decoder tests (fixture + robustness)
    void testMdDecoderMainFrame();
    void testMdDecoderDiscardResync();
    void testMdDecoderFuzz();

    // PlotDataModel tests (ring buffer, decimation, XY pairing)
    void testPlotModelRing();
    void testPlotModelWindowSlice();
    void testPlotModelDecimate();
    void testPlotModelXyMode();

    // Dashboard routing tests (category auto-add, seeded series)
    void testDashboardRouting();

    // rAtTrax-BMS wire-format tests (BMS CSV, VESC kv + contract formats)
    void testRAtTraxBMSLines();

    // Session log tests (binary format, index, export)
    void testSessionRoundTrip();

private:
    int m_testsRun = 0;
    int m_testsPassed = 0;
    int m_testsFailed = 0;
    
    void report(const QString& test, bool passed, const QString& detail = QString());
};

#endif // TESTHARNESS_H
