/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "TestHarness.h"
#include "UnitConverter.h"
#include "MdFrameDecoder.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QTimer>
#include <QThread>
// QtConcurrent removed - not available in minimal Qt6 builds

TestHarness::TestHarness(QObject* parent)
    : QObject(parent)
{
}

void TestHarness::testSingleton() {
    // Test that UnifiedDataStore is a true singleton
    auto& store1 = UnifiedDataStore::instance();
    auto& store2 = UnifiedDataStore::instance();
    Q_ASSERT(&store1 == &store2);
    report("Singleton", true);
}

bool TestHarness::runAllTests() {
    qDebug() << "========================================";
    qDebug() << "  MultiDisplay Unified — Test Suite";
    qDebug() << "========================================";
    
    // DataStore tests
    testSingleton();
    testDataPointSetGet();
    testDataPointValidity();
    testDataPointTimestamps();
    
    // Registry tests
    testRegistryValidation();
    testRegistryMetadata();
    testRegistryConversion();
    testCategoryIndexing();
    
    // Utility tests
    testEndianness();
    testChecksums();
    testClampLerp();
    testAngleConversions();
    testUnitConverter();

    // MD-ECU frame-decoder tests
    testMdDecoderMainFrame();
    testMdDecoderDiscardResync();
    testMdDecoderFuzz();
    
    qDebug() << "========================================";
    qDebug() << "  Results: " << m_testsPassed << "/" << m_testsRun << " passed";
    if (m_testsFailed > 0) {
        qDebug() << "  FAILED: " << m_testsFailed << " test(s)";
    } else {
        qDebug() << "  ALL TESTS PASSED";
    }
    qDebug() << "========================================";
    
    return m_testsFailed == 0;
}

void TestHarness::testDataPointSetGet() {
    auto& store = UnifiedDataStore::instance();
    store.resetValidity();
    
    store.setDataPoint(DataPointID::RPM, 3200.0, "TEST");
    store.setDataPoint(DataPointID::BoostPressure, 145.5, "TEST");
    store.setDataPoint(DataPointID::BMSSOC, 75.0, "TEST");
    
    bool valid = false;
    double rpm = store.getDataPoint(DataPointID::RPM, &valid);
    
    report("testDataPointSetGet", 
           qFuzzyCompare(rpm, 3200.0) && valid,
           QString("RPM: %1 (expected 3200)").arg(rpm));
    
    double boost = store.getDataPoint(DataPointID::BoostPressure, &valid);
    report("testDataPointSetGet_boost",
           qFuzzyCompare(boost, 145.5) && valid,
           QString("Boost: %1 (expected 145.5)").arg(boost));
    
    double soc = store.getDataPoint(DataPointID::BMSSOC, &valid);
    report("testDataPointSetGet_soc",
           qFuzzyCompare(soc, 75.0) && valid,
           QString("SOC: %1 (expected 75)").arg(soc));
    
    // Test unset point
    double unset = store.getDataPoint(DataPointID::EGT1, &valid);
    report("testDataPointSetGet_unset",
           !valid && qFuzzyCompare(unset, 0.0),
           "Unset point should return 0.0 with valid=false");
}

void TestHarness::testDataPointValidity() {
    auto& store = UnifiedDataStore::instance();
    store.resetValidity();
    
    report("testDataPointValidity_initial",
           !store.isValid(DataPointID::RPM),
           "Initial state should be invalid");
    
    store.setDataPoint(DataPointID::RPM, 1000.0, "TEST");
    report("testDataPointValidity_afterSet",
           store.isValid(DataPointID::RPM),
           "Should be valid after setting");
}

void TestHarness::testDataPointTimestamps() {
    auto& store = UnifiedDataStore::instance();
    store.resetValidity();
    
    QDateTime before = QDateTime::currentDateTime();
    store.setDataPoint(DataPointID::RPM, 2000.0, "TEST");
    QDateTime after = QDateTime::currentDateTime();
    
    QDateTime ts = store.getTimestamp(DataPointID::RPM);
    bool ok = !ts.isNull() && ts >= before && ts <= after;
    
    report("testDataPointTimestamps",
           ok,
           QString("Timestamp: %1 (between %2 and %3)")
               .arg(ts.toString())
               .arg(before.toString())
               .arg(after.toString()));
}

