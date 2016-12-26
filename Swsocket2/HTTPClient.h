// HTTPClient.h: interface for the CHTTPClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HTTPCLIENT_H__AC532967_C906_4458_A947_F77169A5D4D6__INCLUDED_)
#define AFX_HTTPCLIENT_H__AC532967_C906_4458_A947_F77169A5D4D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CoreClass.h"

typedef struct _HTTPData
{
   FILE *fp;
   char *TempFileName;
   char SendCmd[1024];
   int ByteTransfer;
   int TotalTransfer;

} HTTPData;

class DLLDIR CHTTPClient : public CBaseClient  
{
private:
	
	int GetTotalTranser(char* buff,int size,HTTPData *userdata);

public:
	CHTTPClient();
	virtual ~CHTTPClient();

	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	virtual int OnConnected(SOCKET_OBJ* sock);
	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

	HANDLE IPGET(char* IP,char* Host, char* PathFile,char* TempFile,int *ReturnCode);

};

#endif // !defined(AFX_HTTPCLIENT_H__AC532967_C906_4458_A947_F77169A5D4D6__INCLUDED_)
