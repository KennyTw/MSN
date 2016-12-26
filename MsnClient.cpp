// MsnClient.cpp: implementation of the CMsnClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MsnClient.h"


#include <process.h>

#include "./Swsocket2/HTTPClient.h"


#include "MsnSwBoard.h"

#include <conio.h>
#include <stdlib.h>
#include <Wininet.h>

#include "md5wrapper.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMsnClient::CMsnClient()
{

	InitializeCriticalSection(&SwBoardSec);

	Connecthand = NULL;

	m_SwBoardList = NULL;
	EventListCount  = 0;

	CResourceMgr::SocketTimeout = 45;
	memset(XFRIP,0,sizeof(XFRIP));

	MsnHandle[0]  = CreateEvent(NULL,FALSE,FALSE,NULL);
	MsnHandle[1]  = CreateEvent(NULL,FALSE,FALSE,NULL);

	unsigned threadID = 0;
	MsnThread = (HANDLE) _beginthreadex(NULL, 0, MsnSwThread, (LPVOID) this, 0, &threadID);
    if (MsnThread== NULL)
    {
            fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
			throw "CreatThread failed";
             
    }


}

CMsnClient::~CMsnClient()
{
	if (SetEvent(MsnHandle[0]) == false)
	{
		int rc = WSAGetLastError();
		printf("SetEvent Error : %d",rc);		
		throw;
	}

	CloseHandle(MsnHandle[0]);
	CloseHandle(MsnHandle[1]);
	
	WaitForSingleObject(MsnThread,INFINITE);
 
	//Destory all Obj Event
	DestorySwObjList();


	DeleteCriticalSection(&SwBoardSec);



}