void TestHarness::testRegistryValidation() {
    auto& registry = DataPointRegistry::instance();
    
    // Valid points
    report("testRegistryValidation_valid",
           registry.isValid(DataPointID::RPM) &&
           registry.isValid(DataPointID::BMSPackVoltage) &&
           registry.isValid(DataPointID::VESCRPM),
           "Known data points should be valid");
    
    // Invalid point
    report("testRegistryValidation_invalid",
           !registry.isValid(static_cast<DataPointID>(0xDEAD)),
           "Unknown data point should be invalid");
}

void TestHarness::testRegistryMetadata() {
    auto& registry = DataPointRegistry::instance();
    
    auto info = registry.getInfo(DataPointID::RPM);
    bool ok = QString(info.name).contains("RPM") &&
              QString(info.unit) == "RPM" &&
              QString(info.category) == "ENGINE/ECU";
    
    report("testRegistryMetadata_RPM",
           ok,
           QString("Name: %1, Unit: %2, Category: %3")
               .arg(info.name)
               .arg(info.unit)
               .arg(info.category));
    
    auto bmsInfo = registry.getInfo(DataPointID::BMSPackVoltage);
    bool bmsOk = QString(bmsInfo.name).contains("Voltage") &&
                 QString(bmsInfo.unit) == "V";
    report("testRegistryMetadata_BMS",
           bmsOk,
           QString("Name: %1, Unit: %2").arg(bmsInfo.name).arg(bmsInfo.unit));
}

void TestHarness::testRegistryConversion() {
    auto& registry = DataPointRegistry::instance();
    
    // RPM: raw * 0.01 = SI (e.g., 32000 raw → 320.0 RPM... wait, let me check)
    // Actually RPM has rawToSI = 1.0, so raw 3200 → SI 3200
    double rpmSI = registry.toSI(DataPointID::RPM, 3200);
    report("testRegistryConversion_RPM",
           qFuzzyCompare(rpmSI, 3200.0),
           QString("3200 raw → %1 SI (expected 3200)").arg(rpmSI));
    
    // CoolantTemp: raw * 0.01 = SI (e.g., 9500 raw → 95.0°C)
    double tempSI = registry.toSI(DataPointID::CoolantTemp, 9500);
    report("testRegistryConversion_CoolantTemp",
           qFuzzyCompare(tempSI, 95.0),
           QString("9500 raw → %1°C SI (expected 95)").arg(tempSI));
    
    // BatteryVoltage: raw * 0.01 = SI (e.g., 1260 raw → 12.6V)
    double voltSI = registry.toSI(DataPointID::BatteryVoltage, 1260);
    report("testRegistryConversion_BatteryVoltage",
           qFuzzyCompare(voltSI, 12.6),
           QString("1260 raw → %1V SI (expected 12.6)").arg(voltSI));
}

void TestHarness::testCategoryIndexing() {
    auto& registry = DataPointRegistry::instance();
    
    auto enginePoints = registry.pointsInCategory("ENGINE/ECU");
    bool hasRPM = enginePoints.contains(DataPointID::RPM);
    bool hasBoost = enginePoints.contains(DataPointID::BoostPressure);
    
    report("testCategoryIndexing_engine",
           hasRPM && hasBoost && enginePoints.size() > 5,
           QString("ENGINE/ECU category has %1 points (includes RPM, Boost)").arg(enginePoints.size()));
    
    auto bmsPoints = registry.pointsInCategory("BMS");
    bool hasPackV = bmsPoints.contains(DataPointID::BMSPackVoltage);
    bool hasSOC = bmsPoints.contains(DataPointID::BMSSOC);
    
    report("testCategoryIndexing_bms",
           hasPackV && hasSOC && bmsPoints.size() > 3,
           QString("BMS category has %1 points (includes PackV, SOC)").arg(bmsPoints.size()));
    
    auto allPoints = registry.allPoints();
    report("testCategoryIndexing_allPoints",
           allPoints.size() >= 50,
           QString("Total registered: %1 points").arg(allPoints.size()));
}

