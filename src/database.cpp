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

#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>
#include <QFile>
#include <QVariant>
#include <QSqlError>
#include <QDebug>

constexpr int SQLITE_BUSY_TIMEOUT = 1000 * 60 * 60 * 24 * 7; //A week of timeout should really be enough

bool deathtrap::open(const QString& name, const QString &dbFolderPath)
{
    if(!QDir(dbFolderPath).exists()) {
        return false;
    }
    if(!QFile::exists(dbFolderPath + "/client.db")) {
        return false;
    }
    if(!QFile::exists(dbFolderPath + "/client.mappings.db")) {
        return false;
    }
    if(!QFile::exists(dbFolderPath + "/client.master.db")) {
        return false;
    }
    if(!QFile::exists(dbFolderPath + "/client.caches.db")) {
        return false;
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=" + QString::number(SQLITE_BUSY_TIMEOUT));
    db.setDatabaseName(dbFolderPath+"/client.db");
    if(!db.open()) return false;
    QSqlQuery attachQuery{QSqlDatabase::database(name)};
    attachQuery.prepare("attach database ? as external_caches");
    attachQuery.addBindValue(dbFolderPath + "/client.caches.db");
    if(!attachQuery.exec())
        qDebug() << attachQuery.lastError();
    attachQuery.prepare("attach database ? as external_mappings");
    attachQuery.addBindValue(dbFolderPath + "/client.mappings.db");
    if(!attachQuery.exec())
        qDebug() << attachQuery.lastError();
    attachQuery.prepare("attach database ? as external_master");
    attachQuery.addBindValue(dbFolderPath + "/client.master.db");
    if(!attachQuery.exec())
        qDebug() << attachQuery.lastError();

    return true;
}

void deathtrap::close(const QString& name)
{
    QSqlDatabase::database(name).close();
    QSqlDatabase::removeDatabase(name);
}

int deathtrap::getVersion(const QString& name)
{
    QSqlQuery q{"select * from version", QSqlDatabase::database(name)};

    if(!q.exec()) return -1;
    q.next();

    return q.value(0).toInt();
}

QSet<int> deathtrap::supported_versions()
{
    return {419};
}

std::pair<QStringList, QStringList> deathtrap::getFileLocations(const QString &name)
{
    std::pair<QStringList, QStringList> result;

    //file locations first, then thumbs
    QSqlQuery q{"select location from client_files_locations order by prefix", QSqlDatabase::database(name)};

    if(!q.exec()) return {};

    for(int i = 0; i < 256; i++)
    {
        if(!q.next()) break;
        result.first.append(q.value(0).toString());
    }

    for(int i = 0; i < 256; i++)
    {
        if(!q.next()) break;
        result.second.append(q.value(0).toString());
    }

    if(result.second.length() != 256 || result.first.length() != 256) return {};

    return result;
}
