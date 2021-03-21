/*
deathtrap
Copyright (C) 2020  Hydroid developers  <hydroid@rbx.run>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "wsserver.h"
#include "wsmessage.h"
#include "database.h"
#include <QCborStreamReader>
#include <QCborStreamWriter>
#include <QCborValue>
#include <QThreadPool>

std::atomic_int deathtrap::WSServer::m_connectionCounter = 0;

deathtrap::WSServer::WSServer(const QString& dbFolderPath, const QString& serverName, QWebSocketServer::SslMode secureMode, QObject *parent) : QWebSocketServer{serverName, secureMode, parent}, m_dbFolderPath(dbFolderPath)
{
    QThreadPool::globalInstance()->setExpiryTimeout(-1);
    connect(this, &WSServer::newConnection, this, &WSServer::handleNewConnection);
}

deathtrap::WSServer::~WSServer()
{
    for(QWebSocket* socket : std::as_const(m_sockets))
    {
        socket->close();
        deathtrap::close(socket->property("dbName").toString());
        socket->deleteLater();
    }
}

void deathtrap::WSServer::deliverReply(QWebSocket *targetSocket, const QByteArray &reply)
{
    if(m_sockets.contains(targetSocket))
    {
        if(targetSocket->sendBinaryMessage(reply) != reply.size())
        {
            qWarning() << "Failed to write data to socket";
        }
    }
}

void deathtrap::WSServer::handleNewConnection()
{
    QWebSocket* socket = nextPendingConnection();
    m_sockets.insert(socket);
    connect(socket, &QWebSocket::binaryMessageReceived, this, &WSServer::handleBinaryMessage);
    connect(socket, &QWebSocket::disconnected, this, &WSServer::handleSocketDisconnected);
}

void deathtrap::WSServer::handleBinaryMessage(const QByteArray &message)
{
    QWebSocket* socket = dynamic_cast<QWebSocket*>(QObject::sender());
    QRunnable* dbWorker = new DatabaseWorker{this, socket, message};
    QThreadPool::globalInstance()->start(dbWorker);
}

void deathtrap::WSServer::handleSocketDisconnected()
{
    QWebSocket* sender = dynamic_cast<QWebSocket*>(QObject::sender());
    m_sockets.remove(sender);
    sender->close();
    deathtrap::close(sender->property("dbName").toString());
    sender->deleteLater();
}

//TODO: document this shit
void deathtrap::DatabaseWorker::run()
{
    QThread* currentThread = QThread::currentThread();

    DatabaseWorkerSignalEmitter* emitter = currentThread->findChild<DatabaseWorkerSignalEmitter*>("emitter", Qt::FindDirectChildrenOnly);
    if(!emitter)
    {
        emitter = new DatabaseWorkerSignalEmitter{currentThread};
    }
    QObject::connect(emitter, &DatabaseWorkerSignalEmitter::finished, m_server, &WSServer::deliverReply, Qt::UniqueConnection);

    bool needToOpenDB = false;
    if(!currentThread->property("dbName").isValid())
    {
        currentThread->setProperty("dbName", "wsserver_db_" + QString::number(WSServer::m_connectionCounter++));
        needToOpenDB = true;
    }
    QString dbName = currentThread->property("dbName").toString();
    if(needToOpenDB)
    {
        if(!deathtrap::open(dbName, m_server->m_dbFolderPath))
        {
            //TODO report error and/or kill socket
        }
    }

    QCborStreamReader reader{m_incomingMessage};
    deathtrap::MessageType msgType = deathtrap::MessageType::Invalid;
    quint64 msgId = 0;

    if(reader.next())
    {
        if(reader.type() != QCborStreamReader::UnsignedInteger || reader.toUnsignedInteger() != NETWORK_PROTOCOL_VERSION)
        {
            qWarning() << "Invalid or missing protocol version";
            return;
        }
    } else
    {
        qWarning() << "Empty message";
        return;
    }

    if(reader.next() && reader.type() == QCborStreamReader::UnsignedInteger)
    {
        msgId = reader.toUnsignedInteger();
    } else
    {
        qWarning() << "Missing message ID";
        return;
    }

    if(reader.next() && reader.type() == QCborStreamReader::UnsignedInteger)
    {
        msgType = static_cast<deathtrap::MessageType>(reader.toUnsignedInteger());
    } else
    {
        qWarning() << "Missing message type";
        return;
    }

    QByteArray result;
    QCborStreamWriter writer(&result);
    writer.append(NETWORK_PROTOCOL_VERSION);
    writer.append(msgId);
    writer.append(static_cast<quint64>(msgType));

    switch(msgType)
    {
    case deathtrap::MessageType::GetVersion:
        writer.append(deathtrap::getVersion(dbName));
        break;
    case deathtrap::MessageType::GetFileLocations:
    {
        const auto locs = deathtrap::getFileLocations(dbName);
        writer.startArray();
        for(const auto& loc : locs.first)
        {
            writer.append(loc);
        }
        writer.endArray();
        writer.startArray();
        for(const auto& loc : locs.second)
        {
            writer.append(loc);
        }
        writer.endArray();
        break;
    }
    case deathtrap::MessageType::IntegrityCheck:
    {
        bool quick = false;
        if(reader.next() && reader.type() == QCborStreamReader::SimpleType)
        {
            quick = reader.toSimpleType() == QCborSimpleType::True;
        }
        const auto res = deathtrap::maintenance::integrityCheck(dbName, quick);
        writer.startArray();
        for(const auto& str: res)
        {
            writer.append(str);
        }
        writer.endArray();
        break;
    }
    case deathtrap::MessageType::Analyze:
        writer.append(deathtrap::maintenance::analyze(dbName));
        break;
    case deathtrap::MessageType::Optimize:
        writer.append(deathtrap::maintenance::optimize(dbName));
        break;
    case deathtrap::MessageType::Vacuum:
        writer.append(deathtrap::maintenance::vacuum(dbName));
        break;
    case deathtrap::MessageType::VacuumInto:
    {
        QString targetDir;
        if(reader.next() && reader.type() == QCborStreamReader::String)
        {
            targetDir = readString(reader);//TODO return optional
        }
        if(!targetDir.isEmpty())//TODO: what to write if its empty/invalid?
        {
            writer.append(deathtrap::maintenance::vacuumInto(dbName, targetDir));
        }
        break;
    }
    case deathtrap::MessageType::TryLockDatabase:
        writer.append(deathtrap::maintenance::tryLockDatabase(dbName));
        break;
    case deathtrap::MessageType::ReleaseLockedDatabase:
        writer.append(deathtrap::maintenance::releaseLockedDatabase());
        break;
    default:
        qWarning() << "Invalid message type";
        return;
    }

    emit emitter->finished(m_targetSocket, result);
}

QString deathtrap::DatabaseWorker::readString(QCborStreamReader &reader)
{
    QString result;
    auto r = reader.readString();
    while (r.status == QCborStreamReader::Ok) {
        result += r.data;
        r = reader.readString();
    }

    if (r.status == QCborStreamReader::Error) {
        result.clear();
    }
    return result;
}

//TODO: document this shit
deathtrap::DatabaseWorkerSignalEmitter::~DatabaseWorkerSignalEmitter()
{
    QThread* thread = dynamic_cast<QThread*>(parent());
    if(thread && thread->property("dbName").isValid())
    {
        deathtrap::close(thread->property("dbName").toString());
    }
}
