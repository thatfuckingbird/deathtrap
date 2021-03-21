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

constexpr auto SUPPORTED_VERSIONS = {433};
constexpr auto DB_NAMES = {"main", "external_caches", "external_mappings", "external_master"};
constexpr int SQLITE_BUSY_TIMEOUT = 1000 * 60 * 60 * 24 * 7; //A week of timeout should really be enough
bool currentlyLockedForBackup = false;
QString dbConnectionUsedForLocking;

bool deathtrap::open(const QString& name, const QString &dbFolderPath, PragmaSynchronous synchronous, PragmaJournalMode journalMode)
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
    {
        qDebug() << attachQuery.lastError();
        close(name);
        return false;
    }
    attachQuery.prepare("attach database ? as external_mappings");
    attachQuery.addBindValue(dbFolderPath + "/client.mappings.db");
    if(!attachQuery.exec())
    {
        qDebug() << attachQuery.lastError();
        close(name);
        return false;
    }
    attachQuery.prepare("attach database ? as external_master");
    attachQuery.addBindValue(dbFolderPath + "/client.master.db");
    if(!attachQuery.exec())
    {
        qDebug() << attachQuery.lastError();
        close(name);
        return false;
    }

    for(const auto& prefix: DB_NAMES)
    {
        QSqlQuery syncQuery{QString{"pragma %1.synchronous = %2"}.arg(prefix, QString::number(static_cast<int>(synchronous))), QSqlDatabase::database(name)};
        if(!syncQuery.exec())
        {
            qDebug() << syncQuery.lastError();
            close(name);
            return false;
        }

        QString journalModeKeyword;
        switch (journalMode)
        {
        case PragmaJournalMode::Delete:
            journalModeKeyword = "DELETE";
            break;
        case PragmaJournalMode::Truncate:
            journalModeKeyword = "TRUNCATE";
            break;
        case PragmaJournalMode::Persist:
            journalModeKeyword = "PERSIST";
            break;
        case PragmaJournalMode::Memory:
            journalModeKeyword = "MEMORY";
            break;
        case PragmaJournalMode::WAL:
            journalModeKeyword = "WAL";
            break;
        case PragmaJournalMode::Off:
            journalModeKeyword = "OFF";
            break;
        }
        QSqlQuery journalQuery{QString{"pragma %1.journal_mode = %2"}.arg(prefix, journalModeKeyword), QSqlDatabase::database(name)};
        if(!journalQuery.exec())
        {
            qDebug() << journalQuery.lastError();
            close(name);
            return false;
        }
    }

    return true;
}

void deathtrap::close(const QString& name)
{
    QSqlDatabase::database(name).close();
    QSqlDatabase::removeDatabase(name);
}

int deathtrap::getVersion(const QString& name)
{
    QSqlQuery q{"select version from version", QSqlDatabase::database(name)};
    if(!q.exec()) return -1;
    q.next();

    return q.value(0).toInt();
}

QSet<int> deathtrap::supportedVersions()
{
    return SUPPORTED_VERSIONS;
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

bool deathtrap::maintenance::tryLockDatabase(const QString &name)
{
    if(currentlyLockedForBackup) return false;

    QSqlQuery q{"begin immediate", QSqlDatabase::database(name)};
    if(!q.exec())
    {
        qDebug() << q.lastError();
        return false;
    }

    currentlyLockedForBackup = true;
    dbConnectionUsedForLocking = name;
    return true;
}

bool deathtrap::maintenance::releaseLockedDatabase()
{
    if(!currentlyLockedForBackup) return false;

    QSqlQuery q{"end transaction", QSqlDatabase::database(dbConnectionUsedForLocking)};
    if(!q.exec()) return false;

    currentlyLockedForBackup = false;
    return true;
}

bool deathtrap::maintenance::shrinkMemory(const QString &name)
{
    QSqlQuery q{"pragma shrink_memory", QSqlDatabase::database(name)};
    return q.exec();
}

bool deathtrap::maintenance::setMaxAuxiliaryThreadCount(const QString &name, std::uint16_t count)
{
    QSqlQuery q{QString{"pragma threads = %1"}.arg(QString::number(count)), QSqlDatabase::database(name)};
    return q.exec();
}

QStringList deathtrap::maintenance::integrityCheck(const QString &name, bool quick)
{
    QStringList result;
    QString pragmaName = quick ? "quick_check" : "integrity_check";
    for(const auto& prefix: DB_NAMES)
    {
        QSqlQuery checkQuery{QString{"pragma %1.%2"}.arg(prefix, pragmaName), QSqlDatabase::database(name)};
        if(checkQuery.exec())
        {
            while(checkQuery.next()) result.append(checkQuery.value(0).toString());
        }
    }
    return result;
}

bool deathtrap::maintenance::analyze(const QString &name)
{
    QSqlQuery q{"analyze", QSqlDatabase::database(name)};
    return q.exec();
}

bool deathtrap::maintenance::vacuum(const QString &name)
{
    bool ok = true;
    for(const auto& prefix: DB_NAMES)
    {
        QSqlQuery vacQuery{QString{"vacuum %1"}.arg(prefix), QSqlDatabase::database(name)};
        if(!vacQuery.exec())
        {
            qDebug() << vacQuery.lastError();
            ok = false;
        }
    }
    return ok;
}

bool deathtrap::maintenance::optimize(const QString &name)
{
    QSqlQuery q{"pragma optimize", QSqlDatabase::database(name)};
    return q.exec();
}

bool deathtrap::maintenance::vacuumInto(const QString &name, const QString &targetDirectory)
{
    bool ok = true;
    for(const auto& prefix: DB_NAMES)
    {
        QSqlQuery vacQuery{QString{"vacuum %1 into ?"}.arg(prefix), QSqlDatabase::database(name)};
        vacQuery.addBindValue(targetDirectory + "/client_" + prefix + ".db");
        if(!vacQuery.exec())
        {
            qDebug() << vacQuery.lastError();
            ok = false;
        }
    }
    return ok;
}
