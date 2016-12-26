// HTTPClient.cpp: implementation of the CHTTPClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HTTPClient.h"
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHTTPClient::CHTTPClient()
{

}

CHTTPClient::~CHTTPClient()
{

}

int CHTTPClient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	CBaseClient::OnDataRead(sock,buf,BytesRead);
	HTTPData *userdata = (HTTPData *)sock->UserData;

	int rc = NO_ERROR;

	userdata->ByteTransfer+= BytesRead;

	int OffSet = 0;
	if (sock->LastCmd == 0)
	{
		//取得 header;
		OffSet = GetTotalTranser(buf->buf,BytesRead,userdata);	
		
		if (OffSet > 0)
		{
			sock->LastCmd ++;
			userdata->ByteTransfer -= OffSet;
			
			fwrite(buf->buf + OffSet,sizeof(char),BytesRead - OffSet,userdata->fp);
		}
		else
		{
			userdata->ByteTransfer = 0;
		}

		
		
	}
	else
	{
		fwrite(buf->buf + OffSet,sizeof(char),BytesRead - OffSet,userdata->fp);
	}

	
	 

	return rc;
}

int CHTTPClient::GetTotalTranser(char* buff,int size,HTTPData *userdata)
{

	int SavePos = 0;
	for (int i= 0 ; i < size ; i ++)
	{
	
		if (strnicmp(buff+i,"100 Continue",12) == 0)
		{
				break;
		}else
		if (strnicmp(buff+i,"/1.1 404 ",9) == 0)
		{
		
			userdata->TotalTransfer = 0;
			break;
		
		} else if (strnicmp(buff+i,"Content-Length:",15) == 0)
		{						
			SavePos = i + 16;

		}else if (SavePos > 0 && buff[i] == char(13) && buff[i+1] == char(10))
		{
			char tmpbuf[255];
			strncpy(tmpbuf,buff + SavePos,i - SavePos);
			tmpbuf[i - SavePos] = 0;

			SavePos = 0;

			userdata->TotalTransfer =  atoi(tmpbuf);

			//OutputDebugString(tmpbuf);
			//OutputDebugString("\r\n");

		}
		if (buff[i] == char(13) && buff[i+1] == char(10) &&
			buff[i+2] == char(13) && buff[i+3] == char(10))
		{
		
			//if (i-1 < sizeof(Header))			
			//  strncpy(Header,buff,i);
			return i+4;
		}
	
	}

	return 0;

}

int CHTTPClient::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{
	CBaseClient::OnDataWrite(sock,buf,BytesSent);
	
	int rc = NO_ERROR;

	return rc;
}

int CHTTPClient::OnConnected(SOCKET_OBJ* sock)
{
	CBaseClient::OnConnected(sock);

	HTTPData *userdata = (HTTPData *)sock->UserData;
 

	return SendLn(sock,userdata->SendCmd);

 

}

void CHTTPClient::OnDisconnect(int SockHandle)
{
	CBaseClient::OnDisconnect(SockHandle);

	 

}

HANDLE CHTTPClient::IPGET(char* IP,char* Host, char* PathFile,char* TempFile,int *ReturnCode)
{
	HANDLE ProcessHeap = GetProcessHeap();
	HTTPData *userdata = (HTTPData *) CHeap::GetHeap(sizeof(HTTPData), &ProcessHeap);

	strcpy(userdata->SendCmd,"GET");
	strcat(userdata->SendCmd," ");
	strcat(userdata->SendCmd,PathFile);
	strcat(userdata->SendCmd," HTTP/1.1\r\n");
	strcat(userdata->SendCmd,"Host: ");
	strcat(userdata->SendCmd,Host);
	strcat(userdata->SendCmd,"\r\n"); 
	strcat(userdata->SendCmd,"Connection: Close\r\n");	
	strcat(userdata->SendCmd,"\r\n\r\n"); 

	userdata->TempFileName = TempFile;

	char TempPath[MAX_PATH];									
	GetTempPath(MAX_PATH,TempPath);

	GetTempFileName(TempPath, // directory for temp files 
        "SF",                    // temp file name prefix 
        0,                        // create unique name 
        userdata->TempFileName);              // buffer for name 

	userdata->fp = fopen(userdata->TempFileName,"w+b");

	return Connect(IPPROTO_TCP,IP,80,10 * 1000,ReturnCode,userdata);
}

void CHTTPClient::OnBeforeDisconnect(SOCKET_OBJ* sock)
{
    HTTPData *userdata = (HTTPData *)sock->UserData;
    fclose(userdata->fp);

	//確認傳輸

	if (userdata->ByteTransfer != userdata->TotalTransfer || userdata->TotalTransfer <= 0)
	{
		//傳輸有問題
		//DeleteFile(userdata->TempFileName);
		//userdata->TempFileName[0] = 0;	
	}
}
