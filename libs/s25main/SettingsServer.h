// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//#include "driver/VideoMode.h"
#include "s25util/ProxySettings.h"
#include "s25util/Singleton.h"
#include <boost/optional.hpp>
#include <array>
#include <map>
#include <string>

#undef interface

namespace validate {
boost::optional<uint16_t> checkPort(const std::string& port);
bool checkPort(int port);
} // namespace validate

/// Configuration class
class SettingsServer : public Singleton<SettingsServer, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 18;

    SettingsServer();

    void Load();
    void Save();

protected:
    void LoadDefaults();

public:
    struct
    {
        unsigned submit_debug_data;
        unsigned use_upnp;
        bool debugMode;
    } global;

    //struct
    //{
    //    std::string language;
    //} language;

    ProxySettings proxy;

    struct
    {
        std::map<unsigned, unsigned> configuration;
    } addons;

private:
    static const int VERSION;
    static const std::array<std::string, 3> SECTION_NAMES;
};

#define SETTINGS_SERVER SettingsServer::inst()
