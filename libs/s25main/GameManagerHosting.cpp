// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameManagerHosting.h"
#include "GlobalVars.h"
#include "Loader.h"
#include "RTTR_Assert.h"
#include "RttrConfig.h"
#include "SettingsHosting.h"
//#include "WindowManager.h"
//#include "desktops/dskLobby.h"
//#include "desktops/dskMainMenu.h"
//#include "desktops/dskSplash.h"
//#include "drivers/AudioDriverWrapper.h"
//#include "drivers/VideoDriverWrapper.h"
#include "files.h"
//#include "network/GameClient.h"
//#include "network/GameServer.h"
//#include "ogl/glArchivItem_Bitmap.h"
#include "RTTR_Version.h"
#include "controls/ctrlChat.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/Archiv.h"
#include "s25util/Log.h"
#include "s25util/MyTime.h"
#include "s25util/StringConversion.h"
#include "s25util/error.h"
//#include "QuickStartGameServer.h"
#include "s25util/md5.hpp"
#include <boost/pointer_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <chrono>
#include <iostream>
#include "HostedServerInfo.h"

bool port_in_use(unsigned short port)
{
    using namespace boost::asio;
    using ip::tcp;

    io_service svc;
    tcp::acceptor a(svc);

    boost::system::error_code ec;
    a.open(tcp::v4(), ec) || a.bind({tcp::v4(), port}, ec);

    return ec == error::address_in_use;
}

GameManagerHosting::GameManagerHosting(Log& log, SettingsHosting& settings) : log_(log), settings_(settings)
{
}

/**
 *  Spiel starten
 */
bool GameManagerHosting::Start()
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
void GameManagerHosting::Stop()
{
    // GAMECLIENT.Stop();
    // GAMESERVER.Stop();
    LOBBYCLIENT.Stop();
    // Global Einstellungen speichern
    // settings_.Save();
}

/**
 *  Hauptschleife.
 */
bool GameManagerHosting::Run()
{
    LOBBYCLIENT.SetProgramVersion(RTTR_Version::GetReadableVersion());
    if(!LOBBYCLIENT.IsLoggedIn() && !LOBBYCLIENT.IsConnecting())
    {
        FramesInfo::UsedClock::time_point currentTime = FramesInfo::UsedClock::now();
        FramesInfo::milliseconds32_t passedTime =
          std::chrono::duration_cast<FramesInfo::milliseconds32_t>(currentTime - last_lobby_time);

        FramesInfo::milliseconds32_t retry_interval(SETTINGS_HOSTING.lobby.retry_interval);

        if(passedTime >= retry_interval)
        {
            std::string user, pass;
            ReadFromEditAndSaveLobbyData(user, pass);

            std::string time_string = s25util::Time::FormatTime("(%H:%i:%s)");

            if(user == "")
            {
                LOG.write(
                  "%s Lobby username is missing, update CONFIG_HOSTING.ini, Retry interval (milliseconds): %d\n")
                  % time_string % SETTINGS_HOSTING.lobby.retry_interval;
            } else if(pass == "")
            {
                LOG.write(
                  "%s Lobby password is missing, update CONFIG_HOSTING.ini, Retry interval (milliseconds): %d\n")
                  % time_string % SETTINGS_HOSTING.lobby.retry_interval;
            } else
            {
                LOG.write("%s Lobby client not logged in, trying login with username %s\n") % time_string % user;

                if(!LOBBYCLIENT.Login(LOADER.GetTextN("client", 0),
                                      s25util::fromStringClassic<unsigned>(LOADER.GetTextN("client", 1)), user, pass,
                                      SETTINGS_HOSTING.server.ipv6))
                {
                    LOG.write("%s Lobby client connection failed to server. Retry interval (milliseconds): %d\n")
                      % time_string % user % SETTINGS_HOSTING.lobby.retry_interval;
                } else
                {
                    LOBBYCLIENT.AddListener(this);
                }
            }
            last_lobby_time = FramesInfo::UsedClock::now();
        }
    }

    LOBBYCLIENT.Run();

    ReadChildProcesses();

    // GAMECLIENT.Run();
    // GAMESERVER.Run();

    // gfCounter_.update();

    return GLOBALVARS.notdone;
}

// void GameManagerHosting::ResetAverageGFPS()
//{
//    gfCounter_ = FrameCounter(FrameCounter::clock::duration::max()); // Never update
//}

