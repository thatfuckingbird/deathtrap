/*
hyfileaccess
Copyright (C) 2020-2021  Hydroid developers  <hydroid@rbx.run>

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
#include <QCommandLineParser>
#include <QTextStream>
#include <QDir>
#include <QMimeDatabase>
#include "hyfileaccess/database.h"
extern "C" {
#include "3rdparty/mongoose/mongoose.h"
}

static const char* s_debug_level = "2";

QStringList thumbPaths;
QStringList filePaths;
QString listeningAddress;
QHash<QString, std::pair<QString, QMimeType>> thumbExtensionCache;
QHash<QString, std::pair<QString, QMimeType>> fileExtensionCache;

void updateExtensionCache(const QString& path, QHash<QString, std::pair<QString, QMimeType>>& cache)
{
    QDir dir{path};
    QMimeDatabase mimeDb;
    for(const auto& entry: dir.entryInfoList(QDir::Files)) {
        cache[entry.baseName()] = {entry.completeSuffix(), mimeDb.mimeTypeForFile(entry)};
    }
}

static std::pair<QString, QString> getPathAndMime(const QString& uri)
{
    QString path, hash;
    std::pair<QString, QMimeType> cachedData;
    if(uri.startsWith("/thumb/by-hash/")) {
        hash = uri.mid(15).left(64);
        if(hash.length() != 64) return {};
        const int index = hash.leftRef(2).toInt(nullptr, 16);
        path = thumbPaths[index];
        cachedData = thumbExtensionCache.value(hash);
        if(cachedData.first.isEmpty()) {
            updateExtensionCache(path, thumbExtensionCache);
        }
        cachedData = thumbExtensionCache.value(hash);
    } else if(uri.startsWith("/file/by-hash/")) {
        hash = uri.mid(16).left(64);
        if(hash.length() != 64) return {};
        const int index = hash.leftRef(2).toInt(nullptr, 16);
        path = filePaths[index];
        cachedData = fileExtensionCache.value(hash);
        if(cachedData.first.isEmpty()) {
            updateExtensionCache(path, fileExtensionCache);
        }
        cachedData = thumbExtensionCache.value(hash);
    }
    if(cachedData.first.isEmpty() || path.isEmpty()) return {};

    return {path + "/" + hash + "." + cachedData.first, cachedData.second.name()};
}

static void callback(struct mg_connection* c, int ev, void* ev_data, [[maybe_unused]] void* fn_data)
{
    if(ev == MG_EV_ACCEPT && mg_url_is_ssl(listeningAddress.toStdString().data())) {
        struct mg_tls_opts opts = {
          //.ca = "ca.pem",         // Uncomment to enable two-way SSL
          .cert = "server.pem", // Certificate PEM file
          .certkey = "server.pem", // This pem conains both cert and key
        };
        mg_tls_init(c, &opts);
    } else if(ev == MG_EV_HTTP_MSG) {
        auto http_data = static_cast<mg_http_message*>(ev_data);
        auto [path, mime] = getPathAndMime(QString::fromUtf8(http_data->uri.ptr, http_data->uri.len));
        if(path.isEmpty()) {
            mg_http_reply(c, 404, "", "%s", "Not found\n");
        } else {
            mg_http_serve_file(c, http_data, path.toStdString().data(), mime.toStdString().data(), nullptr);
        }
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app{argc, argv};

    QCommandLineParser cmdLineParser;
    QCommandLineOption hydrusDbPathOption{QStringList{} << "db-path"
                                                        << "d",
                                          "Hydrus database path.", "path"};
    cmdLineParser.addOption(hydrusDbPathOption);
    QCommandLineOption listeningAddressOption{QStringList{} << "address"
                                                            << "a",
                                              "Listening address.", "address"};
    cmdLineParser.addOption(listeningAddressOption);
    cmdLineParser.process(app);

    QTextStream err{stderr};

    if(!hyfileaccess::open("db", cmdLineParser.value(hydrusDbPathOption))) {
        err << "Failed to open Hydrus database at: " << cmdLineParser.value(hydrusDbPathOption) << Qt::endl;
        return EXIT_FAILURE;
    }
    auto [tmpThumbPaths, tmpFilePaths] = hyfileaccess::getFileLocations("db");
    thumbPaths = std::move(tmpThumbPaths);
    filePaths = std::move(tmpFilePaths);
    hyfileaccess::close("db");

    if(thumbPaths.length() != 256 || thumbPaths.length() != filePaths.length()) {
        err << "Missing file or thumbnail paths";
        return EXIT_FAILURE;
    }

    for(const auto& thumbPath: std::as_const(thumbPaths)) {
        updateExtensionCache(thumbPath, thumbExtensionCache);
    }
    for(const auto& filePath: std::as_const(filePaths)) {
        updateExtensionCache(filePath, fileExtensionCache);
    }

    listeningAddress = cmdLineParser.value(listeningAddressOption);

    mg_mgr mgr;
    mg_log_set(s_debug_level);
    mg_mgr_init(&mgr);
    if(mg_http_listen(&mgr, listeningAddress.toStdString().data(), callback, &mgr) == NULL) {
        err << "Failed to listen on address: " << listeningAddress << Qt::endl;
        return EXIT_FAILURE;
    }

    for(;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);

    return EXIT_SUCCESS;
}
