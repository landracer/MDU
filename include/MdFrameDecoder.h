/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdFrameDecoder.h
 * @brief Pure, hardware-independent MD-ECU frame decoder (state machine + payload decode).
 *
 * Split out of MdSerialComUnified so the wire protocol is unit-testable without
 * a serial port. Framing contract (ancestor app MdSerialComBinary):
 *  - STX 0x02 | TAG | payload | ETX 0x03
 *  - the TAG byte's decimal value IS the total frame length (main frame 0x5F = 95)
 *  - ETX is validated only at byte position (frameLength − 1); 0x03 inside a
 *    payload never ends a frame
 */
#ifndef MD_FRAME_DECODER_H
#define MD_FRAME_DECODER_H

#include "UnifiedDataPoint.h"
#include <QByteArray>
#include <QHash>
#include <QVector>
#include <initializer_list>

// Frame markers
#define MD_FRAME_BEGIN 0x02   // STX
#define MD_FRAME_END   0x03   // ETX

// Tag constants: a tag's decimal value IS the total frame length in bytes
#define MD_SERIALOUT_BINARY_TAG                   95  // 0x5F main sensor frame (92-byte payload)
#define MD_SERIALOUT_BINARY_BOOSTPID_TAG          69  // 0x45 boost PID frame
#define MD_SERIALOUT_BINARY_TAG_N75_DUTY_MAP      22  // 0x16 N75 duty map frame
#define MD_SERIALOUT_BINARY_TAG_N75_SETPOINT_MAP  38  // 0x26 N75 setpoint map frame
#define MD_SERIALOUT_BINARY_TAG_GEAR_RATIO_6G     17  // 0x11 6-speed gear ratio frame
#define MD_SERIALOUT_BINARY_TAG_ACK                4  // 0x04 acknowledgement frame
#define MD_SERIALOUT_BINARY_TAG_N75_PARAMS         23  // 0x17 N75 params frame

#define MD_MAINFRAME_PAYLOAD_SIZE 92
#define MD_MAXFRAME_SIZE          97   // main frame (95) + margin, matching the ancestor buffer

// Frame state machine states
enum FrameStatus {
    MD_STATUS_RECEIVING       = 1,
    MD_STATUS_FRAME_COMPLETE  = 2,
    MD_STATUS_FRAMEERROR      = 3,
    MD_STATUS_WAITING_FOR_TAG = 4
};

/**
 * @brief 17-point piecewise-linear lookup used by the Digifant firmware
 *
 * Port of the ancestor Map16x1: value = interpolation between mapData[idx] and
 * mapData[idx+1] using the low nibble as the fraction.
 */
class DigifantMap {
public:
    DigifantMap() = default;
    void setCurve(std::initializer_list<double> values);
    double mapValue(int raw) const;

private:
    QVector<double> m_data;
};

/** Preloaded ECT / IAT / voltage curves ported from the ancestor app. */
class DigifantMaps {
public:
    DigifantMaps();
    double ect(int raw) const     { return m_ect.mapValue(raw); }
    double iat(int raw) const     { return m_iat.mapValue(raw); }
    double voltage(int raw) const { return m_voltage.mapValue(raw); }

private:
    DigifantMap m_ect;
    DigifantMap m_iat;
    DigifantMap m_voltage;
};

/**
 * @brief Byte-at-a-time frame decoder with the corrected framing contract.
 *
 * feedByte() returns true (and fills outValues) exactly once per complete,
 * ETX-validated main sensor frame. Control frames are framed and counted but
 * not decoded (roadmap: N75 map / gear / boost-PID payload decode).
 */
class MdFrameDecoder {
public:
    void reset();

    /// Feed one wire byte. Returns true when a main sensor frame completed.
    bool feedByte(quint8 byte, QHash<DataPointID, double>& outValues);

    quint8 lastTag() const { return m_tag; }
    int discardedCount() const { return m_discarded; }
    int controlCount() const { return m_control; }

    /// Decode a complete 92-byte main-frame payload (pure function).
    static QHash<DataPointID, double> decodeMainFrame(const QByteArray& payload);

private:
    int m_index = 0;
    int m_status = MD_STATUS_FRAME_COMPLETE;
    int m_framelength = 0;
    quint8 m_tag = 0;
    int m_discarded = 0;
    int m_control = 0;
    quint8 m_buf[MD_MAXFRAME_SIZE];
};

#endif // MD_FRAME_DECODER_H