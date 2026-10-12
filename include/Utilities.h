/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>
#include <QByteArray>
#include <cstdint>

/**
 * @brief Common utility functions for the multidisplay-unified project
 */
namespace MdUtils {

/**
 * @brief Convert bytes from little-endian to host order
 */
inline uint16_t le16ToHost(const QByteArray& data, int offset) {
    if (offset + 1 >= data.size()) return 0;
    return static_cast<uint16_t>(static_cast<uint8_t>(data[offset])) |
           (static_cast<uint16_t>(static_cast<uint8_t>(data[offset + 1])) << 8);
}

inline int16_t le16ToHostSigned(const QByteArray& data, int offset) {
    return static_cast<int16_t>(le16ToHost(data, offset));
}

inline uint32_t le32ToHost(const QByteArray& data, int offset) {
    if (offset + 3 >= data.size()) return 0;
    return static_cast<uint32_t>(static_cast<uint8_t>(data[offset])) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 1])) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 2])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 3])) << 24);
}

inline int32_t le32ToHostSigned(const QByteArray& data, int offset) {
    return static_cast<int32_t>(le32ToHost(data, offset));
}

/**
 * @brief Convert bytes from big-endian to host order
 */
inline uint16_t be16ToHost(const QByteArray& data, int offset) {
    if (offset + 1 >= data.size()) return 0;
    return (static_cast<uint16_t>(static_cast<uint8_t>(data[offset])) << 8) |
           static_cast<uint16_t>(static_cast<uint8_t>(data[offset + 1]));
}

inline uint32_t be32ToHost(const QByteArray& data, int offset) {
    if (offset + 3 >= data.size()) return 0;
    return (static_cast<uint32_t>(static_cast<uint8_t>(data[offset])) << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 2])) << 8) |
           static_cast<uint32_t>(static_cast<uint8_t>(data[offset + 3]));
}

/**
 * @brief Calculate XOR checksum over byte array
 */
inline uint8_t xorChecksum(const QByteArray& data) {
    uint8_t checksum = 0;
    for (int i = 0; i < data.size(); i++) {
        checksum ^= static_cast<uint8_t>(data[i]);
    }
    return checksum;
}

/**
 * @brief Calculate CRC-16/CCITT over byte array
 */
inline uint16_t crc16(const QByteArray& data) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < data.size(); i++) {
        crc ^= static_cast<uint16_t>(static_cast<uint8_t>(data[i])) << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/**
 * @brief Clamp value between min and max
 */
template<typename T>
T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief Linear interpolation
 */
inline double lerp(double a, double b, double t) {
    return a + (b - a) * qBound(0.0, t, 1.0);
}

/**
 * @brief Convert degrees to radians
 */
inline double degToRad(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

/**
 * @brief Convert radians to degrees
 */
inline double radToDeg(double radians) {
    return radians * 180.0 / 3.14159265358979323846;
}

/**
 * @brief Check if value is within tolerance of target
 */
inline bool fuzzyCompare(double a, double b, double tolerance = 0.001) {
    return qAbs(a - b) < tolerance;
}

/**
 * @brief Format a number with appropriate units
 */
QString formatValue(double value, const QString& unit, int decimals = 2);

/**
 * @brief Get human-readable time duration
 */
QString formatDuration(qint64 milliseconds);

/**
 * @brief Sanitize string for display (remove control characters)
 */
QString sanitizeDisplay(const QString& input);

} // namespace MdUtils

#endif // UTILITIES_H
