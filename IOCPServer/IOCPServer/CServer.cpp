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
	DBQueueNumber = 0;
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
	DBQueueNumber = c.DBQueueNumber;
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
		DBExecGetUserData *db = new DBExecGetUserData;
		DBQueueList[ClientList[s].DBQueueNumber].push(db);
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

bool CServer::DBConnection(const char * ODBCName, SQLHENV & henv, SQLHDBC & hdbc, SQLHSTMT & hstmt)
{

	SQLRETURN retcode;

	/*SQLWCHAR * OutConnStr = (SQLWCHAR *)malloc(255);
	SQLSMALLINT * OutConnStrLen = (SQLSMALLINT *)malloc(255);*/

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

#ifdef UNICODE
				retcode = SQLConnect(hdbc, ConvertChar(ODBCName), SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
#else
				retcode = SQLConnect(hdbc, (SQLCHAR*)ODBCName, SQL_NTS, (SQLCHAR*)NULL, 0, NULL, 0);
#endif
				// Connect to data source  
				

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						std::cout << "DB Connect Success" << std::endl;

						return true;
					}
					else
					{
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
						return false;
					}
				}
				else
				{
					HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
					return false;
				}
			}
			else
			{
				HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
				return false;
			}
		}
		else
		{
			HandleDiagnosticRecord(henv, SQL_HANDLE_ENV, retcode);
			return false;
		}

	}
	else
	{
		HandleDiagnosticRecord(henv, SQL_HANDLE_ENV, retcode);
		return false;
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
			
			DisconnectClient(key);
			continue;
		}

		ExtendedOverlapped *o = reinterpret_cast<ExtendedOverlapped *>(p_over);
		//auto iter = ClientList.find(sock);


		if (OVER_RECV == o->overtype)
		{
			int recv_size = data_size;
			char *ptr = o->io_buf;

			//reassemble packet ---------------------------------------------------------------

			while (0 < recv_size) // recv data is exist
			{
				if (0 == ClientList[sock].packet_size) // check it's first time get this packet
				{
					ClientList[sock].packet_size = ptr[0];
				}

				
				int remain = ClientList[sock].packet_size - ClientList[sock].prev_size; // check necessary size for complete assemble packet

				if (remain <= recv_size) // can assemble packet
				{
					memcpy_s(ClientList[sock].prev_packet + ClientList[sock].prev_size,MAX_PACKET_SIZE ,
						ptr, remain);

					
					//assembled packet 
					ProcessPacket(sock, ClientList[sock].prev_packet);



					recv_size -= remain;
					ptr += remain;
					ClientList[sock].packet_size = 0;
					ClientList[sock].prev_size = 0;
				}
				else // can't assemble packet so, need more data
				{
					memcpy_s(ClientList[sock].prev_packet + ClientList[sock].prev_size,MAX_PACKET_SIZE, ptr, recv_size);
					ClientList[sock].prev_size += recv_size;
					ptr += data_size;
					recv_size = 0;
					

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
	int DBQueueCount = 1;
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
		new_client.DBQueueNumber = DBQueueCount++;
		DBQueueCount %=DBQueueList.size();
		
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

void CServer::DBThread(int ThreadIndex)
{
	int tIndex = ThreadIndex;
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;

	//DBConnection("ODBCName", henv, hdbc, hstmt);

#ifdef _DEBUG	
	std::cout << tIndex << " : DBThread Start"<< std::endl;
#endif
	
	
	//std::cout << tIndex << " : DBThread Start" << std::endl;

	DBConnection("ODBCName", henv, hdbc, hstmt);

	while(true)
	{
		
		//IDBExec *dbexec = DBQueueList[tIndex].front()
		if (true == DBQueueList[tIndex].empty())
		{
			continue;
		}
		else
		{
			DBUserData data;
			data.sqlData.hdbc = hdbc;
			data.sqlData.henv = henv;
			data.sqlData.hstmt = hstmt;
			strcpy(	data.id ,"user001");
			strcpy(data.pw, "1111");

			IDBExec* dbexec = DBQueueList[tIndex].front();
			dbexec->Excute((void *)&data);
			DBQueueList[tIndex].pop();
			delete dbexec;
		}

	}
}

void CServer::SetUpServer(int NumberOfWokrerThread, int NumberOfDBThread)
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

	for (int i = 0; i < NumberOfWokrerThread; ++i)
	{
		NetworkThreadList.push_back(new std::thread{ &CServer::WorkerThread, this });

	}
	//-------------------------------------------------------------------------------

	


	//Create DBThreads------------------------------------------------------------------

	for (int i = 0; i < NumberOfDBThread; ++i)
	{
		std::queue<IDBExec *> q;
		DBQueueList.push_back(q);
		DBThreadList.push_back(new std::thread([&]() { DBThread(i); }));
	}

	//----------------------------------------------------------------------------------





	
}

void CServer::StartServer()
{
	AcceptThreadPointer = new std::thread{ &CServer::AcceptThread, this };
}











//--------------------------------------------------------------------------------------
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

void CServer::HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
#ifdef UNICODE
		SQLSMALLINT iRec = 0;
		SQLINTEGER iError;
		WCHAR wszMessage[1000];
		WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
		if (RetCode == SQL_INVALID_HANDLE) {
			fwprintf(stderr, L"Invalid handle!\n");
			return;
		}
		while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
			(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
			// Hide data truncated..
			if (wcsncmp(wszState, L"01004", 5)) {
				fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
			}
		}
#else
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	CHAR wszMessage[1000];
	CHAR wszState[SQL_SQLSTATE_SIZE + 1];
	
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, (SQLCHAR*)wszState, &iError, (SQLCHAR*)wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (strncmp(wszState, "01004", 5)) {
			fprintf(stderr, "[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}

#endif
	
}


SOCKET CServer::GetAcceptSocket()
{
	return acceptSock;
}
HANDLE CServer::GetGlobalIOCompletionPortHandle()
{
	return g_iocp;
}
std::vector<std::thread *> CServer::GetNetworkThreadList()
{
	return NetworkThreadList;
}
std::vector<std::thread *> CServer::GetDBThreadList()
{
	return DBThreadList;
}
std::unordered_map<SOCKET, Client> CServer::GetClientList()
{
	return ClientList;
}
std::thread* CServer::GetAcceptThreadPointer()
{
	return AcceptThreadPointer;
}
