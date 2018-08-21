#pragma once
#include"stdafx.h"

//This For Get Data From DB

struct SQLData {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
};

struct UserData {
	SQLData sqlData;
	char id[10];
	char pw[10];
};