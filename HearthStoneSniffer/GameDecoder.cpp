#include <wx/filename.h>
#include <wx/log.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>

#include "HSSnifferApp.h"
#include "Helper.h"

#include "StartGameState.pb.h"
#include "PowerHistory.pb.h"

#include "GameDecoder.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

template <typename T> void swap_clear(T &v) { if (!v.empty()) { T x; v.swap(x); } }

class GameDecoder::Decode
{
	typedef std::vector<byte> Bytes;
	typedef std::pair<int64_t, Bytes> Message;
	typedef std::vector<Message> MessageList;

public:
	Decode(std::string name, int64_t nanotime)
		: _name(std::move(name))
	{
		wxLogVerbose("%lld %s logging", nanotime, _name);
		_messages.emplace_back(nanotime, Bytes());
	}

	~Decode()
	{
		if (_messages.size() <= 1) {
			return;
		}
	}

	void Add(int64_t nanotime, std::vector<uint8_t> message)
	{

		if (WasCanceled()) {
			// Cancel was called previously, so don't store messages anymore
			return;
		}

		_messages.emplace_back(nanotime, message);

		auto header = reinterpret_cast<int32_t *>(message.data());
		wxLogVerbose("%lld %s (%d, %d)", nanotime, _name, header[0], header[1]);


		HSPacketType type = static_cast<HSPacketType>(header[0]);
		int len = header[1];

		if (len > 0)
		{
			decodePacket(type, len, &message[8]);
		}
		else {
			decodePacket(type, len, NULL);
		}

	}

	void decodePacket(HSPacketType type, int len, uint8_t* data)
	{
		switch (type){
		case GET_GAME_STATE:
			wxLogVerbose("GET_GAME_STATE packet");
			break;
		case CHOOSE_OPTION:
			wxLogVerbose("CHOOSE_OPTION packet");
			break;
		case CHOOSE_ENTITIES:
			wxLogVerbose("CHOOSE_ENTITIES packet");
			break;
		case PRE_CAST:
			wxLogVerbose("PRE_CAST packet");
			break;
		case DEBUG_MESSAGE:
			wxLogVerbose("DEBUG_MESSAGE packet");
			break;
		case CLIENT_PACKET:
			wxLogVerbose("CLIENT_PACKET packet");
			break;
		case START_GAME_STATE:
			wxLogVerbose("START_GAME_STATE packet");
			decodeStartGameState(data, len);
			break;
		case FINISH_GAME_STATE:
			wxLogVerbose("FINISH_GAME_STATE packet");
			break;
		case TURN_TIMER:
			wxLogVerbose("TURN_TIMER packet");
			break;
		case NACK_OPTION:
			wxLogVerbose("NACK_OPTION packet");
			break;
		case GIVE_UP:
			wxLogVerbose("GIVE_UP packet");
			break;
		case GAME_CANCELLED:
			wxLogVerbose("GAME_CANCELLED packet");
			break;
		case ALL_OPTIONS:
			wxLogVerbose("ALL_OPTIONS packet");
			break;
		case USER_UI:
			wxLogVerbose("USER_UI packet");
			break;
		case GAME_SETUP:
			wxLogVerbose("GAME_SETUP packet");
			break;
		case ENTITY_CHOICE:
			wxLogVerbose("ENTITY_CHOICE packet");
			break;
		case PRE_LOAD:
			wxLogVerbose("PRE_LOAD packet");
			break;
		case POWER_HISTORY:
			wxLogVerbose("POWER_HISTORY packet");
			decodePowerHistory(data, len);
			break;
		case NOTIFICATION:
			wxLogVerbose("NOTIFICATION packet");
			break;
		case AUTO_LOGIN:
			wxLogVerbose("AUTO_LOGIN packet");
			break;
		case BEGIN_PLAYING:
			wxLogVerbose("BEGIN_PLAYING packet");
			break;
		case GAME_STARTING:
			wxLogVerbose("GAME_STARTING packet");
			break;
		case DEBUG_CONSOLE_COMMAND:
			wxLogVerbose("DEBUG_CONSOLE_COMMAND packet");
			break;
		case DEBUG_CONSOLE_RESPONSE:
			wxLogVerbose("DEBUG_CONSOLE_RESPONSE packet");
			break;
		case AURORA_HANDSHAKE:
			wxLogVerbose("AURORA_HANDSHAKE packet");
			break;
		default:
			wxLogVerbose("UNKNOWN packet");
			break;
		}
	}

