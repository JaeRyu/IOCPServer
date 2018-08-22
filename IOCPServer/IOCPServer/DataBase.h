#pragma once
#include "stdafx.h"
#include "DBStruct.h"

// set interface for process db data
// it's in queue DbEvent
// and dequeue for using
// add that you want

class IDBExec
{
public:
	virtual bool Excute(void* ptr) = 0;
};



class DBExecGetUserData : public IDBExec
{
public:
	bool Excute(void *ptr) {
	
		DBUserData *data = reinterpret_cast<DBUserData *>(ptr);

		SQLRETURN retcode;
		retcode = SQLAllocHandle(SQL_HANDLE_STMT, data->sqlData.hdbc, &data->sqlData.hstmt);


		CHAR Query[1024] = { 0 };

		SQLCHAR szID[10]="", szPW[10]="";
		SQLINTEGER sX = 0, sY = 0;
		SQLINTEGER sHp = 0, sMp = 0;
		SQLLEN cbID = 0, cbPW = 0;
		SQLLEN cbHP = 0, cbMP = 0;


		sprintf(Query, "EXEC select_ID '%s'", data->id);
		SQLHSTMT hstmt = data->sqlData.hstmt;
		retcode = SQLExecDirect(data->sqlData.hstmt, (SQLCHAR *)Query, SQL_NTS);


		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {



			retcode = SQLBindCol(hstmt, 1, SQL_CHAR, szID, 10, &cbID);
			retcode = SQLBindCol(hstmt, 2, SQL_CHAR, szPW, 10, &cbPW);
			retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &sHp, 4, &cbHP);
			retcode = SQLBindCol(hstmt, 4, SQL_INTEGER, &sMp, 4, &cbMP);

			
				retcode = SQLFetch(hstmt);
				
				if (0 != strncmp((char *)szID, "", 10))
					return false;
				else if (0 != strncmp((char *)szID, data->id, 10))
				{
					std::cout << "Find Success" << std::endl;
					return true;
				}
				else
					return false;
			
		}
		else
		{
			return false;
		}

		return false;
	
	};
};
