/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "MDLoggerCSV.h"
#include <QDebug>
#include <QTextStream>
#include <QTimer>
#include <QStringList>

MDLoggerCSV::MDLoggerCSV(QObject* parent)
    : ICommunication("MDLogger-CSV", parent)
    , m_stream(nullptr)
    , m_parsing(false)
    , m_paused(false)
    , m_playbackSpeed(1.0)
    , m_rowCount(0)
    , m_headerParsed(false)
{
    // Single-shot timer: interval between rows is derived from the CSV
    // timestamp column so playback matches the original logging cadence
    // (divided by m_playbackSpeed).
    m_parseTimer = new QTimer(this);
    m_parseTimer->setSingleShot(true);
    connect(m_parseTimer, &QTimer::timeout, this, &MDLoggerCSV::pumpNextLine);
}

bool MDLoggerCSV::openFile(const QString& filePath) {
    closeFile();
    
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Cannot open file: " + filePath);
        return false;
    }

    m_filePath = filePath;
    m_stream = new QTextStream(&m_file);
    // Qt6: QTextStream defaults to UTF-8, no codec needed
    
    m_rowCount = 0;
    m_headerParsed = false;
    m_parsing = true;
    m_paused = false;
    m_pendingLine.clear();
    m_lastTimestampMs = -1;

    emit connectionStateChanged(true);
    qDebug() << "[MDLogger-CSV] Opened:" << filePath;
    return true;
}

bool MDLoggerCSV::open() {
    // CSV parser requires a file path - this is called by ICommunication interface
    // For now, just report error since we need a file path
    emit errorOccurred("CSV parser requires openFile(filePath) instead of open()");
    return false;
}

void MDLoggerCSV::close() {
    closeFile();
}