	void decodeStartGameState(uint8_t* data, int len)
	{
		StartGameState state;
		state.ParseFromArray(data, len);
		wxLogVerbose(state.DebugString().c_str());
	}

	void decodePowerHistory(uint8_t* data, int len)
	{
		PowerHistory history;
		history.ParseFromArray(data, len);
		auto iter = history.list().begin();
		for (; iter != history.list().end(); iter++)
		{
			PowerHistoryEntity entity = iter->show_entity();
			if (entity.IsInitialized())
			{
				wxLogVerbose(entity.DebugString().c_str());
			}
		}
		// wxLogVerbose(history.DebugString().c_str());
	}

	void Cancel()
	{
		swap_clear(_messages); // clear and release memory
	}

	bool WasCanceled()
	{
		return _messages.empty();
	}


private:
	std::string _name;
	MessageList _messages;
};

GameDecoder::GameDecoder(int64_t nanotime, tcp::Stream *stream)
	: _stream(stream),
	_header(),
	_message(),
	_buffer(_header.data(), _header.data() + _header.size()),
	_decode()
{
	if (_stream->Other()) {
		_decode = reinterpret_cast<GameDecoder*>(_stream->Other()->Callback())->_decode;
	} else {
		_decode = std::make_shared<Decode>(_stream->Endpoints().SrcToDst(), nanotime);
	}
}

GameDecoder::~GameDecoder()
{
	if (_buffer.begin() != _header.data()) {
		wxLogWarning("%s canceling log (stream closed mid-packet)", _stream->Endpoints().SrcToDst());
		_decode->Cancel();
	}
	//wxLogVerbose("stream closed: (%s)", _stream->Endpoints().SrcToDst());
}

void GameDecoder::operator()(int64_t nanotime, std::range<const uint8_t *> data)
{
	if (_decode->WasCanceled()) {
		swap_clear(_message);
		return;
	}

	while (!data.empty()) {
		auto toCopy = std::min(_buffer.size(), data.size());

		std::copy(data.begin(), data.begin() + toCopy, _buffer.begin());
		_buffer.pop_front(toCopy);
		data.pop_front(toCopy);

		if (_buffer.empty()) {
			if (_buffer.end() == _header.data() + _header.size()) {
				// Done reading header, parse it
				auto ptr = reinterpret_cast<uint32_t *>(_header.data());
				auto type = ptr[0];
				auto size = ptr[1];

				// Sanity check the values
				if (type > 1000 || size > 8000) {
					wxLogVerbose("%s canceling log (bad header: %d, %d)", _stream->Endpoints().SrcToDst(), type, size);
					_decode->Cancel();
					swap_clear(_message);
					_buffer = std::make_range(_header.data(), _header.data() + _header.size());
					return;
				}

				// Allocate space for the message and copy the header to the start
				_message.resize(8 + size);
				std::copy(_header.data(), _header.data() + 8, _message.data());
				_buffer = std::make_range(_message.data() + 8, _message.data() + _message.size());

			}
			else {
				// Done reading message, add it to the log
				_decode->Add(nanotime, std::move(_message));

				// Setup for another header next
				_buffer = std::make_range(_header.data(), _header.data() + _header.size());
			}
		}
	}
	// wxLogVerbose("packet: %d (%s)", data.size(), _stream->Endpoints().SrcToDst());
}