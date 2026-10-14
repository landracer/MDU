/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file SessionLog.cpp
 * @brief Binary session recording/replay format — see SessionLog.h for layout.
 */
#include "SessionLog.h"
#include <QDataStream>
#include <QFile>
#include <QJsonArray>
#include <QtEndian>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr char kMagicHeader[4] = { 'M', 'D', 'S', 'N' }; // file header
constexpr char kMagicBlock[4]  = { 'M', 'S', 'B', 'K' }; // sample block
constexpr char kMagicEnd[4]    = { 'M', 'S', 'E', 'N' }; // footer
QDataStream::ByteOrder kOrder = QDataStream::LittleEndian;

} // namespace

SessionLog::~SessionLog() {
    if (m_file) { m_file->close(); m_file->deleteLater(); m_file = nullptr; }
}

bool SessionLog::create(const QString& path, const QDateTime& start, quint32 blockCapacity) {
    if (m_file) return false;                       // already recording
    if (blockCapacity == 0) return false;
    QByteArray hdr;
    {
        QDataStream ds(&hdr, QIODevice::WriteOnly);
        ds.setByteOrder(kOrder);
        ds.writeRawData(kMagicHeader, 4);
        ds << static_cast<quint16>(kVersion) << static_cast<quint16>(0)
           << start.toMSecsSinceEpoch() << blockCapacity << static_cast<quint32>(0);
    }
    auto* f = new QFile(path);
    if (!f->open(QIODevice::WriteOnly | QIODevice::Truncate)) { delete f; return false; }
    if (f->write(hdr) != hdr.size() || !f->flush()) { f->close(); f->deleteLater(); return false; }

    m_file = f;
    m_start = start;
    m_blockCapacity = blockCapacity;
    m_blockCount = 0;
    m_sampleCount = 0;
    m_buffer.clear();
    m_index.clear();
    return true;
}

bool SessionLog::append(DataPointID id, double value, const QDateTime& ts) {
    if (!m_file) return false;
    m_buffer.push_back(SessionSample{ id, value, m_start.msecsTo(ts) * 1000LL });
    if (static_cast<quint32>(m_buffer.size()) >= m_blockCapacity) return flush();
    return true;
}

bool SessionLog::flush() {
    if (!m_file) return false;
    if (m_buffer.isEmpty()) return true;

    const qint64 blockOffset = m_file->pos();   // exact: all prior writes are flushed
    const qint64 t0 = m_buffer.first().relUs;   // capture BEFORE the buffer is cleared
    const quint32 count = static_cast<quint32>(m_buffer.size());
    QByteArray blk;
    {
        QDataStream ds(&blk, QIODevice::WriteOnly);
        ds.setByteOrder(kOrder);
        ds.writeRawData(kMagicBlock, 4);
        ds << t0 << count;
        for (const SessionSample& s : std::as_const(m_buffer))
            ds << s.relUs << static_cast<quint32>(s.id) << s.value;
    }
    if (m_file->write(blk) != blk.size() || !m_file->flush()) return false;

    m_sampleCount += count;
    m_index.append(SessionFile::BlockMeta{ t0, blockOffset });
    ++m_blockCount;
    m_buffer.clear();
    return true;
}

bool SessionLog::finalize() {
    if (!m_file) return false;
    if (!flush()) return false;

    const qint64 indexOffset = m_file->pos();
    QByteArray tail;
    {
        QDataStream ds(&tail, QIODevice::WriteOnly);
        ds.setByteOrder(kOrder);
        for (const SessionFile::BlockMeta& b : std::as_const(m_index))
            ds << b.t0RelUs << b.offset;
        ds.writeRawData(kMagicEnd, 4);
        ds << static_cast<quint16>(kVersion) << static_cast<quint16>(0)
           << m_blockCount << static_cast<quint64>(m_sampleCount) << indexOffset;
    }
    const bool ok = m_file->write(tail) == tail.size() && m_file->flush();
    m_file->close();
    m_file->deleteLater();
    m_file = nullptr;
    m_buffer.clear();
    return ok;
}

