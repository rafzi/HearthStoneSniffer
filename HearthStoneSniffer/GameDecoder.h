#pragma once
#include "tcp/Parser.h"
#include "tcp/Stream.h"

#include <array>
#include <memory>
#include "range.h"
#include <vector>

class GameDecoder :
	public tcp::Parser::Callback
{
	typedef enum {
		GET_GAME_STATE = 1,
		CHOOSE_OPTION,
		CHOOSE_ENTITIES,
		PRE_CAST,
		DEBUG_MESSAGE,
		CLIENT_PACKET,
		START_GAME_STATE,
		FINISH_GAME_STATE,
		TURN_TIMER,
		NACK_OPTION,
		GIVE_UP,
		GAME_CANCELLED,
		ALL_OPTIONS = 14,
		USER_UI,
		GAME_SETUP,
		ENTITY_CHOICE,
		PRE_LOAD,
		POWER_HISTORY,
		NOTIFICATION = 21,
		AUTO_LOGIN = 103,
		BEGIN_PLAYING = 113,
		GAME_STARTING = 114,
		DEBUG_CONSOLE_COMMAND = 123,
		DEBUG_CONSOLE_RESPONSE = 124,
		AURORA_HANDSHAKE = 168,
	} HSPacketType;

public:
	GameDecoder(int64_t nanotime, tcp::Stream *stream);
	virtual ~GameDecoder();

	virtual void operator()(int64_t nanotime, std::range<const uint8_t *> data);

private:
	tcp::Stream * const _stream;

	std::array<uint8_t, 8> _header;
	std::vector<uint8_t> _message;
	std::range<uint8_t *> _buffer;

	class Decode;
	std::shared_ptr<Decode> _decode;
};