int CMsnClient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{

	CBaseClient::OnDataRead(sock,buf,BytesRead);
	_cprintf("OnDataRead: %s\n", buf->buf);

	MSNData *msndata = (MSNData *)sock->UserData;

	char CMD[4]={0};
	strncpy(CMD,buf->buf,3);

	char Sendcmd[20480] = {0};

	if (strcmp(CMD,"QNG") == 0)
	{
		_cprintf("QNG\r\n");
	
	} else
	if (sock->LastCmd == MSN_VER)
	{
		if ( _strnicmp(buf->buf,"VER",3) == 0)
		{
			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);

		 
			strcpy(Sendcmd,"CVR ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," 0x0804 winnt 5.0 i386 MSNMSGR 6.2.0133 MSMSGS service@softworking.com");
			//strcat(Sendcmd," 0x0409 win 4.10 i386 MSNMSGR 5.0.0544 MSMSGS dharma_kenny@yahoo.com.tw");
			

			sock->LastCmd = MSN_CVR;

			SendLn(sock,Sendcmd);
		}
	} 
	else if (sock->LastCmd == MSN_CVR)
	{
	
		if ( _strnicmp(buf->buf,"CVR",3) == 0)
		{

			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);

		 
			strcpy(Sendcmd,"USR ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," TWN I service@softworking.com");
			//strcat(Sendcmd," TWN I dharma_kenny@yahoo.com.tw");
			

			sock->LastCmd = MSN_USR;

			SendLn(sock,Sendcmd);

		}
	
	}
	else if (sock->LastCmd == MSN_USR)
	{
		if ( _strnicmp(buf->buf,"XFR",3) == 0) //Need To Redirect
		{
			MSNParseData pData;
		    memset(&pData , 0 , sizeof(pData));

			ToParseData(buf->buf,' ',&pData);

			SplitAddrPort(pData.Data[3],XFRIP , &XFRport);

			LastStatus = MSN_NEED_RELOGIN;
			return IO_NOTPOSTRECV;
		}
		else if ( _strnicmp(buf->buf,"USR",3) == 0)
		{
			char ChallengeString[2048]={0};
			char PassportURL[255]={0};
			char AuthenticationInfo[2048]={0};

			MSNParseData pData;
		    memset(&pData , 0 , sizeof(pData));

			ToParseData(buf->buf,' ',&pData);
			
		    strcpy(ChallengeString,pData.Data[4]);

		    HINTERNET IHandle=InternetOpen("MSN", INTERNET_OPEN_TYPE_PRECONFIG, 
                                 NULL, NULL, NULL);
 
		    HINTERNET IopenHandle=InternetOpenUrl(IHandle, "https://nexus.passport.com/rdr/pprdr.asp", NULL, 0, 
                                      INTERNET_FLAG_RELOAD, 0);

	 
			char Databuf[2048] = {0};
			unsigned long bytes_read = 2048;
			unsigned long HeaderIdx = 0;

			strcpy(Databuf,"PassportURLs");

			BOOL isOk = HttpQueryInfo(IopenHandle,HTTP_QUERY_CUSTOM , Databuf , &bytes_read , &HeaderIdx);

			if (isOk == TRUE)
			{
				 MSNParseData pData;
				 memset(&pData , 0 , sizeof(pData));

				 ToParseData(Databuf,',',&pData);
				  
				 MSNParseData pData2;
				 memset(&pData2 , 0 , sizeof(pData2));

				 ToParseData(pData.Data[1],'=',&pData2);

				 strcpy(PassportURL , "https://");
				 strcat(PassportURL , pData2.Data[1]); //根據前人先取第一個 URL

				 
			}

			//Close the connection
			InternetCloseHandle(IopenHandle);
			InternetCloseHandle(IHandle);

			if (PassportURL[0] != 0)
			{

				    char SendHeader[1024];

					strcpy(SendHeader , "Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=service%40softworking.com,pwd=helpme,");					
					//strcpy(SendHeader , "Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=dharma_kenny%40yahoo.com.tw,pwd=polki1,");					
					strcat(SendHeader , ChallengeString);
					strcat(SendHeader , "\r\n");

					HINTERNET IHandle2=InternetOpen("MSN", INTERNET_OPEN_TYPE_PRECONFIG, 
										 NULL, NULL, NULL);
 
					HINTERNET IopenHandle2=InternetOpenUrl(IHandle2, PassportURL, SendHeader, strlen(SendHeader), 
											  INTERNET_FLAG_NO_AUTO_REDIRECT |
											  INTERNET_FLAG_RELOAD, 0);

			 
					char Databuf[2048] = {0};
					unsigned long bytes_read = 2048;
					unsigned long HeaderIdx = 0;

					

				    //BOOL isOk = HttpQueryInfo(IopenHandle2,HTTP_QUERY_CUSTOM , Databuf , &bytes_read , &HeaderIdx);

					BOOL isOk = HttpQueryInfo(IopenHandle2,HTTP_QUERY_STATUS_CODE  , Databuf , &bytes_read , &HeaderIdx);

					if (isOk == TRUE)
					{
						  if (strcmp(Databuf,"200") == 0) //OK
						  {
							  strcpy(Databuf,"Authentication-Info");
							  bytes_read = 2048;

							  isOk = HttpQueryInfo(IopenHandle2,HTTP_QUERY_CUSTOM  , Databuf , &bytes_read , &HeaderIdx);

							  if (isOk == TRUE)
							  {

								 

								     MSNParseData pData;
									 memset(&pData , 0 , sizeof(pData));

									 ToParseData(Databuf,',',&pData);
									  
									 MSNParseData pData2;
									 memset(&pData2 , 0 , sizeof(pData2));

									 ToParseData(pData.Data[1],'=',&pData2);

									 if (pData2.Data[1][0] == '\'' )
									 {
									     char* tmpstr = pData2.Data[1];
										 int len = strlen(tmpstr);

										 strncpy(AuthenticationInfo,tmpstr+1 , len - 2);
									 }
									 
									 

								 

								
							  
							  }								
						  }
						  else if (strcmp(Databuf,"302") == 0) //Redirect
						  {
							  //uri = ServerResponse.Headers.Get("Location");
						  }

						 
					}
					
				 

					//Close the connection
					InternetCloseHandle(IopenHandle2);
					InternetCloseHandle(IHandle2);


					if (AuthenticationInfo[0] != 0)
					{
						  
						   msndata->TxnId++;
						   char cTxnId[20];
						   itoa(msndata->TxnId,cTxnId,10);

						 
						   strcpy(Sendcmd,"USR ");
						   strcat(Sendcmd,cTxnId);
						   strcat(Sendcmd," TWN S ");
						   strcat(Sendcmd,AuthenticationInfo);
							

						   sock->LastCmd = MSN_USR2;

						   SendLn(sock,Sendcmd);
									
					}

			}
		}

	}
	else if (sock->LastCmd == MSN_USR2)
	{
		char Tmpstr[50];
		strcpy(Tmpstr,"USR " );

		char cTxnId[20];
		itoa(msndata->TxnId,cTxnId,10);

		strcat(Tmpstr,cTxnId);
		strcat(Tmpstr," OK");

		if ( _strnicmp(buf->buf,Tmpstr,strlen(Tmpstr)) == 0)
		{
						   msndata->TxnId++;
						   char cTxnId[20];
						   itoa(msndata->TxnId,cTxnId,10);

						 
						   strcpy(Sendcmd,"SYN ");
						   strcat(Sendcmd,cTxnId);
						   strcat(Sendcmd," 2010 0");			   
							

						   sock->LastCmd = MSN_SYN;

						   SendLn(sock,Sendcmd);

						   
		}

	}
	else if (sock->LastCmd == MSN_SYN)
	{
						   msndata->TxnId++;
						   char cTxnId[20];
						   itoa(msndata->TxnId,cTxnId,10);

						 
						   strcpy(Sendcmd,"CHG ");
						   strcat(Sendcmd,cTxnId);
						   strcat(Sendcmd," NLN");			   
							

						   sock->LastCmd = MSN_CHG;

						   SendLn(sock,Sendcmd);

						   SetEvent(Connecthand);

						 


	}
	else if (strcmp(CMD,"RNG") == 0 )
	{
		//Connect To SwitchBoard
		 MSNParseData pData;
		 memset(&pData , 0 , sizeof(pData));

		 ToParseData(buf->buf,' ',&pData);

		 MSNParseData pData2;
		 memset(&pData2 , 0 , sizeof(pData2));

		 ToParseData(pData.Data[2],':',&pData2);

		  


		 CMsnSwBoard *sw = new CMsnSwBoard(this);
		 HANDLE hd = sw->ConnectBoard(pData2.Data[0],atoi(pData2.Data[1]),
			              "service@softworking.com",
						//  "dharma_kenny@yahoo.com.tw",
						  pData.Data[4],
						  pData.Data[1],"ANS",pData.Data[5],NULL);

		 InsertSwObjList(&sw,hd);
		 SetEvent(MsnHandle[1]);
	
	}
	else if (strcmp(CMD,"CHL") == 0 )
	{
		 char CHLstr[255];

		 MSNParseData pData;
		 memset(&pData , 0 , sizeof(pData));

		 ToParseData(buf->buf,' ',&pData);

		 strcpy(CHLstr,pData.Data[2]);
		 strcat(CHLstr,"Q1P7W2E4J9R8U3S5");

		 md5wrapper md5;
		 std::string hash1 = md5.getHashFromString(CHLstr);

		 msndata->TxnId++;
		 char cTxnId[20];
		 itoa(msndata->TxnId,cTxnId,10);

		 int slen = hash1.length();

		 char cLen[20];
		 itoa(slen,cLen,10);	
		 

		 strcpy(Sendcmd,"QRY ");
		 strcat(Sendcmd,cTxnId);
		 strcat(Sendcmd," msmsgs@msnmsgr.com ");
		 strcat(Sendcmd,cLen);
		 strcat(Sendcmd,"\r\n");
		 strcat(Sendcmd,hash1.c_str());

		 sock->LastCmd =  MSN_QRY;

		 //SendLn(sock,Sendcmd);
		 SendBuffer(sock,Sendcmd,strlen(Sendcmd));

		// creating a wrapper object
		//md5wrapper md5;

		// create a hash from a string
		//std::string hash1 = md5.getHashFromString("Hello World");

		// create a hash from a file
		//std::string hash2 = md5.getHashFromFile("readme.txt");

	}
	else if (sock->LastCmd == MSN_QRY)
	{
		if (strcmp(CMD,"QRY") == 0)
		{	
			printf("QRY OK !");
		}
	}
	else if (sock->LastCmd == MSN_XFR)
	{
		//Connect To SwitchBoard
		 MSNParseData pData;
		 memset(&pData , 0 , sizeof(pData));

		 ToParseData(buf->buf,' ',&pData);

		 MSNParseData pData2;
		 memset(&pData2 , 0 , sizeof(pData2));

		 ToParseData(pData.Data[3],':',&pData2);
		 
		 sock->LastCmd = MSN_NOTKEEP;

		 if (pData2.DataCount >= 2)
		 {


			 CMsnSwBoard *sw = new CMsnSwBoard(this);
			 HANDLE hd = sw->ConnectBoard(pData2.Data[0],atoi(pData2.Data[1]),
							  "service@softworking.com",
							 //"dharma_kenny@yahoo.com.tw",
							  pData.Data[5],
							  pData.Data[1],"USR",msndata->ToMsgEmail,msndata->SaveMsg);


			 InsertSwObjList(&sw,hd);
		     SetEvent(MsnHandle[1]);

		 }

		  
			
	}


	return NO_ERROR;
}


