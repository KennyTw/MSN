// POPclient.h: interface for the CPOPclient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_POPCLIENT_H__E70EFCC8_777B_45FE_9FC1_0D1B0EA2652E__INCLUDED_)
#define AFX_POPCLIENT_H__E70EFCC8_777B_45FE_9FC1_0D1B0EA2652E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CoreClass.h"

typedef struct _POPClientData
{
	char LoginId[255];
	char Password[255];
	int TotalMail;
	int index;
	char UIDL[255];
	int lastcmd;
	int *ReturnCode;
	FILE *fp;
	char *Result;
} POPClientData;

class DLLDIR CPOPclient : public CBaseClient
{
private:
	#define  NONE  0
	#define  P_USER 1
	#define  P_PASS 2
	#define  P_STAT 3
    #define  P_QUIT 4
	#define  P_RETR 5
	#define  P_LIST 6
	#define P_UIDL 7
	#define P_DELE 8
	#define P_FIRSTRETR 9

	//char* m_LoginId;
	//char* m_Password;

	

	
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );

	

public:

	CPOPclient();
	HANDLE CheckMail(char* HostIp, char* LoginId,char* Password , char *Result , int *ReturnCode);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

};

#endif // !defined(AFX_POPCLIENT_H__E70EFCC8_777B_45FE_9FC1_0D1B0EA2652E__INCLUDED_)