void TestHarness::testEndianness() {
    // Little-endian test
    QByteArray dataLE;
    dataLE.append(static_cast<char>(0x34));
    dataLE.append(static_cast<char>(0x12));
    
    uint16_t result = MdUtils::le16ToHost(dataLE, 0);
    bool leOk = (result == 0x1234);
    report("testEndianness_le16",
           leOk,
           QString("LE 0x34,0x12 → 0x%1 (expected 0x1234)").arg(result, 4, 16));
    
    // Big-endian test
    QByteArray dataBE;
    dataBE.append(static_cast<char>(0x12));
    dataBE.append(static_cast<char>(0x34));
    
    uint16_t beResult = MdUtils::be16ToHost(dataBE, 0);
    bool beOk = (beResult == 0x1234);
    report("testEndianness_be16",
           beOk,
           QString("BE 0x12,0x34 → 0x%1 (expected 0x1234)").arg(beResult, 4, 16));
    
    // 32-bit test
    QByteArray data32;
    data32.append(static_cast<char>(0x78));
    data32.append(static_cast<char>(0x56));
    data32.append(static_cast<char>(0x34));
    data32.append(static_cast<char>(0x12));
    
    uint32_t result32 = MdUtils::le32ToHost(data32, 0);
    bool le32Ok = (result32 == 0x12345678);
    report("testEndianness_le32",
           le32Ok,
           QString("LE 32-bit → 0x%1 (expected 0x12345678)").arg(result32, 8, 16));
}

void TestHarness::testChecksums() {
    QByteArray data;
    data.append('H');
    data.append('e');
    data.append('l');
    data.append('l');
    data.append('o');
    
    uint8_t xorSum = MdUtils::xorChecksum(data);
    report("testChecksums_xor",
           xorSum > 0, // Just verify it produces a non-zero result
           QString("XOR checksum of 'Hello' = 0x%1").arg(xorSum, 2, 16));
    
    uint16_t crc = MdUtils::crc16(data);
    report("testChecksums_crc16",
           crc > 0,
           QString("CRC-16 of 'Hello' = 0x%1").arg(crc, 4, 16));
}

void TestHarness::testClampLerp() {
    bool clampOk = MdUtils::clamp(5, 0, 10) == 5 &&
                   MdUtils::clamp(-5, 0, 10) == 0 &&
                   MdUtils::clamp(15, 0, 10) == 10;
    report("testClampLerp_clamp",
           clampOk,
           "Clamp: 5→5, -5→0, 15→10");
    
    double lerpResult = MdUtils::lerp(0.0, 10.0, 0.5);
    bool lerpOk = qFuzzyCompare(lerpResult, 5.0);
    report("testClampLerp_lerp",
           lerpOk,
           QString("Lerp(0, 10, 0.5) = %1 (expected 5)").arg(lerpResult));
}

void TestHarness::testAngleConversions() {
    double rad = MdUtils::degToRad(180.0);
    bool degOk = qAbs(rad - 3.14159265) < 0.0001;
    report("testAngleConversions_degToRad",
           degOk,
           QString("180° = %1 rad (expected π)").arg(rad, 0, 'f', 6));
    
    double deg = MdUtils::radToDeg(3.14159265);
    bool radOk = qAbs(deg - 180.0) < 0.0001;
    report("testAngleConversions_radToDeg",
           radOk,
           QString("π rad = %1° (expected 180)").arg(deg));
}