int CMsnClient::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{
return NO_ERROR;

}
	
int CMsnClient::OnConnected(SOCKET_OBJ* sock)
{
    _cprintf("Connect\r\n");
	IsConnected = true;

	MSNData *msndata = (MSNData *)sock->UserData;
	char Sendcmd[50] = {0};
	
	msndata->TxnId++;
	char cTxnId[20];
	itoa(msndata->TxnId,cTxnId,10);


	strcpy(Sendcmd,"VER ");
	strcat(Sendcmd,cTxnId);
	strcat(Sendcmd," MSNP10 MSNP9 CVRO");

	sock->LastCmd = MSN_VER;

	SendLn(sock,Sendcmd);
    return NO_ERROR;
}


void CMsnClient::OnThreadConnectErr(SOCKET_OBJ* sock)
{
		printf("CMsnClient OnThreadConnectErr\r\n");

}


void CMsnClient::OnDisconnect(int SockHandle)
{
	printf("CMsnClient Disconnect\r\n");
	IsConnected = false;
}


void CMsnClient::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

		printf("CMsnClient OnBeforeDisconnect\r\n");
}

void CMsnClient::HandleSocketMgr(SOCKET_OBJ *sock)
{
	SendLn(sock,"PNG");	
}

HANDLE CMsnClient::MSNLogin(char* IP , int Port)
{

	int ReturnCode;

	HANDLE ProcessHeap = GetProcessHeap();
	MSNData *userdata = (MSNData *) CHeap::GetHeap(sizeof(MSNData), &ProcessHeap);

	userdata->TxnId = 0;	

	(HANDLE ) Connecthand =  Connect(IPPROTO_TCP,IP ,Port,3000,&ReturnCode,userdata);
	
	


	if (ReturnCode != SOCKET_ERROR)
	{
		
		return Connecthand;
	}
	else
	{
		return 0;
	}

}


