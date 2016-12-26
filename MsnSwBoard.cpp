// MsnSwBoard.cpp: implementation of the CMsnSwBoard class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MsnSwBoard.h"
#include <stdlib.h>
#include <conio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsnSwBoard::CMsnSwBoard(CMsnClient *Parent)
{
    //CResourceMgr::CompletionWaitTime = 1000 * 3;
	m_Parent = Parent;
}

CMsnSwBoard::~CMsnSwBoard()
{


}


int CMsnSwBoard::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{

	CBaseClient::OnDataRead(sock,buf,BytesRead);
	_cprintf("CMsnSwBoard OnDataRead: %s\n", buf->buf);

	MSNData *msndata = (MSNData *)sock->UserData;

	char CMD[4]={0};
	strncpy(CMD,buf->buf,3);

	if (strcmp(CMD,"800") == 0) //Too Busy
	{
		Sleep(1000 * 5);
		return NO_ERROR;
	
	}

	char Sendcmd[20480] = {0};

	 
	if (strcmp(CMD,"BYE") == 0)
	{
			strcpy(Sendcmd,"OUT");
			SendLn(sock,Sendcmd);
			
	
	} else
	if (sock->LastCmd == MSN_ANS || sock->LastCmd == MSN_MSG) 
	{	
		if (strstr(buf->buf,"X-MMS-IM-Format") != NULL)
		{
			//轉送
		 	char *FindStr = strstr(buf->buf,"\r\n\r\n");
			if (FindStr != NULL)
			{
				char Msg[255];
				strcpy(Msg,m_ToEmail);
				strcat(Msg,":\r\n");
				strcat(Msg,FindStr + 4);

				m_Parent->SendMsg("kenny@ezfly.com",Msg);
				//m_Parent->SendMsg("service@softworking.com",Msg);

			
			}


			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);


			WCHAR*  strA; 
			char str[255];
			
			//strcpy(str,"Hi!我是Msn Chat BOT ! 主人現在不在! 請發 mail 給他吧 ! 謝謝 !");
			strcpy(str,"Hi!我是不會聊天的 Msn Chat BOT !!正在把訊息轉送給主人.然後看看他會不會出來接客,但你放心,你的話我一定都會轉達給他的!");

		    int  i=  MultiByteToWideChar(CP_ACP,0,(char*)  str,-1,NULL,0);  
			strA  =  new  WCHAR[i];  
			MultiByteToWideChar  (CP_ACP,0,(char *) str,-1,strA,i);   
			i=  WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);  
			char  *strB=new  char[i];  
			WideCharToMultiByte  (CP_UTF8,0,strA,-1,strB,i,NULL,NULL);  
			
			
			
			char Sendcmd2[2048];
			strcpy(Sendcmd2,"MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=%E6%96%B0%E7%B4%B0%E6%98%8E%E9%AB%94; EF=; CO=0; CS=88; PF=0\r\n\r\n");
			strcat(Sendcmd2,strB);

			delete  []strA;  
			delete  []strB;

			//strcpy(Sendcmd2,"MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=%E6%96%B0%E7%B4%B0%E6%98%8E%E9%AB%94; EF=; CO=0; CS=88; PF=0\r\n\r\n");
			//strcat(Sendcmd2,"Hi.");


			char cLen[20];
			itoa(strlen(Sendcmd2),cLen,10);



			strcpy(Sendcmd,"MSG ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," N ");
			strcat(Sendcmd,cLen);
			strcat(Sendcmd,"\r\n");
			strcat(Sendcmd,Sendcmd2);

		 
			//strcpy(Sendcmd,"MSG ");
			//strcat(Sendcmd,cTxnId);
			//strcat(Sendcmd," N 183\r\nMIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=MS%20Sans%20Serif; EF=; CO=0; CS=0; PF=0\r\n\r\nHi! This is MSN Chat BOT Testing\r\nI'm not here !Sorry! :)"); 		

			sock->LastCmd = MSN_MSG;

			SendBuffer(sock,Sendcmd,strlen(Sendcmd));

		


			
		}		
	}
	else if (sock->LastCmd == MSN_USR )
	{
	
			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);
			
			strcpy(Sendcmd  , "CAL ");			
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," ");	 
			strcat(Sendcmd , m_ToEmail);
			//strcat(Sendcmd," sabin0913@msn.com");	 

			SendLn(sock,Sendcmd);

			sock->LastCmd = MSN_CAL;

	}
	else if (sock->LastCmd == MSN_CAL && strcmp(CMD,"JOI") == 0)
	{
			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);


			//WCHAR*  strA; 
			char str[255];
			strcpy(str,m_Msg);

		    /*int  i=  MultiByteToWideChar(CP_ACP,0,(char*)  str,-1,NULL,0);  
			strA  =  new  WCHAR[i];  
			MultiByteToWideChar  (CP_ACP,0,(char *) str,-1,strA,i);   
			i=  WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);  
			char  *strB=new  char[i];  
			WideCharToMultiByte  (CP_UTF8,0,strA,-1,strB,i,NULL,NULL);  */
 
			
			
			char Sendcmd2[2048];
			strcpy(Sendcmd2,"MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=%E6%96%B0%E7%B4%B0%E6%98%8E%E9%AB%94; EF=; CO=0; CS=88; PF=0\r\n\r\n");
			strcat(Sendcmd2, str);
			//strcat(Sendcmd2,strB);

			//delete  []strA;  
			//delete  []strB;

			//strcpy(Sendcmd2,"MIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=%E6%96%B0%E7%B4%B0%E6%98%8E%E9%AB%94; EF=; CO=0; CS=88; PF=0\r\n\r\n");
			//strcat(Sendcmd2,"Hi.");


			char cLen[20];
			itoa(strlen(Sendcmd2),cLen,10);



			strcpy(Sendcmd,"MSG ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," N ");
			strcat(Sendcmd,cLen);
			strcat(Sendcmd,"\r\n");
			strcat(Sendcmd,Sendcmd2);

		 
			//strcpy(Sendcmd,"MSG ");
			//strcat(Sendcmd,cTxnId);
			//strcat(Sendcmd," N 183\r\nMIME-Version: 1.0\r\nContent-Type: text/plain; charset=UTF-8\r\nX-MMS-IM-Format: FN=MS%20Sans%20Serif; EF=; CO=0; CS=0; PF=0\r\n\r\nHi! This is MSN Chat BOT Testing\r\nI'm not here !Sorry! :)"); 		

			sock->LastCmd = MSN_MSG;

			SendBuffer(sock,Sendcmd,strlen(Sendcmd));

	}
	 
	 


	return NO_ERROR;
}

