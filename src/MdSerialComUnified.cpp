/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdSerialComUnified.cpp
 * @brief MultiDisplay ECU (ATmega2560) binary frame parser implementation.
 *
 * Framing ported faithfully from the original app's MdSerialComBinary:
 *   STX(0x02) | TAG | PAYLOAD... | ETX(0x03)
 * The TAG byte value IS the total frame length in bytes; ETX is validated at
 * byte index (frameLength - 1). Payload bytes equal to 0x03 never terminate a
 * frame (the previous unified build terminated on the first 0x03 seen, which
 * corrupted every frame whose payload contained a 0x03 byte).
 */
#include "MdSerialComUnified.h"
#include <QDebug>

MdSerialComUnified::MdSerialComUnified(const QString& portName, QObject* parent)
    : ICommunication("MD-ECU", parent)
    , m_portName(portName)
    , m_serial(new QSerialPort(this))
    , m_baudRate(115200)
    , m_index(0)
    , m_status(MD_STATUS_FRAME_COMPLETE)
    , m_framelength(0)
{
    m_serial->setPortName(m_portName);

    connect(m_serial, &QSerialPort::readyRead, this, &MdSerialComUnified::onReadyRead);
    connect(m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred(m_serial->errorString());
        }
    });
}

void MdSerialComUnified::setPortName(const QString& name) {
    m_portName = name;
    m_serial->setPortName(name);
}

void MdSerialComUnified::setBaudRate(int baud) {
    m_baudRate = baud;
}

bool MdSerialComUnified::open() {
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_connected = true;
        emit connectionStateChanged(true);
        qDebug() << "[MD-ECU] Connected to" << m_portName << "@" << m_baudRate;
        return true;
    }

    emit errorOccurred("Cannot open port: " + m_serial->errorString());
    return false;
}

void MdSerialComUnified::close() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    emit connectionStateChanged(false);
    qDebug() << "[MD-ECU] Disconnected";
}

bool MdSerialComUnified::isConnected() const {
    return m_connected && m_serial->isOpen();
}

void MdSerialComUnified::onReadyRead() {
    const QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    emit rawReceived(data, data.length());

    // Process bytes one at a time through the length-based state machine.
    // Serial data can arrive in any chunking; frames may span readAll() calls.
    for (int i = 0; i < data.size(); i++) {
        const quint8 d = static_cast<quint8>(data.at(i));

        switch (m_status) {

        case MD_STATUS_FRAMEERROR:
        case MD_STATUS_FRAME_COMPLETE:
            // Waiting for the start of a new frame
            m_index = 0;
            if (d == MD_FRAME_BEGIN) {  // STX
                m_status = MD_STATUS_WAITING_FOR_TAG;
                rcvData[0] = d;
                m_index = 1;
            }
            // else: skip non-STX bytes
            break;

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
                rcvData[m_index] = d;
                m_index++;
                break;
            default:
                qWarning() << "[MD-ECU] Expected tag but got:"
                           << QString("0x%1").arg(d, 2, 16, QChar('0'));
                m_status = MD_STATUS_FRAMEERROR;
                m_index = 0;
                break;
            }
            break;

        case MD_STATUS_RECEIVING:
            rcvData[m_index] = d;
            if (m_index == m_framelength - 1) {
                // Final byte of the frame: it must be ETX, otherwise discard
                if (d != MD_FRAME_END) {
                    m_discardedFrames++;
                    if (m_discardedFrames % 100 == 1) {
                        qWarning() << "[MD-ECU] Frame discarded (ETX mismatch)!"
                                   << "expected length=" << m_framelength
                                   << "#discarded=" << m_discardedFrames;
                    }
                    m_status = MD_STATUS_FRAMEERROR;
                    m_index = 0;
                    break;
                }
                m_status = MD_STATUS_FRAME_COMPLETE;
                m_index = 0;
                const QByteArray frame(reinterpret_cast<const char*>(rcvData), m_framelength);
                dispatchFrame(frame);
            } else {
                m_index++;
                if (m_index >= MD_MAXFRAME_SIZE) {  // safety, cannot occur with max tag 95
                    m_status = MD_STATUS_FRAMEERROR;
                    m_index = 0;
                }
            }
            break;
        }
    }
}

