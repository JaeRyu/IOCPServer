#pragma once
#include "stdafx.h"

// set interface for process db data
// it's in queue DbEvent
// and dequeue for using
// add that you want

__interface IDBExec
{
	virtual void Excute() = 0;
};



class DBExecGetUserData
{
public:
	void Excute();

};