int CMsnSwBoard::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{

	return NO_ERROR;
}
	
int CMsnSwBoard::OnConnected(SOCKET_OBJ* sock)
{
	MSNData *msndata = (MSNData *)sock->UserData;
	char Sendcmd[255] = {0};
	
			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);
			
			strcpy(Sendcmd  , m_FirstCmd );
			strcat(Sendcmd , " ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," ");
			strcat(Sendcmd,m_UserName);
			strcat(Sendcmd," ");
			strcat(Sendcmd,m_Session1);
				
			
			if (strcmp(m_FirstCmd , "ANS") == 0 )
			{
				strcat(Sendcmd," ");
			    strcat(Sendcmd,m_Session2);	

				sock->LastCmd = MSN_ANS;
			} else if (strcmp(m_FirstCmd , "USR") == 0 )
			{
				sock->LastCmd = MSN_USR;
			}


			

			SendLn(sock,Sendcmd);

	return NO_ERROR;
}

void CMsnSwBoard::OnThreadConnectErr(SOCKET_OBJ* sock)
{

}

void CMsnSwBoard::OnDisconnect(int SockHandle)
{
	printf("CMsnSwBoard Disconnect\r\n");
	//CResourceMgr::terminated = true;

    //CloseHandle(handle);
	//delete this;

}

void CMsnSwBoard::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}


HANDLE CMsnSwBoard::ConnectBoard(char* IP , int Port,char*  UserName,char* Session1 , char* Session2 , char *FirstCmd ,char* ToEmail ,  char* Msg )
{
	strcpy(m_UserName , UserName);
	strcpy(m_Session1 , Session1);
	strcpy(m_Session2 , Session2);
	strcpy(m_FirstCmd , FirstCmd);
	
	if (Msg != NULL)
	 strcpy(m_Msg , Msg);

	if (ToEmail != NULL)
	strcpy(m_ToEmail , ToEmail);
	 

	int ReturnCode;

	HANDLE ProcessHeap = GetProcessHeap();
	MSNData *userdata = (MSNData *) CHeap::GetHeap(sizeof(MSNData), &ProcessHeap);

	userdata->TxnId = 0;	

    HANDLE handle = Connect(IPPROTO_TCP,IP ,Port,3000,&ReturnCode,userdata);
  
	if (ReturnCode != SOCKET_ERROR)
	{
		return handle;
	}
	else
	{
		return 0;
	}

}