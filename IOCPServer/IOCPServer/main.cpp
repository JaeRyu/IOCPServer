#include "stdafx.h"
#include "CServer.h"
#include "mdump.h"

int main()
{
	CMiniDump::Begin();



	CServer server;

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	

	server.StartServer(sys_info.dwNumberOfProcessors);

	

	CMiniDump::End();
}