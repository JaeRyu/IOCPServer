#pragma once

#define MAX_BUFF_SIZE 4000
#define MAX_PACKET_SIZE  255
#define SERVER_PORT  8888
#define MAX_STRING 256



// Define kind of packet 
// if Server to Client Set prefix SC_
// if Client to Server Set prefix CS_


#define SC_HELLO 1



#define CS_HELLO 1



#pragma pack (push, 1)

struct packet_form {
	unsigned char size;
	unsigned char type;
};


//Struct { Server -> Client packet }

struct sc_packet_hello {
	unsigned char size;
	unsigned char type;
	unsigned char hello;
};


//Struct { Client -> Server }
struct cs_packet_hello {
	unsigned char size;
	unsigned char type;
	unsigned char hello;
};

#pragma pack(pop)