bool SessionLog::read(const QString& path, SessionFile* out, QString* error) {
    auto fail = [&](const QString& m) { if (error) *error = m; return false; };
    if (!out) return fail(QStringLiteral("no output"));
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return fail(QStringLiteral("cannot open %1").arg(path));
    const QByteArray all = f.readAll();
    f.close();
    const qsizetype kFooter = 4 + 2 + 2 + 4 + 8 + 8;   // = 28 bytes, fixed
    if (all.size() < 24 + kFooter || !all.startsWith(QByteArray(kMagicHeader, 4)))
        return fail(QStringLiteral("bad magic/short file"));
    if (qFromLittleEndian<quint16>(all.constData() + 4) != kVersion)
        return fail(QStringLiteral("unsupported version"));
    const qint64 startMs = qFromLittleEndian<qint64>(all.constData() + 8);

    // Footer first — it marks where the block region ends (indexOff).
    const qsizetype fs = all.size() - kFooter;
    if (all.mid(fs, 4) != QByteArray(kMagicEnd, 4)) return fail(QStringLiteral("missing footer magic"));
    if (qFromLittleEndian<quint16>(all.constData() + fs + 4) != kVersion)
        return fail(QStringLiteral("footer version mismatch"));
    const quint32 blocks  = qFromLittleEndian<quint32>(all.constData() + fs + 8);
    const quint64 samples = qFromLittleEndian<quint64>(all.constData() + fs + 12);
    const qint64 indexOff = qFromLittleEndian<qint64>(all.constData() + fs + 20);
    if (indexOff < 24 || indexOff > fs) return fail(QStringLiteral("bogus index offset"));

    SessionFile file;
    file.startTime = QDateTime::fromMSecsSinceEpoch(startMs, Qt::UTC);
    file.blockCapacity = qFromLittleEndian<quint32>(all.constData() + 16);

    const QByteArray wantBlock(kMagicBlock, 4);
    qint64 pos = 24;
    while (pos < indexOff) {
        if (pos + 16 > indexOff || all.mid(pos, 4) != wantBlock)
            return fail(QStringLiteral("unknown tag (corrupt stream)"));
        const qint64 t0 = qFromLittleEndian<qint64>(all.constData() + pos + 4);
        const quint32 count = qFromLittleEndian<quint32>(all.constData() + pos + 12);
        if (count == 0 || pos + 16 + qint64(count) * 20 > indexOff)
            return fail(QStringLiteral("bad block header"));
        file.index.append(SessionFile::BlockMeta{ t0, pos });
        pos += 16;
        for (quint32 i = 0; i < count; ++i) {
            const qint64 relUs = qFromLittleEndian<qint64>(all.constData() + pos);
            const quint32 rawId = qFromLittleEndian<quint32>(all.constData() + pos + 8);
            const double value = qFromLittleEndian<double>(all.constData() + pos + 12);
            file.samples.append(SessionSample{ static_cast<DataPointID>(rawId), value, relUs });
            pos += 20;
        }
    }
    if (static_cast<qint64>(samples) != file.samples.size()
        || static_cast<quint64>(file.index.size()) != blocks
        || fs - indexOff != qint64(file.index.size()) * 16)
        return fail(QStringLiteral("index/count mismatch"));
    *out = file;
    return true;
}

QVector<SessionSample> SessionLog::slice(const SessionFile& f, qint64 fromRelUs, qint64 toRelUs)
{
    // Samples are stored chronologically — binary-search the first sample at or
    // after fromRelUs, then collect until past toRelUs.
    QVector<SessionSample> out;
    int lo = 0, hi = f.samples.size() - 1, first = f.samples.size();
    while (lo <= hi) {
        const int mid = (lo + hi) / 2;
        if (f.samples[mid].relUs >= fromRelUs) { first = mid; hi = mid - 1; }
        else lo = mid + 1;
    }
    for (int i = first; i < f.samples.size() && f.samples[i].relUs <= toRelUs; ++i)
        out.append(f.samples[i]);
    return out;
}

QString SessionLog::toCsv(const SessionFile& f)
{
    QString out = QStringLiteral("# MDU session log — recorded %1\n"
                                 "# relUs,dpId,dpName,unit,value\n")
                      .arg(f.startTime.toString(Qt::ISODate));
    for (const SessionSample& s : f.samples)
        out += QStringLiteral("%1,0x%2,%3,%4,%5\n")
                   .arg(s.relUs)
                   .arg(static_cast<uint32_t>(s.id), 4, 16, QChar('0'))
                   .arg(DataPointName(s.id), DataPointUnit(s.id),
                        QString::number(s.value, 'g', 17));
    return out;
}

QByteArray SessionLog::toJson(const SessionFile& f)
{
    QJsonArray arr;
    for (const SessionSample& s : f.samples) {
        QJsonObject o;
        o.insert(QStringLiteral("id"),
                 QStringLiteral("0x%1").arg(static_cast<uint32_t>(s.id), 4, 16, QChar('0')));
        o.insert(QStringLiteral("name"), DataPointName(s.id));
        o.insert(QStringLiteral("unit"), DataPointUnit(s.id));
        o.insert(QStringLiteral("relUs"), static_cast<double>(s.relUs));
        o.insert(QStringLiteral("value"), s.value);
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("start"), f.startTime.toString(Qt::ISODate));
    root.insert(QStringLiteral("samples"), arr);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
