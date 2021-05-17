// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "QuickStartGameServer.h"
#include "SettingsServer.h"
#include "network/CreateServerInfo.h"
#include "network/GameServer.h"
#include "s25util/Log.h"
#include "s25util/strAlgos.h"
#include "s25util/strFuncs.h"
#include <boost/filesystem.hpp>
#include <mygettext/mygettext.h>

bool QuickStartGameServer(const boost::filesystem::path& map_path, uint16_t port, const std::string owner, bool ipv6)
{
    if(!exists(map_path))
    {
        LOG.write(_("Given map (%1%) does not exist!\n")) % map_path;
        return false;
    }

    std::string passwd = "";
    std::string game_name = "Game for " + owner;
    const CreateServerInfo csi(ServerType::Lobby, port, game_name, passwd, ipv6, SETTINGS_SERVER.global.use_upnp, owner, false, false);

    LOG.write(_("Loading game...\n"));
    const std::string extension = s25util::toLower(map_path.extension().string());

    if(!(extension == ".swd" || extension == ".wld"))
    {
        return false;
    }

    std::string hostPw = createRandString(20);
    //LOG.write("hostPw:%s\n") % hostPw;
    if(!GAMESERVER.Start(csi, map_path, MapType::OldMap, hostPw))
        return false;

	return true;
}
