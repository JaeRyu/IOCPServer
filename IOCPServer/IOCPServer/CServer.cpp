#include "stdafx.h"
#include "CServer.h"



Client::Client()
{
	ZeroMemory(&exover.wsaover, sizeof(WSAOVERLAPPED));
	ZeroMemory(exover.io_buf, MAX_BUFF_SIZE);
	exover.overtype = OVER_RECV;
	exover.wsabuf.buf = exover.io_buf;
	exover.wsabuf.len = sizeof(exover.io_buf);
	packet_size = 0;
	prev_size = 0;
}

Client::Client(const Client& c)
{
	ZeroMemory(&exover.wsaover, sizeof(WSAOVERLAPPED));
	ZeroMemory(exover.io_buf, MAX_BUFF_SIZE);
	exover.overtype = OVER_RECV;
	exover.wsabuf.buf = exover.io_buf;
	exover.wsabuf.len = sizeof(exover.io_buf);
	packet_size = 0;
	prev_size = 0;
}

CServer::CServer()
{
}


CServer::~CServer()
{
}

void CServer::DisconnectClient(SOCKET s)
{
	closesocket(s);
}

void CServer::ProcessPacket(SOCKET s, char * packet)
{
	packet_form *pf = reinterpret_cast<packet_form*>(packet);

	switch (pf->type)
	{
		
	case CS_HELLO:
	{
		sc_packet_hello p;
		p.hello = 0;
		p.size = sizeof(p);
		p.type = SC_HELLO;

		SendPacket(s, &p);
	}
		break;


	}

}

void CServer::SendPacket(SOCKET target, void * packet)
{
	ExtendedOverlapped *o = new ExtendedOverlapped;
	char *p = reinterpret_cast<char *>(packet);
	memcpy_s(o->io_buf, MAX_BUFF_SIZE, packet, p[0]);
	o->overtype = OVER_SEND;
	o->wsabuf.buf = o->io_buf;
	o->wsabuf.len = p[0];
	ZeroMemory(&o->wsaover, sizeof(WSAOVERLAPPED));
	
	int ret = WSASend(target, &o->wsabuf, 1, NULL, NULL, &o->wsaover, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			err_display("Error[Send]", err_no);
	}

}

void CServer::WorkerThread()
{

	while (true)
	{
		unsigned long  data_size;
		unsigned long long key;
		WSAOVERLAPPED *p_over;

		BOOL is_success = GetQueuedCompletionStatus(g_iocp, &data_size, &key, &p_over, INFINITE);
		SOCKET sock = key;

		// handle error 
		if (0 == is_success)
		{
			int err_no = WSAGetLastError();
			err_display("Error[GQCS]", err_no);
			 
			if (ERROR_NETNAME_DELETED == err_no)
				DisconnectClient(key);
			continue;
		}

		// handle disconnect

		if (0 == data_size) {
			
			std::cout << "Client [" << key << "] Disconnected." << std::endl;
			DisconnectClient(key);
			continue;
		}

		ExtendedOverlapped *o = reinterpret_cast<ExtendedOverlapped *>(p_over);
		//auto iter = ClientList.find(sock);


		if (OVER_RECV == o->overtype)
		{
			//int recv_size = data_size;
			char *ptr = o->io_buf;

			//reassemble packet ---------------------------------------------------------------

			while (0 < data_size) // recv data is exist
			{
				if (0 == ClientList[sock].packet_size) // check it's first time get this packet
				{
					ClientList[sock].packet_size = ptr[0];
				}

				
				int remain = ClientList[sock].packet_size - ClientList[sock].prev_size; // check necessary size for complete assemble packet

				if (remain <= data_size) // can assemble packet
				{
					memcpy(ClientList[sock].prev_packet + ClientList[sock].prev_size,
						ptr, remain);

					
					//assembled packet 
					ProcessPacket(sock, ClientList[sock].prev_packet);



					data_size -= remain;
					ptr += remain;
					ClientList[sock].packet_size = 0;
					ClientList[sock].prev_size = 0;
				}
				else // can't assemble packet so, need more data
				{
					memcpy(ClientList[sock].prev_packet + ClientList[sock].prev_size, ptr, data_size);
					ClientList[sock].prev_size += data_size;
					ptr += data_size;
					data_size = 0;
					

				}


			}

			unsigned long rflag = 0;
			//ZeroMemory(&o->wsaover, sizeof(WSAOVERLAPPED));
			int ret = WSARecv(sock, &o->wsabuf, 1, NULL, &rflag, &o->wsaover, NULL);
			if (0 != ret)
			{
				int err_no = WSAGetLastError();
				if (WSA_IO_PENDING != err_no)
					err_display("Error[RECV]", err_no);
			}


		}
		else if (o->overtype == OVER_SEND)
		{
			delete o;
		}

	}
}