void TestHarness::testUnitConverter() {
    auto& conv = UnitConverter::instance();
    
    // Force known starting units (also verifies setDisplayUnit)
    conv.setDisplayUnit("°C", "°C");
    conv.setDisplayUnit("kPa", "kPa");
    conv.setDisplayUnit("λ", "λ");
    conv.setDisplayUnit("km/h", "km/h");
    
    // °C → °F
    conv.setDisplayUnit("°C", "°F");
    double f = conv.toDisplay("°C", 100.0);
    report("testUnitConverter_CtoF", qAbs(f - 212.0) < 0.001,
           QString("100°C = %1°F (expected 212)").arg(f));
    
    // kPa → PSI → bar via cycling
    QString u1 = conv.cycleUnit("kPa"); // kPa → PSI
    double psi = conv.toDisplay("kPa", 100.0);
    report("testUnitConverter_kPaToPSI", u1 == "PSI" && qAbs(psi - 14.5038) < 0.001,
           QString("100 kPa = %1 %2 (expected 14.5038 PSI)").arg(psi).arg(u1));
    
    QString u2 = conv.cycleUnit("kPa"); // PSI → bar
    double bar = conv.toDisplay("kPa", 100.0);
    report("testUnitConverter_kPaToBar", u2 == "bar" && qAbs(bar - 1.0) < 0.001,
           QString("100 kPa = %1 %2 (expected 1 bar)").arg(bar).arg(u2));
    
    QString u3 = conv.cycleUnit("kPa"); // bar → kPa (wraps around)
    report("testUnitConverter_cycleWraps", u3 == "kPa",
           QString("cycle wraps back to %1 (expected kPa)").arg(u3));
    
    // λ → AFR (stoich gasoline ×14.7)
    conv.setDisplayUnit("λ", "AFR");
    double afr = conv.toDisplay("λ", 1.0);
    report("testUnitConverter_lambdaToAFR", qAbs(afr - 14.7) < 0.001,
           QString("λ 1.00 = %1 AFR (expected 14.7)").arg(afr));
    
    // km/h → mph
    conv.setDisplayUnit("km/h", "mph");
    double mph = conv.toDisplay("km/h", 100.0);
    report("testUnitConverter_kmhToMph", qAbs(mph - 62.1371) < 0.001,
           QString("100 km/h = %1 mph (expected 62.1371)").arg(mph));
    
    // Non-convertible units pass through untouched
    double v = conv.toDisplay("V", 12.6);
    report("testUnitConverter_passthrough",
           !conv.isConvertible("V") && qAbs(v - 12.6) < 0.001,
           QString("12.6 V passes through as %1 (V not convertible)").arg(v));
    
    // Restore canonical defaults so persisted settings stay clean after tests
    conv.setDisplayUnit("°C", "°C");
    conv.setDisplayUnit("kPa", "kPa");
    conv.setDisplayUnit("λ", "λ");
    conv.setDisplayUnit("km/h", "km/h");
}

void TestHarness::report(const QString& test, bool passed, const QString& detail) {
    m_testsRun++;
    
    if (passed) {
        m_testsPassed++;
        if (detail.isEmpty()) {
            qDebug() << "  ✓" << test;
        } else {
            qDebug() << "  ✓" << test << ":" << detail;
        }
    } else {
        m_testsFailed++;
        qWarning() << "  ✗" << test << ":" << detail;
    }
}

// ──────────────────────────────────────────────
// MD-ECU frame decoder tests (golden fixtures + robustness)
// ──────────────────────────────────────────────

// Golden main-frame payload (92 bytes) with known field values.
// RPM 4782, boost 145.34, throttle 20.0%, λ 1.00, MAF 2.50, ECT 90.16,
// EGT1 777, batt 12.60V, speed 95.28 km/h, gear 3, N75 128/7.50,
// EFR 0xFFFF sentinel -> 0, knock 5000, full Digifant block.
static QByteArray fixturePayload()
{
    return QByteArray::fromHex(
        "78070000" "AE12" "C638" "33" "6400" "FA00" "3823" "0903"
        "0000000000000000000000000000"
        "EC04" "111122223333" "444455556666" "3825" "03" "80" "EE02"
        "C8" "12" "FFFF" "8813" "64" "96" "14" "50" "60" "0C" "22"
        "64" "38" "32" "05" "000000000000" "60" "01F4"
        "0102030405060708090A" "1770" "2E16" "2A");
}

static QByteArray fixtureFrame(bool goodEtx = true)
{
    QByteArray frame;
    frame.append(char(0x02));            // STX
    frame.append(char(95));               // TAG = total frame length (0x5F)
    frame.append(fixturePayload());
    frame.append(goodEtx ? char(0x03) : char(0x04));  // ETX (or corrupted)
    return frame;
}

