// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessage_GameCommand.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"
#include <s25util/Log.h>

//////////////////////////////////////////////////////////////////////////

GameMessage_GameCommand::GameMessage_GameCommand() : GameMessageWithPlayer(NMS_GAMECOMMANDS) {}

GameMessage_GameCommand::GameMessage_GameCommand(uint8_t player, const AsyncChecksum& checksum,
                                                 const std::vector<gc::GameCommandPtr>& gcs)
    : GameMessageWithPlayer(NMS_GAMECOMMANDS, player), cmds(checksum, gcs)
{
    //LOG.writeToFile(">>> NMS_GAMECOMMANDS(%d, %d, %d)\n") % unsigned(player) % checksum.getHash() % gcs.size();
}

void GameMessage_GameCommand::Serialize(Serializer& ser) const
{
    GameMessageWithPlayer::Serialize(ser);
    cmds.Serialize(ser);
}

void GameMessage_GameCommand::Deserialize(Serializer& ser)
{
    GameMessageWithPlayer::Deserialize(ser);
    cmds.Deserialize(ser);
}

bool GameMessage_GameCommand::Run(GameMessageInterface* callback) const
{
    //LOG.writeToFile("<<< NMS_GAMECOMMANDS(%d, %d, %d)\n") % unsigned(player) % cmds.checksum.getHash() % cmds.gcs.size();
    return callback->OnGameMessage(*this);
}
