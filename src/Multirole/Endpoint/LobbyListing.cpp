#include "LobbyListing.hpp"

#include <asio/write.hpp>
#include <fmt/format.h> // fmt::to_string
#include <nlohmann/json.hpp>

#include "../Lobby.hpp"
#include "../Workaround.hpp"

namespace Ignis::Multirole::Endpoint
{

// public

LobbyListing::LobbyListing(
	asio::io_context& ioCtx,
	unsigned short port,
	Lobby& lobby)
	:
	acceptor(ioCtx, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port)),
	serializeTimer(ioCtx),
	lobby(lobby),
	serialized(std::make_shared<std::string>())
{
	Workaround::SetCloseOnExec(acceptor.native_handle());
	DoAccept();
	DoSerialize();
}

void LobbyListing::Stop()
{
	acceptor.close();
	serializeTimer.cancel();
}

// private

void LobbyListing::DoSerialize()
{
	serializeTimer.expires_after(std::chrono::seconds(2));
	serializeTimer.async_wait([this](const std::error_code& ec)
	{
		if(ec)
			return;
		auto ComposeHeader = [](std::size_t length, std::string_view mime)
		{
			constexpr const char* HTTP_HEADER_FORMAT_STRING =
			"HTTP/1.0 200 OK\r\n"
			"Content-Length: {:d}\r\n"
			"Content-Type: {:s}\r\n\r\n";
			return fmt::format(HTTP_HEADER_FORMAT_STRING, length, mime);
		};
		nlohmann::json j{{"rooms", nlohmann::json::array()}};
		nlohmann::json& ar = j["rooms"];
		for(auto& rp : lobby.GetAllRoomsProperties())
		{
			ar.emplace_back();
			auto& room = ar.back();
			room["roomid"] = rp.id;
			room["roomname"] = ""; // NOTE: UNUSED but expected atm
			room["roomnotes"] = rp.notes;
			room["roommode"] = 0; // NOTE: UNUSED but expected atm
			room["needpass"] = rp.passworded;
			room["team1"] = rp.hostInfo.t0Count;
			room["team2"] = rp.hostInfo.t1Count;
			room["best_of"] = rp.hostInfo.bestOf;
			room["duel_flag"] = rp.hostInfo.duelFlags;
			room["forbidden_types"] = rp.hostInfo.forb;
			room["extra_rules"] = rp.hostInfo.extraRules;
			room["start_lp"] = rp.hostInfo.startingLP;
			room["start_hand"] = rp.hostInfo.startingDrawCount;
			room["draw_count"] = rp.hostInfo.drawCountPerTurn;
			room["time_limit"] = rp.hostInfo.timeLimitInSeconds;
			room["rule"] = rp.hostInfo.allowed;
			room["no_check"] = static_cast<bool>(rp.hostInfo.dontCheckDeck);
			room["no_shuffle"] = static_cast<bool>(rp.hostInfo.dontShuffleDeck);
			room["banlist_hash"] = rp.hostInfo.banlistHash;
			room["istart"] = rp.started ? "start" : "waiting";
			auto& ac = room["users"];
			for(auto& kv : rp.duelists)
			{
				ac.emplace_back();
				auto& client = ac.back();
				client["name"] = kv.second;
				client["pos"] = kv.first;
			}
		}
		constexpr auto eHandler = nlohmann::json::error_handler_t::replace;
		const std::string strJ = j.dump(-1, 0, false, eHandler); // DUMP EET
		{
			std::scoped_lock lock(mSerialized);
			serialized = std::make_shared<std::string>(
				ComposeHeader(strJ.size(), "application/json") + strJ
			);
		}
		DoSerialize();
	});
}

void LobbyListing::DoAccept()
{
	acceptor.async_accept(
	[this](const std::error_code& ec, asio::ip::tcp::socket socket)
	{
		if(!acceptor.is_open())
			return;
		if(!ec)
		{
			Workaround::SetCloseOnExec(socket.native_handle());
			std::scoped_lock lock(mSerialized);
			std::make_shared<Connection>(std::move(socket), serialized)->DoRead();
		}
		DoAccept();
	});
}

LobbyListing::Connection::Connection(
	asio::ip::tcp::socket socket,
	std::shared_ptr<std::string> data)
	:
	socket(std::move(socket)),
	outgoing(std::move(data)),
	incoming(),
	writeCalled(false)
{}

void LobbyListing::Connection::DoRead()
{
	auto self(shared_from_this());
	socket.async_read_some(asio::buffer(incoming),
	[this, self](const std::error_code& ec, std::size_t /*unused*/)
	{
		if(!ec)
		{
			if(!writeCalled)
			{
				writeCalled = true;
				DoWrite();
			}
			DoRead();
		}
		else if(ec != asio::error::operation_aborted && socket.is_open())
		{
			socket.close();
		}
	});
}

void LobbyListing::Connection::DoWrite()
{
	auto self(shared_from_this());
	asio::async_write(socket, asio::buffer(*outgoing),
	[this, self](std::error_code ec, std::size_t /*unused*/)
	{
		if(!ec)
			socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
	});
}

} // namespace Ignis::Multirole::Endpoint
