#pragma once
#include "stdafx.h"
#include "protocol.h"

enum OVERLAPPEDTYPE{OVER_RECV, OVER_SEND };

struct ExtendedOverlapped { // Custom WSAOVERLAPPED struct
	WSAOVERLAPPED wsaover;
	OVERLAPPEDTYPE overtype;
	WSABUF wsabuf;
	char io_buf[MAX_BUFF_SIZE];
};


class Client {
public:
	SOCKET s;
	ExtendedOverlapped exover;

	//for Build Packet
	int packet_size; // total size of packet to get
	int prev_size; // size of packet that get before
	char prev_packet[MAX_PACKET_SIZE]; // saved packet

	//Additional Variable-------------------



	//--------------------------------------

	//Constructor
	Client();
	Client(const Client& c);
};

class CServer
{
	SOCKET acceptSock;
	HANDLE g_iocp;
	std::vector<std::thread *> NetworkThreadList;
	std::vector<std::thread *> DBThreadList;
	std::unordered_map<SOCKET, Client> ClientList;
	

private:
	void err_quit(const char * msg);
	void err_display(const char * msg, int err_no);


public: // constructor and destructor
	CServer();
	~CServer();


public: // normal function

	void DisconnectClient(SOCKET s);
	void ProcessPacket(SOCKET s, char *packet); 

public: // Send Function
	void SendPacket(SOCKET target, void *packet);

public://threadList
	void WorkerThread();
	void AcceptThread();
	void DBThread();

public:
	void StartServer(int NumberOfWokrerThread);
	void StopServer();
};

