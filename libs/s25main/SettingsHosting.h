// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "driver/VideoMode.h"
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
class SettingsHosting : public Singleton<SettingsHosting, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 18;

    SettingsHosting();

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

    struct
    {
        std::string language;
    } language;

    struct
    {
        std::string name;
        std::string password;
        bool save_password;
        uint32_t retry_interval;
    } lobby;

    struct
    {
        uint16_t portStart;
        uint16_t portEnd;
        bool ipv6; /// listen/connect on ipv6 as default or not
    } server;

    ProxySettings proxy;

private:
    static const int VERSION;
    static const std::array<std::string, 5> SECTION_NAMES;
};

#define SETTINGS_HOSTING SettingsHosting::inst()
