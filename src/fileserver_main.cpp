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
#include "database.h"
extern "C" { //TODO is this ok
#include "3rdparty/mongoose/mongoose.h"
}
#include <iostream>
#include <vector>

//TODO add SSL cert support
//TODO https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests

static const char *s_debug_level = "2";
static const char *s_root_dir = ".";
static const char *s_listening_address = "http://localhost:8017";

std::vector<std::string> tpaths;
std::vector<std::string> fpaths;

static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
      //TODO serve file here
      auto http_data = static_cast<mg_http_message*>(ev_data);
      std::cout << std::string{http_data->uri.ptr, http_data->uri.len} << std::endl;
  }
  (void) fn_data;
}

int main (int argc, char *argv[])
{
    QCoreApplication app{ argc, argv };
    if(!deathtrap::open("db", app.arguments().at(1)))
    {
        //TODO
    }
    const auto [tmpfpaths, tmptpaths] = deathtrap::getFileLocations("db");
    deathtrap::close("db");

    for(const auto& p : tmpfpaths) {
        fpaths.push_back(p.toStdString());
    }
    for(const auto& p : tmptpaths) {
        tpaths.push_back(p.toStdString());
    }

    //TODO check length is 256

    mg_mgr mgr;
    mg_connection *c = nullptr;
    mg_log_set(s_debug_level);
    mg_mgr_init(&mgr);
    if ((c = mg_http_listen(&mgr, s_listening_address, cb, &mgr)) == NULL)
    {
        LOG(LL_ERROR, ("Cannot listen on %s. Use http://ADDR:PORT or :PORT", s_listening_address));
        exit(EXIT_FAILURE);
    }

    LOG(LL_INFO, ("Starting Mongoose v%s, serving [%s]", MG_VERSION, s_root_dir));
    for (;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);

    return EXIT_SUCCESS;
}