void TestHarness::testMdDecoderMainFrame()
{
    MdFrameDecoder dec;
    QHash<DataPointID, double> out;
    bool done = false;
    const QByteArray frame = fixtureFrame();
    for (char c : frame) {
        if (dec.feedByte(static_cast<quint8>(c), out)) { done = true; break; }
    }

    report("testMdDecoder_mainFrame",
           done && out.value(DataPointID::RPM) == 4782.0
               && qFuzzyCompare(out.value(DataPointID::BoostPressure), 145.34)
               && qFuzzyCompare(out.value(DataPointID::ThrottlePosition), 20.0)
               && qFuzzyCompare(out.value(DataPointID::Lambda), 1.0)
               && qFuzzyCompare(out.value(DataPointID::MAF), 2.5)
               && qFuzzyCompare(out.value(DataPointID::CoolantTemp), 90.16),
           "core fields exact");

    report("testMdDecoder_egtBatterySpeed",
           out.value(DataPointID::EGT1) == 777.0
               && out.value(DataPointID::EGT2) == 0.0
               && qFuzzyCompare(out.value(DataPointID::BatteryVoltage), 12.6)
               && out.value(DataPointID::VdoPressure1) == 4369.0
               && out.value(DataPointID::VdoTemp1) == 17476.0
               && qFuzzyCompare(out.value(DataPointID::VehicleSpeed), 95.28)
               && out.value(DataPointID::Gear) == 3.0,
           "EGT/VDO/speed exact");

    report("testMdDecoder_efrSentinel",
           out.value(DataPointID::EFRSpeed) == 0.0
               && out.value(DataPointID::Knock) == 5000.0
               && out.value(DataPointID::N75DutyCycle) == 128.0
               && out.value(DataPointID::N75Pwm) == 200.0
               && qFuzzyCompare(out.value(DataPointID::N75Setpoint), 7.5)
               && out.value(DataPointID::EcuFlags) == 18.0,
           "EFR 0xFFFF sentinel -> 0, N75/knock exact");

    report("testMdDecoder_digifant",
           out.value(DataPointID::DFBoost) == 100.0
               && out.value(DataPointID::DFLambda) == 150.0
               && qFuzzyCompare(out.value(DataPointID::DFEct), 10.0)
               && qFuzzyCompare(out.value(DataPointID::DFIat), 63.0)
               && qFuzzyCompare(out.value(DataPointID::DFVoltage), 9.6)
               && qAbs(out.value(DataPointID::DFIgnition) - 38.7437) < 1e-9
               && qAbs(out.value(DataPointID::DFCyl1KnockRetard) - 17.57815) < 1e-9
               && out.value(DataPointID::DFCyl1KnockDecay) == 5.0
               && qFuzzyCompare(out.value(DataPointID::DFInjectPulse), 1000.0),
           "Digifant maps/formulas exact");

    report("testMdDecoder_digifantDerived",
           qFuzzyCompare(out.value(DataPointID::DFComputedRpm), 5000.0)
               && out.value(DataPointID::DFRpmDeltaHall) == 6000.0
               && out.value(DataPointID::DFKlineFreq) == 5678.0
               && out.value(DataPointID::DFActiveFrame) == 42.0,
           "computed RPM 30e6/6000 = 5000");
}

void TestHarness::testMdDecoderDiscardResync()
{
    MdFrameDecoder dec;
    QHash<DataPointID, double> out;

    // 1) Corrupted frame (wrong byte at the length-based end position) is dropped
    bool decoded = false;
    for (char c : fixtureFrame(false)) {
        if (dec.feedByte(static_cast<quint8>(c), out)) { decoded = true; break; }
    }
    report("testMdDecoder_discardCorrupt",
           !decoded && dec.discardedCount() == 1,
           "frame with bad ETX discarded, not decoded");

    // 2) Decoder re-syncs on the next valid frame (garbage between frames ignored)
    QByteArray stream;
    stream.append("JUNK");
    stream.append(fixtureFrame(true));
    decoded = false;
    out.clear();
    for (char c : stream) {
        if (dec.feedByte(static_cast<quint8>(c), out)) { decoded = true; break; }
    }
    report("testMdDecoder_resync",
           decoded && out.value(DataPointID::RPM) == 4782.0 && dec.discardedCount() == 1,
           "resyncs after corruption, values still exact");
}

void TestHarness::testMdDecoderFuzz()
{
    // Seeded random byte stream must never crash the decoder nor wedge it:
    // after the fuzz burst the golden frame still decodes exactly.
    QRandomGenerator rg(20261012u);
    MdFrameDecoder dec;
    QHash<DataPointID, double> junk;
    for (int i = 0; i < 50000; i++) {
        dec.feedByte(static_cast<quint8>(rg.bounded(256)), junk);
    }
    report("testMdDecoder_fuzzSurvival", true,
           QString("50000 random bytes survived (discarded=%1)").arg(dec.discardedCount()));

    bool decoded = false;
    QHash<DataPointID, double> out;
    for (char c : fixtureFrame(true)) {
        if (dec.feedByte(static_cast<quint8>(c), out)) { decoded = true; break; }
    }
    report("testMdDecoder_fuzzThenExact",
           decoded && out.value(DataPointID::RPM) == 4782.0
               && qFuzzyCompare(out.value(DataPointID::BoostPressure), 145.34),
           "decodes golden frame after fuzz");
}
