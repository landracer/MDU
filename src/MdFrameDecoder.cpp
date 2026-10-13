/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdFrameDecoder.cpp
 * @brief Pure MD-ECU frame decoder implementation (framing + main-frame + Digifant decode).
 *
 * Byte order per the ancestor app: main-frame AVR fields are little-endian
 * (lo + (hi << 8)); the Digifant injector-pulse and hall-delta fields keep the
 * 68HC11's native big-endian order.
 */
#include "MdFrameDecoder.h"

// ──────────────────────────────────────────────
// Digifant lookup curves (ported from ancestor Map16x1.cpp)
// ──────────────────────────────────────────────
void DigifantMap::setCurve(std::initializer_list<double> values) {
    m_data.clear();
    m_data.reserve(static_cast<int>(values.size()));
    for (double v : values) m_data.append(v);
}

double DigifantMap::mapValue(int raw) const {
    if (m_data.size() < 17) return 0.0;
    const int idx = raw >> 4;
    const int w = raw & 0xF;
    if (idx >= 0xF) return m_data.at(qMin(idx, m_data.size() - 1));
    // ancestor: ((w * d[idx+1]) + (-(w-16) * d[idx])) / 16
    return ((w * m_data.at(idx + 1)) + ((16 - w) * m_data.at(idx))) / 16.0;
}

DigifantMaps::DigifantMaps() {
    m_ect.setCurve({130, 65, 38, 28, 18, 10, 2, -6, -14, -22, -30, -38, -46, -54, -62, -62, -62});
    m_iat.setCurve({117, 108, 99, 90, 81, 72, 63, 54, 45, 36, 27, 18, 9, 0, -9, -18, -27});
    m_voltage.setCurve({0, 1.6, 3.2, 4.8, 6.4, 8, 9.6, 11.2, 12.8, 14.3, 15.9, 17.5, 19.1, 20.7, 22.3, 23.9, 25.5});
}

// ──────────────────────────────────────────────
// State machine
// ──────────────────────────────────────────────
void MdFrameDecoder::reset() {
    m_index = 0;
    m_status = MD_STATUS_FRAME_COMPLETE;
    m_framelength = 0;
    m_tag = 0;
}

bool MdFrameDecoder::feedByte(quint8 d, QHash<DataPointID, double>& outValues) {
    switch (m_status) {

    case MD_STATUS_FRAMEERROR:
    case MD_STATUS_FRAME_COMPLETE:
        // Waiting for the start of a new frame
        m_index = 0;
        if (d == MD_FRAME_BEGIN) {  // STX
            m_status = MD_STATUS_WAITING_FOR_TAG;
            m_buf[0] = d;
            m_index = 1;
        }
        return false;

    case MD_STATUS_WAITING_FOR_TAG:
        switch (d) {
        case MD_SERIALOUT_BINARY_TAG:
        case MD_SERIALOUT_BINARY_BOOSTPID_TAG:
        case MD_SERIALOUT_BINARY_TAG_N75_DUTY_MAP:
        case MD_SERIALOUT_BINARY_TAG_N75_SETPOINT_MAP:
        case MD_SERIALOUT_BINARY_TAG_ACK:
        case MD_SERIALOUT_BINARY_TAG_N75_PARAMS:
        case MD_SERIALOUT_BINARY_TAG_GEAR_RATIO_6G:
            m_status = MD_STATUS_RECEIVING;
            m_framelength = d;  // tag value IS the total frame length
            m_tag = d;
            m_buf[m_index] = d;
            m_index++;
            break;
        default:
            m_status = MD_STATUS_FRAMEERROR;
            m_index = 0;
            break;
        }
        return false;

    case MD_STATUS_RECEIVING:
        m_buf[m_index] = d;
        if (m_index == m_framelength - 1) {
            // Final byte of the frame: it must be ETX, otherwise discard
            if (d != MD_FRAME_END) {
                m_discarded++;
                m_status = MD_STATUS_FRAMEERROR;
                m_index = 0;
                return false;
            }
            m_status = MD_STATUS_FRAME_COMPLETE;
            m_index = 0;
            if (m_tag == MD_SERIALOUT_BINARY_TAG) {
                outValues = decodeMainFrame(
                    QByteArray(reinterpret_cast<const char*>(m_buf + 2), m_framelength - 3));
                return true;
            }
            m_control++;  // framed control/config frame — decode pending (roadmap)
            return false;
        }
        m_index++;
        if (m_index >= MD_MAXFRAME_SIZE) {  // safety, cannot occur with max tag 95
            m_status = MD_STATUS_FRAMEERROR;
            m_index = 0;
        }
        return false;
    }

    return false;
}

