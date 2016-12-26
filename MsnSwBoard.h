// MsnSwBoard.h: interface for the CMsnSwBoard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSNSWBOARD_H__7EA30FC3_F740_4377_91A4_CAB4BEF7330A__INCLUDED_)
#define AFX_MSNSWBOARD_H__7EA30FC3_F740_4377_91A4_CAB4BEF7330A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "./Swsocket2/CoreClass.h"
#include "MsnClient.h"



class CMsnSwBoard : public CBaseClient  
{

private:
	
	char m_UserName[255];
	char m_Session1[255];
	char m_Session2[255];
	char m_FirstCmd[4];
	char m_Msg[4096];
	char m_ToEmail[50];

	CMsnClient *m_Parent ; 

public:
	CMsnSwBoard(CMsnClient *Parent);
	virtual ~CMsnSwBoard();

	HANDLE ConnectBoard(char* IP , int Port , char*  UserName,char* Session1 , char* Session2 , char * FirstCmd , char* ToEmail , char* Msg  );

protected:	
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	
	virtual int OnConnected(SOCKET_OBJ* sock);
	virtual void OnThreadConnectErr(SOCKET_OBJ* sock);

	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

};

#endif // !defined(AFX_MSNSWBOARD_H__7EA30FC3_F740_4377_91A4_CAB4BEF7330A__INCLUDED_)
