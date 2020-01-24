#ifndef CTOSMSG_HPP
#define CTOSMSG_HPP
#include <cstdint>

namespace YGOPro
{

class CTOSMsg
{
public:
	enum
	{
		HEADER_LENGTH = 3,
		MSG_MAX_LENGTH = 1021
	};
	using LengthType = int16_t;
	enum class MsgType : int8_t
	{
		RESPONSE      = 0x01,
		UPDATE_DECK   = 0x02,
		HAND_RESULT   = 0x03,
		TP_RESULT     = 0x04,
		PLAYER_INFO   = 0x10,
		CREATE_GAME   = 0x11,
		JOIN_GAME     = 0x12,
		LEAVE_GAME    = 0x13,
		SURRENDER     = 0x14,
		TIME_CONFIRM  = 0x15,
		CHAT          = 0x16,
		HS_TODUELIST  = 0x20,
		HS_TOOBSERVER = 0x21,
		HS_READY      = 0x22,
		HS_NOTREADY   = 0x23,
		HS_KICK       = 0x24,
		HS_START      = 0x25
	};

	struct HostInfo
	{
		uint32_t lflist;
		uint8_t rule;
		uint8_t mode;
		uint8_t duel_rule;
		uint8_t no_check_deck;
		uint8_t no_shuffle_deck;
		uint32_t start_lp;
		uint8_t start_hand;
		uint8_t draw_count;
		uint16_t time_limit;
		uint64_t handshake;
		int32_t team1;
		int32_t team2;
		int32_t best_of;
		uint32_t duel_flag;
		int32_t forbiddentypes;
		uint16_t extra_rules;
	};

	struct PlayerInfo
	{
		uint16_t name[20];
	};

	struct CreateGame
	{
		HostInfo info;
		uint16_t name[20];
		uint16_t pass[20];
		char notes[200];
	};

	CTOSMsg() : r(data + HEADER_LENGTH)
	{}

	LengthType GetLength() const
	{
		LengthType v;
		std::memcpy(&v, data, sizeof(LengthType));
		return v - 1u;
	}

	MsgType GetType() const
	{
		MsgType v;
		std::memcpy(&v, data + sizeof(LengthType), sizeof(MsgType));
		return v;
	}

	bool IsHeaderValid() const
	{
		if(GetLength() > MSG_MAX_LENGTH)
			return false;
		if(GetType() > MsgType::HS_START)
			return false;
		return true;
	}

	uint8_t* Data()
	{
		return data;
	}

	uint8_t* Body()
	{
		return data + HEADER_LENGTH;
	}

#define X(s) \
	std::pair<bool, s> Get##s() const \
	{ \
		std::pair<bool, s> p; \
		if(GetLength() != sizeof(s)) \
			return p; \
		p.first = true; \
		std::memcpy(&p.second, data + HEADER_LENGTH, sizeof(s)); \
		return p; \
	}
	X(PlayerInfo)
	X(CreateGame)
#undef X

private:
	uint8_t data[HEADER_LENGTH + MSG_MAX_LENGTH];
	uint8_t* r;
};

} // namespace YGOPro

#endif // CTOSMSG_HPP
