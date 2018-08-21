#pragma once
#include "stdafx.h"
#include "protocol.h"
#include "DBStruct.h"
#include "DataBase.h"
#include "etcFunction.h"


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
private: // variable
	SOCKET acceptSock;
	HANDLE g_iocp;
	std::vector<std::thread *> NetworkThreadList;
	std::vector<std::thread *> DBThreadList;
	std::unordered_map<SOCKET, Client> ClientList;

	std::vector<std::queue<IDBExec>> DBQueueList;

	//std::thread* AcceptThreadPointer;

public: // Getter

	SOCKET GetAcceptSocket();
	HANDLE GetGlobalIOCompletionPortHandle();
	std::vector<std::thread *> GetNetworkThreadList();
	std::vector<std::thread *> GetDBThreadList();
	std::unordered_map<SOCKET, Client> GetClientList();


private:
	void err_quit(const char * msg);
	void err_display(const char * msg, int err_no);
	void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

public: // constructor and destructor
	CServer();
	~CServer();


public: // normal function

	void DisconnectClient(SOCKET s);
	void ProcessPacket(SOCKET s, char *packet); 

public: // Send Function
	void SendPacket(SOCKET target, void *packet);


private: // DB Function
	bool DBConnection(const char *ODBCName, SQLHENV &henv,SQLHDBC &hdbc, SQLHSTMT &hstmt);


public://threadList
	void WorkerThread();
	void AcceptThread();
	void DBThread(int Threadindex);

public:
	void StartServer(int NumberOfWokrerThread);
	void StopServer();
};

