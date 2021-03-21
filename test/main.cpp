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

#include <QCoreApplication>
#include <QDir>
#include "main.h"
#include "database.h"

void DatabaseTests::init()
{
    deathtrap::open("testDB", "test/db");
}

void DatabaseTests::cleanup()
{
    deathtrap::close("testDB");
}

void DatabaseTests::test_getVersion()
{
    QCOMPARE(deathtrap::getVersion("testDB"), 433);
}

void DatabaseTests::test_getFileLocations()
{
    QCOMPARE(deathtrap::getFileLocations("testDB").first.length(), 256);
    QCOMPARE(deathtrap::getFileLocations("testDB").second.length(), 256);
    QCOMPARE(deathtrap::getFileLocations("testDB").first[0], "client_files_alt_location");
    QCOMPARE(deathtrap::getFileLocations("testDB").first[1], "client_files");
}

void DatabaseTests::test_setMaxAuxiliaryThreadCount()
{
    QVERIFY(deathtrap::maintenance::setMaxAuxiliaryThreadCount("testDB", 5));
    QVERIFY(deathtrap::maintenance::setMaxAuxiliaryThreadCount("testDB", 0));
    QVERIFY(deathtrap::maintenance::setMaxAuxiliaryThreadCount("testDB", 5));
}

void DatabaseTests::test_shrinkMemory()
{
    QVERIFY(deathtrap::maintenance::shrinkMemory("testDB"));
}

void DatabaseTests::test_integrityCheck()
{
    QStringList expected = {"ok", "ok", "ok", "ok"};
    QCOMPARE(deathtrap::maintenance::integrityCheck("testDB", false), expected);
    QCOMPARE(deathtrap::maintenance::integrityCheck("testDB", true), expected);
}

void DatabaseTests::test_analyze()
{
    QVERIFY(deathtrap::maintenance::analyze("testDB"));
}

void DatabaseTests::test_optimize()
{
    QVERIFY(deathtrap::maintenance::optimize("testDB"));
}

void DatabaseTests::test_vacuum()
{
    QVERIFY(deathtrap::maintenance::vacuum("testDB"));
}

void DatabaseTests::test_vacuumInto()
{
    const auto dbNames = {"main", "external_caches", "external_mappings", "external_master"};

    for(const auto& name: dbNames)
    {
        QString fName = (QDir::tempPath()+"/client_%1.db").arg(name);
        if(QFile::exists(fName)) QFile::remove(fName);
    }

    QVERIFY(deathtrap::maintenance::vacuumInto("testDB", QDir::tempPath()));

    for(const auto& name: dbNames)
    {
        QString fName = (QDir::tempPath()+"/client_%1.db").arg(name);
        QVERIFY(QFile::exists(fName));
    }

    for(const auto& name: dbNames)
    {
        QString fName = (QDir::tempPath()+"/client_%1.db").arg(name);
        if(QFile::exists(fName)) QFile::remove(fName);
    }
}

void DatabaseTests::test_DatabaseLocking()
{
    QVERIFY(!deathtrap::maintenance::releaseLockedDatabase());
    QVERIFY(deathtrap::maintenance::tryLockDatabase("testDB"));
    QVERIFY(!deathtrap::maintenance::tryLockDatabase("testDB"));
    QVERIFY(deathtrap::maintenance::releaseLockedDatabase());
    QVERIFY(!deathtrap::maintenance::releaseLockedDatabase());
}

QTEST_MAIN(DatabaseTests)
