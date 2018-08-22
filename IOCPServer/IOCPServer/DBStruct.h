#pragma once
#include"stdafx.h"

//This For Get Data From DB

struct DBSQLData {
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
};

struct DBUserData {
	DBSQLData sqlData;
	char id[10];
	char pw[10];
};