static GameManagerHosting* globalGameManagerHosting = nullptr;

GameManagerHosting& getGlobalGameManagerHosting()
{
    return *globalGameManagerHosting;
}
void setGlobalGameManagerHosting(GameManagerHosting* gameManagerHosting)
{
    globalGameManagerHosting = gameManagerHosting;
}

void GameManagerHosting::LC_Chat(const std::string& player, const std::string& text)
{
    unsigned player_color = ctrlChat::CalcUniqueColor(player);

    std::string time_string = s25util::Time::FormatTime("(%H:%i:%s)");
    auto msg_color = COLOR_YELLOW;

    LOG.write("%s <") % time_string;
    LOG.writeColored("%1%", player_color) % player;
    LOG.write(">: ");
    LOG.writeColored("%1%", msg_color) % text;
    LOG.write("\n");

    if(text.substr(0, 6) == ".host ")
    {
        std::string map_name = "RTTR/MAPS/NEW/" + text.substr(7);
        std::string response_msg = StartServer(map_name, player);

        if(response_msg == "")
        {
            response_msg = "Game created for owner " + player + " with map "
                           + map_name; // + ". Password:" + GAMESERVER.getPassword();
        } else
        {
            response_msg = "Error creating game with map: " + response_msg;
        }
        LOBBYCLIENT.SendChat(response_msg);
    }
};

void GameManagerHosting::ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass)
{
    // Potential false positive: User uses new password which is equal to the hash of the old one. HIGHLY unlikely, so
    // ignore

    constexpr auto md5HashLen = 32;

    SETTINGS_HOSTING.Load();

    user = SETTINGS_HOSTING.lobby.name;
    pass = SETTINGS_HOSTING.lobby.password;

    if(pass == "")
    {
        return;
    }

    // if not md5 format
    if(!(pass.size() == md5HashLen + 4 && pass.substr(0, 4) == "md5:"))
    {
        pass = "md5:" + s25util::md5(pass).toString();
        SETTINGS_HOSTING.lobby.password = pass;
        SETTINGS_HOSTING.Save();
    }
    pass = SETTINGS_HOSTING.lobby.password.substr(4);
}

void GameManagerHosting::LC_Status_Error(const std::string& error)
{
    std::string time_string = s25util::Time::FormatTime("(%H:%i:%s)");
    LOG.write("%s Lobby client error: %s; Check CONFIG_HOSTING.ini\n") % time_string % error;
}

void GameManagerHosting::LC_Status_ConnectionLost()
{
    LC_Status_IncompleteMessage();
}

void GameManagerHosting::LC_Status_IncompleteMessage()
{
    std::string time_string = s25util::Time::FormatTime("(%H:%i:%s)");
    LOG.write("%s Lost connection to lobby!\n") % time_string;
}

int GameManagerHosting::FindFirstPort()
{
    for(int port = SETTINGS_HOSTING.server.portStart; port <= SETTINGS_HOSTING.server.portEnd; ++port)
    {
        if(!port_in_use(port))
            return port;
    }
    return -1;
}

std::string GameManagerHosting::StartServer(std::string map_name, std::string owner)
{
    int port = FindFirstPort();
    if (port == -1) {
        return "No ports available";
    }

    
    if(!boost::filesystem::exists(map_name))
    {
        return "Given map does not exist!";
    }

    HostedServerInfo* h = new HostedServerInfo(map_name, port, SETTINGS_HOSTING.server.ipv6, owner, ios);
    hosted_servers.push_back(h);

    return "";
}

void GameManagerHosting::ReadChildProcesses()
{
    ios.run();
    // ios.reset();
    // ios.poll();
    for(auto h_it = hosted_servers.begin(); h_it != hosted_servers.end();)
    {
        if((*h_it)->GetState() == HostedServerInfo::HostedState::Stopped)
        {
            //std::string text = "Game stopped with map_name=" + (*h_it)->GetMapName() + ", owner_name="
            //                   + (*h_it)->GetOwnerName() + ", pid=" + std::to_string((*h_it)->GetPID()) + "\n";
            //LOG.write(text.c_str());
            LOBBYCLIENT.DeleteServer();
            delete *h_it;
            h_it = hosted_servers.erase(h_it);
            continue;

        }

        ++h_it;     
    }
}

