// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SettingsServer.h"
#include "RTTR_Version.h"
#include "RttrConfig.h"
//#include "drivers/AudioDriverWrapper.h"
//#include "drivers/VideoDriverWrapper.h"
#include "files.h"
#include "helpers/strUtils.h"
//#include "languages.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/StringConversion.h"
#include "s25util/System.h"
#include "s25util/error.h"
#include <boost/dll/shared_library.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/filesystem/operations.hpp>

namespace bfs = boost::filesystem;

const int SettingsServer::VERSION = 7;
const std::array<std::string, 3> SettingsServer::SECTION_NAMES = {
  {"global"/*, "language"*/, "proxy", "addons"}};


SettingsServer::SettingsServer() //-V730
{
    LoadDefaults();
}

void SettingsServer::LoadDefaults()
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
    //language.language.clear();
    // }

    //LANGUAGES.setLanguage(language.language);

    proxy = ProxySettings();
    proxy.port = 1080;

 
    // addons
    // {
    addons.configuration.clear();
    // }
}

///////////////////////////////////////////////////////////////////////////////
// Routine zum Laden der Konfiguration
void SettingsServer::Load()
{
    libsiedler2::Archiv settings;
    const auto settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config_server);
    try
    {
        if(libsiedler2::Load(settingsPath, settings) != 0 /*|| settings.size() != SECTION_NAMES.size()*/)
            throw std::runtime_error("File missing or invalid");

        const libsiedler2::ArchivItem_Ini* iniGlobal =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
        //const libsiedler2::ArchivItem_Ini* iniLanguage =
        //  static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
        const libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));
        const libsiedler2::ArchivItem_Ini* iniAddons =
          static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

        // ist eine der Kategorien nicht vorhanden?
        if(!iniGlobal /*|| !iniLanguage*/ || !iniProxy || !iniAddons)
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
        //language.language = iniLanguage->getValue("language");
        // }

        //LANGUAGES.setLanguage(language.language);

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
        //else if(proxy.type == ProxyType::Socks4 && global.ipv6)
        //    global.ipv6 = false;

        // addons
        // {
        for(unsigned addon = 0; addon < iniAddons->size(); ++addon)
        {
            const auto* item = dynamic_cast<const libsiedler2::ArchivItem_Text*>(iniAddons->get(addon));

            if(item)
                addons.configuration.insert(std::make_pair(s25util::fromStringClassic<unsigned>(item->getName()),
                                                           s25util::fromStringClassic<unsigned>(item->getText())));
        }
        // }

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
void SettingsServer::Save()
{
    libsiedler2::Archiv settings;
    settings.alloc(SECTION_NAMES.size());
    for(unsigned i = 0; i < SECTION_NAMES.size(); ++i)
        settings.set(i, std::make_unique<libsiedler2::ArchivItem_Ini>(SECTION_NAMES[i]));

    libsiedler2::ArchivItem_Ini* iniGlobal = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("global"));
    //libsiedler2::ArchivItem_Ini* iniLanguage = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("language"));
    libsiedler2::ArchivItem_Ini* iniProxy = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("proxy"));
    libsiedler2::ArchivItem_Ini* iniAddons = static_cast<libsiedler2::ArchivItem_Ini*>(settings.find("addons"));

    // ist eine der Kategorien nicht vorhanden?
    RTTR_Assert(iniGlobal /*&& iniLanguage*/ && iniProxy && iniAddons);

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
    //iniLanguage->setValue("language", language.language);
    // }

    // proxy
    // {
    iniProxy->setValue("proxy", proxy.hostname);
    iniProxy->setValue("port", proxy.port);
    iniProxy->setValue("typ", static_cast<int>(proxy.type));
    // }

    // addons
    // {
    iniAddons->clear();
    for(const auto& it : addons.configuration)
        iniAddons->addValue(s25util::toStringClassic(it.first), s25util::toStringClassic(it.second));
    // }

    bfs::path settingsPath = RTTRCONFIG.ExpandPath(s25::resources::config_server);
    if(libsiedler2::Write(settingsPath, settings) == 0)
        bfs::permissions(settingsPath, bfs::owner_read | bfs::owner_write);
}
