#ifndef RHS_MSG_HDR
#define RHS_MSG_HDR

#include <string>

//TODO REMOVE THIS
//enum msg_code { START_CAMERA, STOP_CAMERA, QUIT };

namespace rhs {

const uint16_t START_CAMERA  = 1U;
const uint16_t STOP_CAMERA   = 2U;
const uint16_t QUIT          = 3U;
const uint16_t OBJECT_DATA   = 4U;
const uint16_t STREAM_START  = 5U;
const uint16_t STREAM_STOP   = 6U;

class Message {
public:

	Message(uint16_t t, std::string pl=std::string()) : type(t), payload(pl) {  }
	~Message() {  }

	uint16_t type;
	std::string payload;
};

}

#endif