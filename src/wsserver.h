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

#include <QWebSocketServer>
#include <QWebSocket>
#include <QString>
#include <QRunnable>
#include <atomic>

namespace deathtrap
{
    class WSServer;

    class DatabaseWorkerSignalEmitter : public QObject
    {
        Q_OBJECT
    public:
        DatabaseWorkerSignalEmitter(QObject* parent) : QObject{parent} {
            setObjectName("emitter"); //TODO: document this shit
        }
        virtual ~DatabaseWorkerSignalEmitter();

    signals:
        void finished(QWebSocket* targetSocket, const QByteArray& result);
    };

    class DatabaseWorker : public QRunnable
    {
    public:
        DatabaseWorker(WSServer* server, QWebSocket* targetSocket, const QByteArray&& incomingMessage) : m_server(server), m_targetSocket(targetSocket), m_incomingMessage(incomingMessage) {}
        virtual void run() override;

    private:
        WSServer* m_server;
        QWebSocket* m_targetSocket;
        QByteArray m_incomingMessage;
    };

    class WSServer : public QWebSocketServer
    {
        Q_OBJECT

    public:
        WSServer(const QString& dbFolderPath, const QString& serverName, QWebSocketServer::SslMode secureMode, QObject *parent = nullptr);
        virtual ~WSServer();
        friend class DatabaseWorker;

    public slots:
        void deliverReply(QWebSocket* targetSocket, const QByteArray& reply);

    private slots:
        void handleNewConnection();
        void handleBinaryMessage(const QByteArray& message);
        void handleSocketDisconnected();

    private:
        QString m_dbFolderPath;
        QSet<QWebSocket*> m_sockets;
        static std::atomic_int m_connectionCounter;
    };
}