// ──────────────────────────────────────────────
// Main sensor frame decode (92-byte payload)
// ──────────────────────────────────────────────
QHash<DataPointID, double> MdFrameDecoder::decodeMainFrame(const QByteArray& data) {
    QHash<DataPointID, double> values;
    if (data.size() < MD_MAINFRAME_PAYLOAD_SIZE) {
        return values;  // not a full main sensor frame
    }
    const uchar* d = reinterpret_cast<const uchar*>(data.constData());

    auto le16 = [&d](int at) { return static_cast<quint16>(d[at] | (d[at + 1] << 8)); };
    auto be16 = [&d](int at) { return static_cast<quint16>((d[at] << 8) | d[at + 1]); };

    // Payload 0-3: timestamp millis — received, not mapped
    values[DataPointID::RPM] = static_cast<double>(static_cast<qint16>(le16(4)));
    values[DataPointID::BoostPressure] = le16(6) / 100.0;    // absolute; bench check pending
    values[DataPointID::ThrottlePosition] = d[8] / 255.0 * 100.0;
    values[DataPointID::Lambda] = le16(9) / 100.0;
    values[DataPointID::MAF] = le16(11) / 100.0;
    values[DataPointID::CoolantTemp] = le16(13) / 100.0;

    static const DataPointID egtIds[8] = {
        DataPointID::EGT1, DataPointID::EGT2, DataPointID::EGT3, DataPointID::EGT4,
        DataPointID::EGT5, DataPointID::EGT6, DataPointID::EGT7, DataPointID::EGT8
    };
    for (int ch = 0; ch < 8; ch++) {
        values[egtIds[ch]] = static_cast<double>(le16(15 + ch * 2));
    }

    values[DataPointID::BatteryVoltage] = le16(31) / 100.0;
    values[DataPointID::VdoPressure1] = static_cast<double>(le16(33));
    values[DataPointID::VdoPressure2] = static_cast<double>(le16(35));
    values[DataPointID::VdoPressure3] = static_cast<double>(le16(37));
    values[DataPointID::VdoTemp1] = static_cast<double>(le16(39));
    values[DataPointID::VdoTemp2] = static_cast<double>(le16(41));
    values[DataPointID::VdoTemp3] = static_cast<double>(le16(43));
    values[DataPointID::VehicleSpeed] = le16(45) / 100.0;
    values[DataPointID::Gear] = static_cast<double>(static_cast<quint8>(d[47]));
    values[DataPointID::N75DutyCycle] = static_cast<double>(static_cast<quint8>(d[48]));
    values[DataPointID::N75Setpoint] = le16(49) / 100.0;
    values[DataPointID::N75Pwm] = static_cast<double>(static_cast<quint8>(d[51]));
    values[DataPointID::EcuFlags] = static_cast<double>(static_cast<quint8>(d[52]));

    // EFR turbo speed: 0xFFFF sentinel = sensor not fitted
    const quint16 efr_raw = le16(53);
    values[DataPointID::EFRSpeed] = (efr_raw == 0xFFFF)
        ? 0.0
        : 40000000.0 / static_cast<double>(efr_raw);

    values[DataPointID::Knock] = static_cast<double>(le16(55));

    // ── Digifant block (payload 57-91) ──
    static const DigifantMaps kMaps;  // immutable, built once

    values[DataPointID::DFBoost] = static_cast<double>(static_cast<quint8>(d[57]));
    values[DataPointID::DFLambda] = static_cast<double>(static_cast<quint8>(d[58]));
    values[DataPointID::DFKnock] = static_cast<double>(static_cast<quint8>(d[59]));
    values[DataPointID::DFEct] = kMaps.ect(static_cast<quint8>(d[60]));
    values[DataPointID::DFIat] = kMaps.iat(static_cast<quint8>(d[61]));
    values[DataPointID::DFThrottlePoti] = static_cast<double>(static_cast<quint8>(d[62]));
    values[DataPointID::DFFlags] = static_cast<double>(static_cast<quint8>(d[63]));
    values[DataPointID::DFIgnition] = static_cast<quint8>(d[64]) * -0.351563 + 73.9;
    values[DataPointID::DFLcFlags] = static_cast<double>(static_cast<quint8>(d[65]));

    // cyl1-4 knock retard/decay pairs: 66/67, 68/69, 70/71, 72/73
    static const DataPointID krIds[4] = {
        DataPointID::DFCyl1KnockRetard, DataPointID::DFCyl2KnockRetard,
        DataPointID::DFCyl3KnockRetard, DataPointID::DFCyl4KnockRetard };
    static const DataPointID kdIds[4] = {
        DataPointID::DFCyl1KnockDecay, DataPointID::DFCyl2KnockDecay,
        DataPointID::DFCyl3KnockDecay, DataPointID::DFCyl4KnockDecay };
    for (int cyl = 0; cyl < 4; cyl++) {
        values[krIds[cyl]] = static_cast<quint8>(d[66 + cyl * 2]) * 0.351563;
        values[kdIds[cyl]] = static_cast<double>(static_cast<quint8>(d[67 + cyl * 2]));
    }

    values[DataPointID::DFVoltage] = kMaps.voltage(static_cast<quint8>(d[74]));
    values[DataPointID::DFInjectPulse] = static_cast<double>(be16(75)) * 2.0;  // µs
    values[DataPointID::DFColdEnrich] = static_cast<double>(static_cast<quint8>(d[77]));
    values[DataPointID::DFWarmEnrich] = static_cast<double>(static_cast<quint8>(d[78]));
    values[DataPointID::DFEctEnrich] = static_cast<double>(static_cast<quint8>(d[79]));
    values[DataPointID::DFAccelEnrich] = static_cast<double>(static_cast<quint8>(d[80]));
    values[DataPointID::DFCounterEnrich] = static_cast<double>(static_cast<quint8>(d[81]));
    values[DataPointID::DFIatEnrich] = static_cast<double>(static_cast<quint8>(d[82]));
    values[DataPointID::DFIgnCounter] = static_cast<double>(static_cast<quint8>(d[83]));
    values[DataPointID::DFIgnAddon] = static_cast<double>(static_cast<quint8>(d[84]));
    values[DataPointID::DFEctAddon] = static_cast<double>(static_cast<quint8>(d[85]));
    values[DataPointID::DFISV] = static_cast<double>(static_cast<quint8>(d[86]));

    const quint16 hallDelta = be16(87);
    values[DataPointID::DFRpmDeltaHall] = static_cast<double>(hallDelta);
    values[DataPointID::DFComputedRpm] = (hallDelta > 0)
        ? 30000000.0 / static_cast<double>(hallDelta) : 0.0;
    values[DataPointID::DFKlineFreq] = static_cast<double>(le16(89));
    values[DataPointID::DFActiveFrame] = static_cast<double>(static_cast<quint8>(d[91]));

    return values;
}