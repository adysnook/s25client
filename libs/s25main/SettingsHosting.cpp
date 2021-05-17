// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SettingsHosting.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
#include "drivers/AudioDriverWrapper.h"
#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/strUtils.h"
#include "languages.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/error.h"
#include <boost/filesystem/operations.hpp>

const int SettingsHosting::VERSION = 1;
const std::array<std::string, 5> SettingsHosting::SECTION_NAMES = {
  {"global", "language", "lobby", "server", "proxy"}};

/* already defined in Settings.cpp
namespace validate {
boost::optional<uint16_t> checkPort(const std::string& port)
{
    int32_t iPort;
    if((helpers::tryFromString(port, iPort) || s25util::tryFromStringClassic(port, iPort)) && checkPort(iPort))
        return static_cast<uint16_t>(iPort);
    else
        return boost::none;
}
bool checkPort(int port)
{
    // Disallow port 0 as it may cause problems
    return port > 0 && port <= 65535;
}
} // namespace validate
*/


SettingsHosting::SettingsHosting() //-V730
{
    LoadDefaults();
}

void SettingsHosting::LoadDefaults()
{
    // global
    // {
    // 0 = ask user at start,1 = enabled, 2 = disabled
    global.submit_debug_data = 0;
    global.use_upnp = 2;
    global.debugMode = false;
    // }

    // language
    // {
    language.language.clear();
    // }

    LANGUAGES.setLanguage(language.language);

    // lobby
    // {

    lobby.name = "";
    lobby.password.clear();
    lobby.save_password = false;
    lobby.retry_interval = 10000; //10s
    // }

    // server
    // {
    server.portStart = 36650;
    server.portEnd = 37650;
    server.ipv6 = false;
    // }

    proxy = ProxySettings();
    proxy.port = 1080;
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Laden der Konfiguration
void SettingsHosting::Load()
{
    libsiedler2::Archiv settings;
    const auto settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config_hosting);
    try
    {
        if(libsiedler2::Load(settingsPath, settings) != 0 /*|| settings.size() != SECTION_NAMES.size()*/)
            throw std::runtime_error("File missing or invalid");

        const libsiedler2::ArchivItem_Ini* iniGlobal =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
        const libsiedler2::ArchivItem_Ini* iniLanguage =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
        const libsiedler2::ArchivItem_Ini* iniLobby = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("lobby"));
        const libsiedler2::ArchivItem_Ini* iniServer =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("server"));
        const libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));

        // ist eine der Kategorien nicht vorhanden?
        if(!iniGlobal || !iniLanguage || !iniLobby || !iniServer || !iniProxy)
        {
            throw std::runtime_error("Missing section");
        }
        // stimmt die Settingsversion?
        if(iniGlobal->getValueI("version") != VERSION)
        {
            throw std::runtime_error("Wrong version");
        }

        // global
        // {
        // stimmt die Spielrevision überein?
        if(iniGlobal->getValue("gameversion") != RTTR_Version::GetRevision())
            s25util::warning("Your application version has changed - please recheck your settings!\n");

        global.submit_debug_data = iniGlobal->getValueI("submit_debug_data");
        global.use_upnp = iniGlobal->getValueI("use_upnp");
        global.debugMode = (iniGlobal->getValueI("debugMode") != 0);

        // };

        // language
        // {
        language.language = iniLanguage->getValue("language");
        // }

        LANGUAGES.setLanguage(language.language);

        // lobby
        // {
        lobby.name = iniLobby->getValue("name");
        lobby.password = iniLobby->getValue("password");
        lobby.save_password = (iniLobby->getValueI("save_password") != 0);
        lobby.retry_interval = iniLobby->getValueI("retry_interval");
        // }

        // server
        // {
        boost::optional<uint16_t> port_start = validate::checkPort(iniServer->getValue("port_start"));
        server.portStart = port_start.value_or(36650);
        boost::optional<uint16_t> port_end = validate::checkPort(iniServer->getValue("port_end"));
        server.portEnd = port_end.value_or(37650);
        server.ipv6 = (iniServer->getValueI("ipv6") != 0);
        // }

        // proxy
        // {
        proxy.hostname = iniProxy->getValue("proxy");
        boost::optional<uint16_t> port = validate::checkPort(iniProxy->getValue("port"));
        proxy.port = port.value_or(1080);
        proxy.type = ProxyType(iniProxy->getValueI("typ"));
        // }

        // leere proxyadresse deaktiviert proxy komplett
        // deaktivierter proxy entfernt proxyadresse
        if(proxy.hostname.empty() || (proxy.type != ProxyType::Socks4 && proxy.type != ProxyType::Socks5))
        {
            proxy.type = ProxyType::None;
            proxy.hostname.clear();
        }
        // aktivierter Socks v4 deaktiviert ipv6
        //else if(proxy.type == ProxyType::Socks4 && server.ipv6)
        //    server.ipv6 = false;

    } catch(std::runtime_error& e)
    {
        s25util::warning(std::string("Could not use settings from \"") + settingsPath.string()
                         + "\", using default values. Reason: " + e.what());
        //LoadDefaults();
        //Save();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Speichern der Konfiguration
void SettingsHosting::Save()
{
    libsiedler2::Archiv settings;
    settings.alloc(SECTION_NAMES.size());
    for(unsigned i = 0; i < SECTION_NAMES.size(); ++i)
        settings.set(i, std::make_unique<libsiedler2::ArchivItem_Ini>(SECTION_NAMES[i]));

    libsiedler2::ArchivItem_Ini* iniGlobal = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
    libsiedler2::ArchivItem_Ini* iniLanguage = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
    libsiedler2::ArchivItem_Ini* iniLobby = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("lobby"));
    libsiedler2::ArchivItem_Ini* iniServer = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("server"));
    libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniGlobal && iniLanguage && iniLobby && iniServer && iniProxy);

    // global
    // {
    iniGlobal->setValue("version", VERSION);
    iniGlobal->setValue("gameversion", RTTR_Version::GetRevision());
    iniGlobal->setValue("submit_debug_data", global.submit_debug_data);
    iniGlobal->setValue("use_upnp", global.use_upnp);
    iniGlobal->setValue("debugMode", global.debugMode ? 1 : 0);
    // };

    // language
    // {
    iniLanguage->setValue("language", language.language);
    // }

    // lobby
    // {
    iniLobby->setValue("name", lobby.name);
    iniLobby->setValue("password", lobby.password);
    iniLobby->setValue("save_password", (lobby.save_password ? 1 : 0));
    iniLobby->setValue("retry_interval", lobby.retry_interval);
    // }

    // server
    // {
    iniServer->setValue("port_start", server.portStart);
    iniServer->setValue("port_end", server.portEnd);
    iniServer->setValue("ipv6", (server.ipv6 ? 1 : 0));
    // }

    // proxy
    // {
    iniProxy->setValue("proxy", proxy.hostname);
    iniProxy->setValue("port", proxy.port);
    iniProxy->setValue("typ", static_cast<int>(proxy.type));
    // }

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config_hosting);
    if(libsiedler2::Write(settingsPath, settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);
}