void CMsnClient::SplitAddrPort(char *AIP , char* IP , int* Port)
{
	int len = strlen(AIP);
	
	for (int i = 0 ; i < len ; i++)
	{
		if (AIP[i] == ':')
		{
			
			strncpy(IP,AIP,i);
			*Port = atoi(AIP+i+1);

			break;
		
		}

	}

}

void CMsnClient::ToParseData(char *AStr , char SplitChar, MSNParseData* aPData)
{
	if (AStr == NULL) return;
	
	int len = strlen(AStr);
	int ignore = 0;
	int SavePos = 0;
	bool bBreak = false;
	int QuoteCount = 0;

	for (int i = 0 ; i < len ; i++)
	{
		
		if (AStr[i] == '"' || AStr[i] == '\'')
		{
			QuoteCount ++;
		}
		
		if (AStr[i] == '(' || (AStr[i] == '[' )  )		
		{
			if (ignore == 0)
			{
				if (i - SavePos == 0)
				SavePos = i+1; 

				
			}

			if (AStr[i] == '[' && AStr[i+1] == ']' )
			{
				if (ignore != 0)
				ignore ++;
			}
			else
			{
				ignore ++;
			}
		}
		else if (AStr[i] == ')' || AStr[i] == ']'  || (ignore == 0 && AStr[i] == SplitChar) || i == len - 1 || AStr[i] == char(13))			
		//else if (AStr[i] == ')'  || (ignore == 0 && AStr[i] == ' ') || i == len - 1 || AStr[i] == char(13))			
		{
			if ((AStr[i] == ')' || AStr[i] == ']' ) && ignore > 0)
			//if (AStr[i] == ')' )
			ignore --;

			if (ignore == 0 && AStr[i] != ']' && (QuoteCount % 2) == 0)
			{
				 
					QuoteCount = 0 ;

					if (SavePos != i )
					{
						aPData->DataCount ++;
						aPData->Data[aPData->DataCount -1] =  AStr + SavePos;
						

						if (AStr[i] == char(13))
							 bBreak = true;

						if (i != len - 1 || AStr[i] == ')' || AStr[i] == ']' )
							AStr[i] = 0;

						if (bBreak) break;
					}

				SavePos = i + 1;
				 
			
			}


		}
			
	
	}
	

}