void CServer::AcceptThread()
{

	while (true)
	{
		SOCKADDR_IN c_addr;
		ZeroMemory(&c_addr, sizeof(SOCKADDR_IN));
		c_addr.sin_family = AF_INET;
		c_addr.sin_port = htons(SERVER_PORT);
		c_addr.sin_addr.s_addr = INADDR_ANY;

		int c_addr_len = sizeof(c_addr);

		SOCKET new_socket = WSAAccept(acceptSock, reinterpret_cast<sockaddr *>(&c_addr), &c_addr_len, NULL, NULL);


		//Add to IOCP and ClientList
		Client new_client = Client();
		new_client.s = new_socket;
		
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_socket), g_iocp, new_socket, 0);


		if (!ClientList.insert(std::make_pair(new_socket, new_client)).second)
		{
			err_quit("Accept[ClientList Insert]");
		}
		/*int option = TRUE;
		setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));*/


		
		//Change Socket Status for Recv
		unsigned long flag = 0;
		int ret = WSARecv(new_socket, &ClientList[new_socket].exover.wsabuf, 1, NULL, &flag, &ClientList[new_socket].exover.wsaover, NULL);
		if (0 != ret)
		{
			int err_no = WSAGetLastError();
			if (err_no != WSA_IO_PENDING)
				err_display("Error[Accept_WSARecv]", err_no);
		}

	}
}

void CServer::DBThread()
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;

}

void CServer::StartServer(int NumberOfWokrerThread)
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	
	//Set Socket for Accept---------------------------------------------------------------

	//char buf[10];
	acceptSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN bind_addr;
	ZeroMemory(&bind_addr, sizeof(SOCKADDR_IN));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(SERVER_PORT);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	int bind_length = sizeof(bind_addr);
	int ret;
	ret = bind(acceptSock, reinterpret_cast<sockaddr *>(&bind_addr), bind_length);

	if (0 != ret)
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
			err_display("Error[Bind]", err_no);
	}

	ret = listen(acceptSock, 1000);

	if (0 != ret)
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
			err_display("Error[Listen]", err_no);
	}

	int option = TRUE;
	setsockopt(acceptSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&option, sizeof(option));//Turn Off Nagle Algorithm

	//-------------------------------------------------------------------------------



	//Create Theads------------------------------------------------------------------
	//Always AccpetThread is 0 of NetworkthreadList
	NetworkThreadList.push_back(new std::thread{ &CServer::AcceptThread, this });

	for (int i = 0; i < NumberOfWokrerThread; ++i)
	{
		NetworkThreadList.push_back(new std::thread{ &CServer::WorkerThread, this });

	}
	//-------------------------------------------------------------------------------

	for (int i = 0; i < NetworkThreadList.size(); ++i)
	{
		NetworkThreadList[i]->join();
	}

}


void CServer::err_quit(const char * msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("Error : %s\n", msg);
	LocalFree(lpMsgBuf);
	exit(1);
}

void CServer::err_display(const char * msg, int err_no)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << "[" << msg << "]" << (char *)lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
}