void MDLoggerCSV::closeFile() {
    if (m_stream) {
        delete m_stream;
        m_stream = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_parsing = false;
    m_paused = false;
    m_parseTimer->stop();
    m_pendingLine.clear();
    m_lastTimestampMs = -1;
    
    emit connectionStateChanged(false);
    qDebug() << "[MDLogger-CSV] Closed:" << m_filePath;
}

void MDLoggerCSV::parseLine(const QString& line) {
    if (line.startsWith("timestamp")) {
        parseHeader(line);
    } else {
        parseDataLine(line);
    }
}

void MDLoggerCSV::parseHeader(const QString& line) {
    // Header: timestamp,rpm,boost,egt1,egt2,egt3,egt4,egt5,egt6,egt7,egt8,lambda,throttle,speed,gear,efr_speed,bat,latitude,longitude,speed_kmh,course,sats,fix
    m_headerParsed = true;
    qDebug() << "[MDLogger-CSV] Header parsed:" << line;
}

void MDLoggerCSV::parseDataLine(const QString& line) {
    // CSV format (23 columns, written by MDLogger v1.0.0):
    // timestamp,rpm,boost,egt1..egt8,lambda,throttle,speed,gear,efr_speed,bat,
    // latitude,longitude,speed_kmh,course,sats,fix
    //
    // MDLogger PID masking can leave fields EMPTY (consecutive commas), so we
    // must keep empty parts to preserve column indexing and skip blank fields.
    // Most values are pre-scaled at write time EXCEPT:
    //   latitude/longitude : raw int × 1e6  -> divide by 1e6 (degrees)
    //   speed_kmh          : raw × 100      -> divide by 100 (km/h)
    //   course             : raw × 100      -> divide by 100 (degrees)
    
    const QStringList fields = line.split(',', Qt::KeepEmptyParts);
    if (fields.size() < 23) {
        qWarning() << "[MDLogger-CSV] Invalid row, got" << fields.size() << "fields (expected 23)";
        return;
    }

    auto fieldAt = [&fields](int idx, bool& ok) -> double {
        const QString s = fields[idx].trimmed();
        if (s.isEmpty()) { ok = false; return 0.0; }
        return s.toDouble(&ok);
    };

    QHash<DataPointID, double> values;
    bool ok = false;
    double v = 0.0;

    // Field 0: timestamp = millis() uptime; drives playback pacing
    v = fieldAt(0, ok);
    if (ok) m_lastTimestampMs = v;

    // Engine data (pre-scaled at write)
    static const struct { int idx; DataPointID id; } kEngineMap[] = {
        { 1,  DataPointID::RPM },
        { 2,  DataPointID::BoostPressure },
        { 3,  DataPointID::EGT1 },
        { 4,  DataPointID::EGT2 },
        { 5,  DataPointID::EGT3 },
        { 6,  DataPointID::EGT4 },
        { 7,  DataPointID::EGT5 },
        { 8,  DataPointID::EGT6 },
        { 9,  DataPointID::EGT7 },
        { 10, DataPointID::EGT8 },
        { 11, DataPointID::Lambda },
        { 12, DataPointID::ThrottlePosition },
        { 13, DataPointID::VehicleSpeed },
        { 14, DataPointID::Gear },
        { 15, DataPointID::EFRSpeed },
        { 16, DataPointID::BatteryVoltage },
    };
    for (const auto& m : kEngineMap) {
        v = fieldAt(m.idx, ok);
        if (ok) values[m.id] = v;
    }

    // GPS data — lat/lon stored as int × 1e6; both 0 means no fix yet
    bool latOk = false, lonOk = false;
    const double lat = fieldAt(17, latOk);
    const double lon = fieldAt(18, lonOk);
    if (latOk && lonOk && !(lat == 0.0 && lon == 0.0)) {
        values[DataPointID::GPSLatitude]  = lat / 1e6;
        values[DataPointID::GPSLongitude] = lon / 1e6;
        
        v = fieldAt(19, ok);                       // speed_kmh × 100
        if (ok) values[DataPointID::GPSSpeed] = v / 100.0;
        
        v = fieldAt(20, ok);                       // course × 100
        if (ok) values[DataPointID::GPSCourse] = v / 100.0;
        
        v = fieldAt(21, ok);
        if (ok) values[DataPointID::GPSSatellites] = v;
        
        v = fieldAt(22, ok);
        if (ok) values[DataPointID::GPSFixQuality] = v;
    }

    if (values.isEmpty()) {
        return; // fully-masked row, nothing to report
    }

    m_rowCount++;
    emit framesParsed(values, "MDLogger-CSV");
    emit lineParsed(values);
}

void MDLoggerCSV::pumpNextLine() {
    if (m_paused || !m_stream || !m_parsing) {
        return;
    }

    // Emit the buffered line (its scheduled time has arrived)
    if (!m_pendingLine.isEmpty()) {
        parseLine(m_pendingLine);
        m_pendingLine.clear();
    }

    // Buffer the next non-empty line and schedule it from the timestamp delta
    QString next;
    while (!m_stream->atEnd()) {
        next = m_stream->readLine().trimmed();
        if (!next.isEmpty()) break;
        next.clear();
    }

    if (next.isEmpty()) {
        m_parsing = false;
        m_parseTimer->stop();
        qDebug() << "[MDLogger-CSV] Playback complete," << m_rowCount << "rows";
        emit parsingComplete();
        return;
    }

    m_pendingLine = next;

    int intervalMs = 10; // fallback when timestamps are missing/invalid
    bool ok = false;
    const double nextTs = next.section(',', 0, 0).trimmed().toDouble(&ok);
    if (ok && m_lastTimestampMs >= 0 && nextTs > m_lastTimestampMs) {
        const double speed = (m_playbackSpeed > 0.0) ? m_playbackSpeed : 1.0;
        intervalMs = qBound(0, int((nextTs - m_lastTimestampMs) / speed), 5000);
    }
    m_parseTimer->start(intervalMs);
}

void MDLoggerCSV::startParsing() {
    if (!m_stream || !m_file.isOpen()) {
        emit parsingError("No file open");
        return;
    }

    m_parsing = true;
    m_paused = false;
    
    // Read and parse header first
    if (!m_headerParsed) {
        QString header = m_stream->readLine().trimmed();
        parseLine(header); // routes to parseHeader() or parseDataLine() for headerless files
    }
    
    m_parseTimer->start(0); // pump the first data line immediately
    qDebug() << "[MDLogger-CSV] Started parsing at" << m_playbackSpeed << "x speed";
}

void MDLoggerCSV::togglePause() {
    m_paused = !m_paused;
    if (m_paused) {
        m_parseTimer->stop();
        qDebug() << "[MDLogger-CSV] Paused";
    } else {
        m_parseTimer->start(0);
        qDebug() << "[MDLogger-CSV] Resumed";
    }
}
