
#pragma once

#include <boost/process/async.hpp>
#include <boost/asio.hpp>
#pragma warning(push)
#pragma warning(disable : 4244) // 'argument': conversion from 'ptrdiff_t' to 'int', possible loss of data
#include <boost/process.hpp>
#pragma warning(pop)

class HostedServerInfo
{
public:
    enum HostedState
    {
        Starting,
        Config,
        Playing,
        Stopped
    };

private:
    std::string map_name;
    std::string owner_name;
    uint16_t port;
    HostedState state;
    boost::process::async_pipe pipe_out;
#ifdef _WIN32
    const std::string s25server_path = "s25server.exe";
#else
    const std::string s25server_path = "s25server";
#endif
    boost::process::child proc;

    boost::asio::streambuf buf;
    void _handler(const boost::system::error_code& ec, std::size_t size);

public:

    HostedServerInfo(std::string map, uint16_t port, bool ipv6, std::string owner, boost::asio::io_service& ios);

    HostedState GetState() const { return state; }
    auto GetPID() const { return proc.id(); }
    auto GetMapName() const { return map_name; }
    auto GetOwnerName() const { return owner_name; }
};