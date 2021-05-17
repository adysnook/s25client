// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>

/// Tries to start a game (map only) and returns whether this was successfull
bool QuickStartGameServer(const boost::filesystem::path& mapOrReplayPath, uint16_t port, const std::string owner, bool ipv6);
