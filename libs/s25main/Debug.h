// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/Socket.h"
#include <boost/filesystem/path.hpp>
#include <vector>

class BinaryFile;
class Replay;

// This is for catching crashes and reporting bugs, it does not slow down anything.
class DebugInfo
{
    Socket sock;

public:
    DebugInfo(const ProxySettings& proxy, unsigned int gfNumber);
    ~DebugInfo();

    static std::vector<void*> GetStackTrace(void* ctx = nullptr) noexcept;

    bool Send(const void* buffer, size_t length);
    bool SendSigned(int32_t i);
    bool SendUnsigned(uint32_t i);
    bool SendString(const char* str, size_t len = 0);
    bool SendString(const std::string& str);

    bool SendStackTrace(const std::vector<void*>& stacktrace);
    bool SendReplay(Replay* rpl);
    bool SendAsyncLog(const boost::filesystem::path& asyncLogFilepath);
    bool SendFile(BinaryFile& file);
};
