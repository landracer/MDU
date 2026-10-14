/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file SessionLog.h
 * @brief Pillar 4 — session recording & replay.
 *
 * Binary session log format (magic "MDSN"): a header (start time, block
 * capacity), a sequence of sample blocks, and a footer index mapping
 * timestamp → block offset so replay can seek without scanning the file.
 * Format is dense LE binary:
 *
 *   header : "MDSN" | u16 version | u16 rsv | i64 startEpochMs | u32 blockCap | u32 rsv
 *   block  : "MSBK" | i64 t0RelUs | u32 count | count × (i64 relUs | u32 dpId | f64 value)
 *   footer : "MSEN" | u16 version | u16 rsv | u32 blockCount | u64 sampleCount | i64 indexOff
 *   index  : blockCount × (i64 t0RelUs | i64 blockOff)
 *
 * Deliberately Qt-core only (no widgets, no serial) so it is fully unit-testable.
 */
#ifndef SESSIONLOG_H
#define SESSIONLOG_H

#include <QByteArray>
#include <QFile>
#include <QDateTime>
#include <QVector>
#include <QList>
#include <QString>
#include "UnifiedDataPoint.h"

/** One replayed sample: data point, canonical value, time relative to session start. */
struct SessionSample {
    DataPointID id = DataPointID::Invalid;
    double value = 0.0;
    qint64 relUs = 0;   // microseconds since the session start timestamp
};

/** Everything decoded from a session file. */
struct SessionFile {
    QDateTime startTime;              // wall clock at recording start
    quint32 blockCapacity = 0;        // as recorded in the header
    QVector<SessionSample> samples;   // write-order (== chronological)
    struct BlockMeta { qint64 t0RelUs = 0; qint64 offset = 0; };
    QList<BlockMeta> index;           // timestamp index (sorted by t0RelUs)

    qint64 sampleCount() const { return samples.size(); }
};

class SessionLog {
public:
    static constexpr quint32 kVersion = 1;

    // ── recording ──────────────────────────────────────────────────────
    /** Create a new session file (truncating any existing file). */
    bool create(const QString& path,
                const QDateTime& start = QDateTime::currentDateTimeUtc(),
                quint32 blockCapacity = 256);
    /** Buffer one sample; auto-flushes a block when the buffer is full. */
    bool append(DataPointID id, double value, const QDateTime& ts);
    /** Flush buffered samples as a block (file stays open for more data). */
    bool flush();
    /** Flush remaining samples, write footer + timestamp index, close. */
    bool finalize();
    bool isRecording() const { return m_file != nullptr; }
    QString path() const { return m_file ? m_file->fileName() : QString(); }
    qint64 recordedCount() const { return m_sampleCount; }

    // ── replay ─────────────────────────────────────────────────────────
    /** Parse a session file; false on magic/version mismatch or truncation. */
    static bool read(const QString& path, SessionFile* out, QString* error = nullptr);
    /** Binary-search the index: samples recorded between two relative times. */
    static QVector<SessionSample> slice(const SessionFile& f, qint64 fromRelUs, qint64 toRelUs);

    // ── export (with unit metadata) ────────────────────────────────────
    static QString toCsv(const SessionFile& f);
    static QByteArray toJson(const SessionFile& f);

    ~SessionLog();

private:
    QFile* m_file = nullptr;
    QDateTime m_start;
    quint32 m_blockCapacity = 0;
    quint32 m_blockCount = 0;
    qint64 m_sampleCount = 0;
    QVector<SessionSample> m_buffer;
    QList<SessionFile::BlockMeta> m_index;
};

/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */

#endif // SESSIONLOG_H
