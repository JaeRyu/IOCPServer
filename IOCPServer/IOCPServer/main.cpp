#include "stdafx.h"
#include "CServer.h"
#include "mdump.h"

int main()
{
	CMiniDump::Begin();

	CServer server;
	server.StartServer(1);

	

	CMiniDump::End();
}