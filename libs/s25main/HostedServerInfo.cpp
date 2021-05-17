// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later


#include "HostedServerInfo.h"
#include <boost/bind/bind.hpp>
#include <iomanip>
#include "s25util/Log.h"
#include "liblobby/LobbyClient.h"

void HostedServerInfo::_handler(const boost::system::error_code& ec, std::size_t size)
{
	std::stringstream sstream;
	if(ec)
	{
		state = HostedState::Stopped;
		sstream << "[" << std::setw(6) << proc.id() << "] Stopped" << std::endl;
		LOG.write(sstream.str().c_str());
		return;
	}

	std::istream is(&buf);
	std::string line;
	std::getline(is, line);
	if (line.substr(line.length() - 1) == "\r") {
		line = line.substr(0, line.length() - 1);
	}
	sstream << "[" << std::setw(6) << proc.id() << "]:" << line << std::endl;
	LOG.write(sstream.str().c_str());

	if (line.substr(0, 1) == ".") {
		const std::string gsok_command = ".GameServer started:";
		if(line.substr(0, gsok_command.length()) == gsok_command)
		{
			std::string gamename = line.substr(gsok_command.length());
			LOBBYCLIENT.AddServer(gamename, map_name, false, port);
		}

		const std::string update_command = ".UpdateServerNumPlayers:";
		if (line.substr(0, update_command.length()) == update_command) {
			int num_on  = std::stoi(line.substr(update_command.length() , 1));
			int num_max = std::stoi(line.substr(update_command.length() + 2, 1));

			LOBBYCLIENT.UpdateServerNumPlayers(num_on, num_max);
		}
	}

	// call async read again
	boost::asio::async_read_until(
	  pipe_out, buf, '\n',
	  boost::bind(&HostedServerInfo::_handler, this, boost::placeholders::_1, boost::placeholders::_2));
}

HostedServerInfo::HostedServerInfo(std::string map, uint16_t port, bool ipv6, std::string owner, boost::asio::io_service& ios)
	: map_name(map), port(port), owner_name(owner), state(HostedState::Starting), pipe_out(ios), buf(1024),
	  proc(s25server_path,
		   std::vector<std::string>(
			 {"--map", map_name, "-p", std::to_string(port), "-o", owner, "--ipv6", std::to_string(ipv6)}),
		   boost::process::std_in.close(), boost::process::std_out > pipe_out, ios)
{
	std::stringstream sstream;
	std::vector<std::string> params(
	  {"--map", map_name, "-p", std::to_string(port), "-o", owner, "--ipv6", std::to_string(ipv6)});

	std::string command = s25server_path;
	sstream << "[" << std::setw(6) << proc.id() << "] " << s25server_path;
	for(unsigned i = 0; i < params.size(); ++i)
		sstream << ' ' << params[i];
	sstream << std::endl;

	LOG.write(sstream.str().c_str());

	boost::asio::async_read_until(
	  pipe_out, buf, '\n',
	  boost::bind(&HostedServerInfo::_handler, this, boost::placeholders::_1, boost::placeholders::_2)
	);
};
