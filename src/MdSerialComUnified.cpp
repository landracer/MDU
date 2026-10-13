/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdSerialComUnified.cpp
 * @brief MultiDisplay ECU serial transport — thin wrapper around MdFrameDecoder.
 *
 * All protocol logic lives in the pure, unit-tested MdFrameDecoder; this class
 * only owns the QSerialPort and forwards bytes into the decoder.
 */
#include "MdSerialComUnified.h"
#include <QDebug>

MdSerialComUnified::MdSerialComUnified(const QString& portName, QObject* parent)
    : ICommunication("MD-ECU", parent)
    , m_portName(portName)
    , m_serial(new QSerialPort(this))
    , m_baudRate(115200)
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

    // Serial data arrives in arbitrary chunks; the decoder carries frame state
    // across calls (frames may span reads, several frames may share one chunk).
    int loggedControl = -1;
    for (int i = 0; i < data.size(); i++) {
        QHash<DataPointID, double> values;
        if (m_decoder.feedByte(static_cast<quint8>(data.at(i)), values)) {
            if (!values.isEmpty()) {
                emit framesParsed(values, "MD-ECU");
                emit frameParsed(values);
            }
        } else if (m_decoder.controlCount() != loggedControl) {
            // A control/config frame completed — rate-capped diagnostic log
            loggedControl = m_decoder.controlCount();
            qDebug() << "[MD-ECU] Control frame tag=0x"
                     << QString("%1").arg(m_decoder.lastTag(), 2, 16, QChar('0'))
                     << "received (decode pending, see A-class.md roadmap)";
        }
    }
}