#ifndef RHS_MSG_HDR
#define RHS_MSG_HDR

#include <string>

//TODO REMOVE THIS
//enum msg_code { START_CAMERA, STOP_CAMERA, QUIT };

namespace rhs {

typedef uint16_t message_t;

const message_t START_CAMERA  = 1U;
const message_t STOP_CAMERA   = 2U;
const message_t QUIT          = 3U;
const message_t OBJECT_DATA   = 4U;

class Message {
public:

	Message(uint16_t type, std::string pl=std::string());
	~Message();

	uint16_t type;
	std::string payload;
};

Message::Message(uint16_t t, std::string pl) 
		: type(t), payload(pl) {

}

Message::~Message() {

}

}

#endif