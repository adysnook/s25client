// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FrameCounter.h"
#include <boost/optional.hpp>

class Log;
class SettingsServer;

/// "Die" GameManager-Klasse
class GameManagerServer
{
public:
    GameManagerServer(Log& log, SettingsServer& settings);

    bool Start();
    void Stop();
    bool Run();

    /// Average FPS Zähler zurücksetzen.
    void ResetAverageGFPS();
    FrameCounter::clock::duration GetRuntime() { return gfCounter_.getCurIntervalLength(); }
    unsigned GetNumFrames() { return gfCounter_.getCurNumFrames(); }
    unsigned GetAverageGFPS() { return gfCounter_.getCurFrameRate(); }

private:

    Log& log_;
    SettingsServer& settings_;
    FrameCounter gfCounter_;
};

GameManagerServer& getGlobalGameManagerServer();
void setGlobalGameManagerServer(GameManagerServer* gameManagerServer);

#define GAMEMANAGERSERVER getGlobalGameManagerServer()
