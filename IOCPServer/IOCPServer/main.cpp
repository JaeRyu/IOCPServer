#include "stdafx.h"
#include "CServer.h"
#include "mdump.h"

int main()
{
	CMiniDump::Begin();
	setlocale(LC_ALL, "korean");


	CServer server;

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	

	server.SetUpServer(sys_info.dwNumberOfProcessors, 4);

	server.StartServer();

	auto list = server.GetNetworkThreadList();

	for (auto i : list)
	{
		i->join();
	}

	CMiniDump::End();
}