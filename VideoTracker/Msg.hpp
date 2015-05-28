// =============================================================================
//
//  This file is part of the final project source code for the course "Ad hoc
//  and sensor networks" (Master's degree in Computer Science, University of
//  Pisa)
//
//  Copyright (C) 2015, Andrea Maggiordomo
//
// =============================================================================

#ifndef RHS_MSG_HDR
#define RHS_MSG_HDR

#include <string>

namespace rhs {

// class Message
// Protocol message. The payload is (for now) transmitted in plain text
class Message {
public:
	// type codes
	static const uint16_t START_CAMERA  = 1U;
	static const uint16_t STOP_CAMERA   = 2U;
	static const uint16_t QUIT          = 3U;
	static const uint16_t OBJECT_DATA   = 4U;
	static const uint16_t STREAM_START  = 5U;
	static const uint16_t STREAM_STOP   = 6U;

	// Constructor (type and payload)
	Message(uint16_t t, std::string pl=std::string()) : type(t), payload(pl) {  }
	~Message() {  }

	uint16_t type;
	std::string payload;
};

}

#endif