void CMsnClient::SendMsg(char* ToEmail , char* Msg)
{
	SOCKET_OBJ* sock = CResourceMgr::m_SocketList; //Client 只會有一個 connection

	if (sock == NULL) return;

	MSNData *msndata = (MSNData *)sock->UserData;
	
	char Sendcmd[4096] = {0};

			msndata->TxnId++;
			char cTxnId[20];
			itoa(msndata->TxnId,cTxnId,10);

						 
			strcpy(Sendcmd,"XFR ");
			strcat(Sendcmd,cTxnId);
			strcat(Sendcmd," SB");	
			
			sock->LastCmd = MSN_XFR;

			strcpy(msndata->ToMsgEmail,ToEmail);
			strcpy(msndata->SaveMsg , Msg);
 

   		    SendLn(sock,Sendcmd);

	
}

bool CMsnClient::GetLoginServerAddres(char* Addr , int Len)
{
	
	 

	return true;

}

void CMsnClient::InsertSwObjList(CMsnSwBoard** SwObj , HANDLE hand )
{
	EnterCriticalSection(&SwBoardSec);

	SwBoardEventList *end=NULL, 
               *ptr=NULL;

	SwBoardEventList  *obj = new SwBoardEventList; 
	EventListCount ++;
	
	 
	 
 
	obj->SwObj = *SwObj;
	obj->hand = hand;

    // Find the end of the list
    ptr = m_SwBoardList;
    if (ptr)
    {
        while (ptr->next)
            ptr = ptr->next;
        end = ptr;
    }

    obj->next = NULL;
    obj->prev = end;

    if (end == NULL)
    {
        // List is empty
        m_SwBoardList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	LeaveCriticalSection(&SwBoardSec);

}

void CMsnClient::RenumberList()
{
	EnterCriticalSection(&SwBoardSec);

	SwBoardEventList *ptr = m_SwBoardList;
	int idx = HoldEventCount ;
 

	while (ptr)
	{	
		MsnHandle[idx] = ptr->hand;
		idx ++;
	     
		ptr = ptr->next; 	
	  
	}

	LeaveCriticalSection(&SwBoardSec);

}

void CMsnClient::RemoveSwObjList(HANDLE hand)
{
	EnterCriticalSection(&SwBoardSec);

	SwBoardEventList *ptr = m_SwBoardList;
	SwBoardEventList *obj = NULL;

	while (ptr)
	{	  
	    if (ptr->hand == hand)
		{
			obj = ptr;
			break;
			
		} 
		ptr = ptr->next; 	
	  
	}

	 

	
	if (m_SwBoardList != NULL && obj != NULL)
    {
       EventListCount --;  
        // Fix up the next and prev pointers
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;

        if (m_SwBoardList == obj)
           m_SwBoardList = obj->next;

		CloseHandle(obj->hand);
		delete obj->SwObj;
		delete obj;
    }

	LeaveCriticalSection(&SwBoardSec);

}

void CMsnClient::DestorySwObjList()
{
	SwBoardEventList *ptr = m_SwBoardList;
	while (ptr)
	{	  
	     
		SwBoardEventList *nextptr = ptr->next;

		CloseHandle(ptr->hand);
		delete ptr->SwObj;

		delete ptr;
 

		ptr =  nextptr;
	}

	m_SwBoardList = NULL;

}


unsigned __stdcall MsnSwThread(LPVOID lpParam)
{

	CMsnClient *m_MsnClient = NULL;
    m_MsnClient = (CMsnClient *) lpParam;

	DWORD hEvent ;
	int HdCount = HoldEventCount;
	 

	while (1)
	{
		
		
		HdCount = m_MsnClient->EventListCount + HoldEventCount;		
		hEvent = WaitForMultipleObjects(HdCount,
			                            m_MsnClient->MsnHandle,
										FALSE,
										INFINITE);

		

		if (hEvent == WAIT_OBJECT_0)
		{
			break;		
		}
		else if (hEvent == WAIT_OBJECT_0 + 1)
		{
			m_MsnClient->RenumberList();		
		}
		else if (hEvent > WAIT_OBJECT_0 + 1)
		{
			m_MsnClient->RemoveSwObjList(
			m_MsnClient->MsnHandle[hEvent - WAIT_OBJECT_0]);


			m_MsnClient->RenumberList();

		
		}




	}



	 _endthreadex(0);
     return 0;
}