void MdSerialComUnified::dispatchFrame(const QByteArray& frame) {
    const quint8 tag = static_cast<quint8>(frame.at(1));
    const QByteArray payload = frame.mid(2, frame.size() - 3);  // strip STX + TAG + ETX

    if (tag == MD_SERIALOUT_BINARY_TAG) {
        processMainData(payload);
        return;
    }

    // Control/config frames (N75 duty/setpoint maps, N75 params, gear ratios,
    // boost PID, ACK) are correctly framed here but not yet decoded into data
    // points — tracked as roadmap items in A-class.md.
    qDebug() << "[MD-ECU] Control frame received, tag=0x"
             << QString("%1").arg(tag, 2, 16, QChar('0'))
             << "payload" << payload.size() << "bytes (decode pending)";
}

void MdSerialComUnified::processMainData(const QByteArray& data) {
    // True main-frame payload layout (92 bytes), derived from the original
    // app's convertReceivedMd2Frame(). Offsets are payload-relative:
    //   0-3    timestamp millis (not mapped)
    //   4-5    calRPM           int16 LE, raw RPM
    //   6-7    calAbsoluteBoost uint16 LE /100 (absolute boost; the original
    //          app applied (raw/100 - 1). Gauge-vs-absolute semantics is an
    //          open bench-verification item — see A-class.md)
    //   8      throttle uint8, /255 -> %
    //   9-10   calLambdaF /100 -> lambda ratio
    //   11-12  calLMM /100 -> g/s (MAF)
    //   13-14  caseTemp /100 -> degC
    //   15-30  EGT1-8 (8 x uint16 LE, raw counts; original firmware scale unverified)
    //   31-32  batVolt /100 -> V
    //   33-44  VDO pressure 1-3 + VDO temp 1-3 (raw; no DP mapping yet)
    //   45-46  speedF /100 -> km/h
    //   47     gear (0 = neutral, 1-6)
    //   48     N75 duty (raw 0-255)
    //   49-50  N75 requested boost /100
    //   51     N75 requested boost PWM (raw; no DP yet)
    //   52     status flags (no DP yet)
    //   53-54  EFR reading: 0xFFFF sentinel = no sensor, else 40,000,000/raw -> RPM
    //   55-56  knock (raw; no DP yet)
    //   57-91  Digifant (68HC11) block (not mapped — roadmap)
    if (data.size() < MD_MAINFRAME_PAYLOAD_SIZE) {
        return;  // not a full main sensor frame
    }

    QHash<DataPointID, double> values;

    // Raw byte access for precise offset control
    const uchar* d = reinterpret_cast<const uchar*>(data.constData());

    // Payload 0-3: timestamp millis — not mapped, just received

    // RPM (int16 LE, offset 4-5) — raw value, no scaling
    const int16_t rpm_raw = static_cast<int16_t>(d[4] | (d[5] << 8));
    values[DataPointID::RPM] = static_cast<double>(rpm_raw);

    // Boost (uint16 LE, offset 6-7) — fixed-point /100. NOTE: original app
    // treated this as absolute and subtracted 1.0 before display; unified
    // stores the raw scaled value — verify semantics on the bench.
    const uint16_t boost_raw = static_cast<uint16_t>(d[6] | (d[7] << 8));
    values[DataPointID::BoostPressure] = boost_raw / 100.0;

    // Throttle (uint8, offset 8) — /255 -> %
    values[DataPointID::ThrottlePosition] = static_cast<double>(d[8]) / 255.0 * 100.0;

    // Lambda (uint16 LE, offset 9-10) — /100 -> ratio
    const uint16_t lambda_raw = static_cast<uint16_t>(d[9] | (d[10] << 8));
    values[DataPointID::Lambda] = lambda_raw / 100.0;

    // LMM/MAF (uint16 LE, offset 11-12) — /100 -> g/s
    const uint16_t lmm_raw = static_cast<uint16_t>(d[11] | (d[12] << 8));
    values[DataPointID::MAF] = lmm_raw / 100.0;

    // Case temperature (uint16 LE, offset 13-14) — /100 -> degC
    const uint16_t caseTemp_raw = static_cast<uint16_t>(d[13] | (d[14] << 8));
    values[DataPointID::CoolantTemp] = caseTemp_raw / 100.0;

    // EGT channels 1-8 (uint16 LE each, offsets 15-30) — raw counts
    static const DataPointID egtIds[8] = {
        DataPointID::EGT1, DataPointID::EGT2, DataPointID::EGT3, DataPointID::EGT4,
        DataPointID::EGT5, DataPointID::EGT6, DataPointID::EGT7, DataPointID::EGT8
    };
    for (int ch = 0; ch < 8; ch++) {
        const uint16_t egt_raw = static_cast<uint16_t>(d[15 + ch * 2] | (d[15 + ch * 2 + 1] << 8));
        values[egtIds[ch]] = static_cast<double>(egt_raw);
    }

    // Battery voltage (uint16 LE, offset 31-32) — /100 -> V
    const uint16_t bat_raw = static_cast<uint16_t>(d[31] | (d[32] << 8));
    values[DataPointID::BatteryVoltage] = bat_raw / 100.0;

    // VDO sensors (offsets 33-44) — raw, not mapped to DP IDs yet
    const uint16_t vdoPres1 = static_cast<uint16_t>(d[33] | (d[34] << 8));
    const uint16_t vdoPres2 = static_cast<uint16_t>(d[35] | (d[36] << 8));
    const uint16_t vdoPres3 = static_cast<uint16_t>(d[37] | (d[38] << 8));
    const uint16_t vdoTemp1 = static_cast<uint16_t>(d[39] | (d[40] << 8));
    const uint16_t vdoTemp2 = static_cast<uint16_t>(d[41] | (d[42] << 8));
    const uint16_t vdoTemp3 = static_cast<uint16_t>(d[43] | (d[44] << 8));
    Q_UNUSED(vdoPres1); Q_UNUSED(vdoPres2); Q_UNUSED(vdoPres3);
    Q_UNUSED(vdoTemp1); Q_UNUSED(vdoTemp2); Q_UNUSED(vdoTemp3);

    // Vehicle speed (uint16 LE, offset 45-46) — /100 -> km/h
    const uint16_t speed_raw = static_cast<uint16_t>(d[45] | (d[46] << 8));
    values[DataPointID::VehicleSpeed] = speed_raw / 100.0;

    // Gear (uint8, offset 47) — 0 = neutral, 1-6 = gear
    values[DataPointID::Gear] = static_cast<double>(static_cast<uint8_t>(d[47]));

    // N75 duty (uint8, offset 48) — raw 0-255 PWM
    values[DataPointID::N75DutyCycle] = static_cast<double>(static_cast<uint8_t>(d[48]));

    // N75 requested boost (uint16 LE, offset 49-50) — /100
    const uint16_t n75_boost_raw = static_cast<uint16_t>(d[49] | (d[50] << 8));
    values[DataPointID::N75Setpoint] = n75_boost_raw / 100.0;

    // Payload 51 (N75 PWM output), 52 (status flags), 55-56 (knock): parsed
    // upstream in firmware but not yet mapped to DataPointIDs (roadmap).

    // EFR turbo speed (uint16 LE, offset 53-54): 0xFFFF = no sensor fitted
    const uint16_t efr_raw = static_cast<uint16_t>(d[53] | (d[54] << 8));
    if (efr_raw == 0xFFFF) {
        values[DataPointID::EFRSpeed] = 0.0;  // sensor not present
    } else {
        values[DataPointID::EFRSpeed] = 40000000.0 / static_cast<double>(efr_raw);
    }

    // Emit parsed data points
    if (!values.isEmpty()) {
        emit framesParsed(values, "MD-ECU");
        emit frameParsed(values);
    }
}
