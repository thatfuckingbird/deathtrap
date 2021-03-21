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

#pragma once

#include <QString>
#include <QSet>

namespace deathtrap
{
    enum class PragmaSynchronous
    {
        Off = 0,
        Normal = 1,
        Full = 2,
        Extra = 3
    };

    enum class PragmaJournalMode
    {
        Delete,
        Truncate,
        Persist,
        Memory,
        WAL,
        Off
    };

    QSet<int> supportedVersions();

    bool open(const QString& name, const QString& dbFolderPath, PragmaSynchronous synchronous = PragmaSynchronous::Full, PragmaJournalMode journalMode = PragmaJournalMode::WAL);
    void close(const QString& name);

    int getVersion(const QString& name);
    std::pair<QStringList, QStringList> getFileLocations(const QString& name);

    namespace maintenance
    {
        bool setMaxAuxiliaryThreadCount(const QString& name, uint16_t count);
        bool shrinkMemory(const QString& name);
        QStringList integrityCheck(const QString& name, bool quick = false);
        bool analyze(const QString& name);
        bool optimize(const QString& name);
        bool vacuum(const QString& name);
        bool vacuumInto(const QString& name, const QString& targetDirectory);
        bool tryLockDatabase(const QString& name);
        bool releaseLockedDatabase();
    }
}
