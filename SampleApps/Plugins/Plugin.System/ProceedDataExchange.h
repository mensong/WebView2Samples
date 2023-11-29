#ifndef __PROCEED_DATA_EXCHANGE_H   
#define __PROCEED_DATA_EXCHANGE_H

#include <windows.h>
#include <iostream>
#include <tchar.h>

using namespace std;

typedef struct
{
	long int dataOffset[10];
	long int dataID[10];
	long int arrayLen;
} DataT;

class ProceedDataExchange
{
public:
	enum RESULT
	{
		NEWDATA = 1,
		EXISTDATA = 0,

		NOEXISTDATA = -1,
		ASKFORWRITEFAIL = -2,
		ASKFORREADFAIL = -3,
	};

public:
	ProceedDataExchange(const TCHAR memoryName[], long int memorySize, BOOL bGlobal = FALSE);
	~ProceedDataExchange();

	bool isValid();

	//dataSize的读写长度需要一样才能成功
	RESULT writePackage(const void* PData, long int dataSize, long int dataID, BOOL block);//write data package
	RESULT readPackage(void* PData, long int dataSize, long int dataID, BOOL block);//read data package
	RESULT readAndWritePackage(void* pReadData, const void* pWriteData, long int dataSize, long int dataID, BOOL block);//atom read and write data package

private:
	HANDLE hMapFile;				//map handle(data map)
	HANDLE hMapFileWRLock;			//(lock map)

	HANDLE hMutexWRLock;			//(mutex handle)
	HANDLE hEventWRLock;			//(event handle)

	HANDLE lockStart;				//Memory Start handle
	HANDLE hMutexWRLockStart;		//Lock Start handle

	LPCTSTR memoryPBuf;				//memory pointer
	LPCTSTR memoryPBufWRLock;		//lock memory pointer
	DataT dataInform;

	int askWriteLock(BOOL);
	void unLockWriteLock(void);
	int askReadLock(BOOL);
	void unLockReadLock(void);

	RESULT rawWritePackage(const void* PData, long int dataSize, long int dataID);//write data package
	RESULT rawReadPackage(void* PData, long int dataSize, long int dataID);//read data package

	void release();

};

#endif	
