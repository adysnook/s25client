// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FrameCounter.h"
#include "FramesInfo.h"
#include "liblobby/LobbyInterface.h"
#include <boost/optional.hpp>
#include "SettingsHosting.h"
#include "s25util/Log.h"
#include <iostream>
#include "liblobby/LobbyClient.h"
#include <boost/asio.hpp>

class Log;
class SettingsHosting;
class HostedServerInfo;

/// "Die" GameManager-Klasse
class GameManagerHosting : public LobbyInterface
{
public:
    GameManagerHosting(Log& log, SettingsHosting& settings);

    bool Start();
    void Stop();
    bool Run();

    FrameCounter::clock::duration GetRuntime() { return gfCounter_.getCurIntervalLength(); }

    void LC_Chat(const std::string& player, const std::string& text);
    void LC_Status_Error(const std::string& error) override;
    void LC_Status_ConnectionLost() override;
    void LC_Status_IncompleteMessage() override;

private:
    Log& log_;
    SettingsHosting& settings_;
    FrameCounter gfCounter_;
    void ReadFromEditAndSaveLobbyData(std::string& user, std::string& pass);
    FramesInfo::UsedClock::time_point last_lobby_time;
    int FindFirstPort();
    std::string StartServer(const std::string map_name, const std::string owner);
    std::vector<HostedServerInfo*> hosted_servers;
    void ReadChildProcesses();
    boost::asio::io_service ios;
};

GameManagerHosting& getGlobalGameManagerHosting();
void setGlobalGameManagerHosting(GameManagerHosting* gameManagerHosting);

#define GAMEMANAGERHOSTING getGlobalGameManagerHosting()
