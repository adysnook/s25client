// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameManagerServer.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "RttrConfig.h"
#include "SettingsServer.h"
//#include "WindowManager.h"
//#include "desktops/dskLobby.h"
//#include "desktops/dskMainMenu.h"
//#include "desktops/dskSplash.h"
//#include "drivers/AudioDriverWrapper.h"
//#include "drivers/VideoDriverWrapper.h"
#include "files.h"
//#include "network/GameClient.h"
#include "network/GameServer.h"
//#include "ogl/glArchivItem_Bitmap.h"
//#include "liblobby/LobbyClient.h"
#include "libsiedler2/Archiv.h"
#include "s25util/Log.h"
#include "s25util/error.h"
#include "RTTR_Version.h"
#include "s25util/StringConversion.h"
//#include "controls/ctrlChat.h"
#include "s25util/MyTime.h"
#include "QuickStartGameServer.h"
#include "s25util/md5.hpp"
#include <chrono>
#include <boost/pointer_cast.hpp>

GameManagerServer::GameManagerServer(Log& log, SettingsServer& settings)
    : log_(log), settings_(settings)
{
    ResetAverageGFPS();
}

/**
 *  Spiel starten
 */
bool GameManagerServer::Start()
{
    // Einstellungen laden
    settings_.Load();

    // Treibereinstellungen abspeichern
    settings_.Save();

    return true;
}

/**
 *  Spiel beenden.
 */
void GameManagerServer::Stop()
{
    //GAMECLIENT.Stop();
    GAMESERVER.Stop();
    //LOBBYCLIENT.Stop();
    // Global Einstellungen speichern
    //settings_.Save();
}

/**
 *  Hauptschleife.
 */
bool GameManagerServer::Run()
{  
    //GAMECLIENT.Run();
    GAMESERVER.Run();

    gfCounter_.update();

    return GLOBALVARS.notdone;
}

void GameManagerServer::ResetAverageGFPS()
{
    gfCounter_ = FrameCounter(FrameCounter::clock::duration::max()); // Never update
}

static GameManagerServer* globalGameManagerServer = nullptr;

GameManagerServer& getGlobalGameManagerServer()
{
    return *globalGameManagerServer;
}
void setGlobalGameManagerServer(GameManagerServer* gameManagerServer)
{
    globalGameManagerServer = gameManagerServer;
}
/*
void GameManagerServer::ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass)
{

    // Potential false positive: User uses new password which is equal to the hash of the old one. HIGHLY unlikely, so
    // ignore

    constexpr auto md5HashLen = 32;

    SETTINGS_SERVER.Load();

    user = SETTINGS_SERVER.lobby.name;
    pass = SETTINGS_SERVER.lobby.password;

    if(pass == "")
    {
        return;
    }

    //if not md5 format
    if (!(pass.size() == md5HashLen + 4 && pass.substr(0, 4) == "md5:"))
    {
        pass = "md5:" + s25util::md5(pass).toString();
        SETTINGS_SERVER.lobby.password = pass;
        SETTINGS_SERVER.Save();
    }
    pass = SETTINGS_SERVER.lobby.password.substr(4);           
}
*/
