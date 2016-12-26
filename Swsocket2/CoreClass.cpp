#include "stdafx.h"
#include "CoreClass.h"
#include <process.h>
#include <stdio.h>
#include <conio.h> 
#define _WIN32_DCOM 
#include <objbase.h>



CBaseClass::CBaseClass()
{
 
}
CBaseClass::~CBaseClass()
{}

CResourceMgr::CResourceMgr()
{    
	//SocketCount = 0;
	//SockObjCounter  = 0;
	//BufferObjCounter =  0;
	ResourceHeap = GetProcessHeap();
	m_SocketList = NULL;
	//terminated = false;

	SocketTimeout = 0;

	InitializeCriticalSection(&SocketListSection);

	SockMgrEvent = WSACreateEvent();
	//CompletionWaitTime = INFINITE;
}
CResourceMgr::~CResourceMgr()
{
	DeleteCriticalSection(&SocketListSection);
	WSACloseEvent(SockMgrEvent);
	SockMgrEvent = NULL;
}




BUFFER_OBJ *CResourceMgr::GetBufferObj(int buflen)
{
	//BufferObjCounter++;
	//InterlockedIncrement((long *)  &BufferObjCounter);

	//_cprintf("BufferObjCounter: %d \r\n",BufferObjCounter);

	BUFFER_OBJ *newobj=NULL;

    // Allocate the object
    newobj = (BUFFER_OBJ *) CHeap::GetHeap(sizeof(BUFFER_OBJ),&ResourceHeap);
    if (newobj == NULL)
    {
        fprintf(stderr, "GetBufferObj: HeapAlloc failed: %d\n", GetLastError());
        throw;
    }
    // Allocate the buffer
    newobj->buf = (char *) CHeap::GetHeap((sizeof(BYTE) *buflen),&ResourceHeap);
    if (newobj->buf == NULL)
    {
        fprintf(stderr, "GetBufferObj: HeapAlloc failed: %d\n", GetLastError());
        throw;
    }
	//memset(&newobj->ol, 0, sizeof(newobj->ol));

    newobj->buflen = buflen;
    

	//InterlockedIncrement(&SocketCount);

	//newobj->TimeoutEvent = (HANDLE) CreateWaitableTimer(NULL, TRUE, "WaitableTimer");


    return newobj;
}
void CResourceMgr::FreeBufferObj(BUFFER_OBJ *obj)
{
	//BufferObjCounter--;
	//InterlockedDecrement((long *)  &BufferObjCounter);

	//_cprintf("BufferObjCounter: %d \r\n",BufferObjCounter);

	
	if (obj->buf != NULL)
	{	
		CHeap::FreeHeap(obj->buf,&ResourceHeap);
		obj->buf = NULL;
		CHeap::FreeHeap(obj,&ResourceHeap);
	}

//	CloseHandle(obj->TimeoutEvent);

	//InterlockedDecrement(&SocketCount);
	//if (SocketCount != 1)
	  //printf("Free A Socket : %d\n",SocketCount);

}
SOCKET_OBJ *CResourceMgr::GetSocketObj(SOCKET s, int af , int Protocol,int SocketType)
{
	//SockObjCounter++;

	//InterlockedIncrement((long *)  &SockObjCounter);

	//_cprintf("SockObjCounter: %d \r\n",SockObjCounter);

	SOCKET_OBJ  *sockobj=NULL;

    sockobj = (SOCKET_OBJ *)  CHeap::GetHeap(sizeof(SOCKET_OBJ),&ResourceHeap);
    if (sockobj == NULL)
    {
        fprintf(stderr, "GetSocketObj: HeapAlloc failed: %d\n", GetLastError());
        throw;
    }

    
	// Initialize the members
    sockobj->s = s;
    sockobj->af = af;
	sockobj->Protocol = Protocol;
	sockobj->LastCmd = 0;
	sockobj->UserData = NULL;
	//sockobj->fp = NULL;
	sockobj->SocketType = SocketType;
	sockobj->addrlen = sizeof(sockobj->addr);
	time(&sockobj->LastUseTime);

	
//	if (SocketType == ST_FOR_NORMAL)
		
	

    // For TCP we initialize the IO count to one since the AcceptEx is posted
    //    to receive data
    //sockobj->IoCountIssued = ((sockobj->Protocol == IPPROTO_TCP) ? 1 : 0);

    InitializeCriticalSection(&sockobj->SockCritSec);
	AddSocketList(&m_SocketList,sockobj); 

    

    return sockobj;

}
void CResourceMgr::FreeSocketObj(SOCKET_OBJ *obj)
{
	BUFFER_OBJ  *ptr=NULL,
                *tmp=NULL;





	//SockObjCounter--;
	//InterlockedDecrement((long *)  &SockObjCounter);

	//_cprintf("SockObjCounter: %d \r\n",SockObjCounter);

	InterlockedIncrement((long *) &obj->bClosing);

	//if (obj->OutstandingOps != 0)
    //{
        // Still outstanding operations so just return
		//LeaveCriticalSection(&obj->SockCritSec);
      //  return;
    //}


	EnterCriticalSection(&obj->SockCritSec);

	//if (obj->OutstandingOps != 0)
    //{    
        //return;
    //}

	
	//InterlockedDecrement((long *)&obj->OutstandingOps); //故意扣 1 以防止 double free
	//if (obj->SocketType == ST_FOR_NORMAL)
	DelSocketList(&m_SocketList,obj);

   
    // Close the socket if it hasn't already been closed
    if (obj->s != INVALID_SOCKET)
    {
        
		//closesocket(obj->s);
		shutdown(obj->s,SD_BOTH);

		//LINGER       lingerStruct;   
		//lingerStruct.l_onoff  = 1;   
		//lingerStruct.l_linger =  0;   
		//setsockopt(obj->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		closesocket(obj->s);

        obj->s = INVALID_SOCKET;
    }

	//釋放 UserData
	if (obj->UserData != NULL)
	{
		CHeap::FreeHeap(obj->UserData,&ResourceHeap);
		obj->UserData = NULL;

	}

    LeaveCriticalSection(&obj->SockCritSec);
	DeleteCriticalSection(&obj->SockCritSec);



	//OutputDebugString("Before Free a Socket\n");
    CHeap::FreeHeap(obj,&ResourceHeap);

	

	//system("cls");
}
void CResourceMgr::InsertPendingSend(SOCKET_OBJ *sock, BUFFER_OBJ *send)
{
	BUFFER_OBJ *ptr=NULL,
               *prev=NULL;

/*
    EnterCriticalSection(&sock->SockCritSec);

    send->next = NULL;

    // This loop finds the place to put the send within the list.
    //    The send list is in the same order as the receives were
    //    posted.
    ptr = sock->OutOfOrderSends;
    while (ptr)
    {
        if (send->IoOrder < ptr->IoOrder)
        {
            break;
        }

        prev = ptr;
        ptr = ptr->next;
    }
    if (prev == NULL)
    {
        // Inserting at head
        sock->OutOfOrderSends = send;
        send->next = ptr;
    }
    else
    {
        // Insertion somewhere in the middle
        prev->next = send;
        send->next = ptr;
    }

    LeaveCriticalSection(&sock->SockCritSec);*/
}
int CResourceMgr::DoSends(SOCKET_OBJ *sock)
{
/*
	BUFFER_OBJ *sendobj=NULL;
    int         ret;

    ret = NO_ERROR;

    EnterCriticalSection(&sock->SockCritSec);

    sendobj = sock->OutOfOrderSends;
    while ((sendobj) && (sendobj->IoOrder == sock->LastSendIssued))
    {
        if (CCoreSocket::PostSend(sock, sendobj) != NO_ERROR)
        {
            FreeBufferObj(sendobj);
            
            ret = SOCKET_ERROR;
            break;
        }
        sock->OutOfOrderSends = sendobj = sendobj->next;
    }

    LeaveCriticalSection(&sock->SockCritSec);

    return ret;*/
	return NULL;
}


int CResourceMgr::SendLn(SOCKET_OBJ* clientobj,char* SendStr)
{

//	EnterCriticalSection(&clientobj->SockCritSec);

	int rc = 0;
	int sendlen = strlen(SendStr)+2;

	BUFFER_OBJ *sendobj = GetBufferObj(sendlen);
	memcpy(sendobj->buf, SendStr, sendlen);

	int i = 13;
	memcpy(sendobj->buf+(sendlen-2),&i,1);
	i = 10;
	memcpy(sendobj->buf+(sendlen-1),&i,1);

	if (rc = CCoreSocket::PostSend(clientobj, sendobj) != NO_ERROR)
	{
				 //OutputDebugString("CCoreSocket::PostSend 219\n");
				 FreeBufferObj(sendobj);
				 //error = SOCKET_ERROR;
	}

//	LeaveCriticalSection(&clientobj->SockCritSec);

    

	return rc;
}

int CResourceMgr::SendBufferData(SOCKET_OBJ* socketobj,char *DataBuffer , unsigned int len)
{

	int error = NO_ERROR;
	BUFFER_OBJ* sendobj = GetBufferObj(len);

	memcpy(sendobj->buf,DataBuffer,len);
	sendobj->buflen  = len;

			if (CCoreSocket::PostSend(socketobj, sendobj) != NO_ERROR)
			{
				//OutputDebugString("CResourceMgr::SendBufferObj 257\n");
				FreeBufferObj(sendobj);
            
	            error = SOCKET_ERROR;
		         
			}

	return error;


}

int CResourceMgr::SendBuffer(SOCKET_OBJ* socketobj,BUFFER_OBJ* bufferobj)
{

	int error = NO_ERROR;

	 if (CCoreSocket::PostSend(socketobj, bufferobj) != NO_ERROR)
	 {
	
				FreeBufferObj(bufferobj);            
	            error = SOCKET_ERROR;
		         
	 }

	return error;


}

int CResourceMgr::SendBufferObj(SOCKET_OBJ* clientobj,BUFFER_OBJ* sendbuf, int len)
{
	int error = NO_ERROR;

	        BUFFER_OBJ* sendobj = GetBufferObj(len);

            //if (clientobj->Protocol == IPPROTO_UDP)
            //{
            //    memcpy(&sendobj->addr, &sendobj->addr, sendbuf->addrlen);
            //}


			memcpy(sendobj->buf,sendbuf->buf,len);
			sendobj->buflen  = len;

			
            // Swap the buffers (i.e. buffer we just received becomes the send buffer)
            //tmp              = sendobj->buf;
            //sendobj->buflen  = BytesRead;
            //sendobj->buf     = buf->buf;
            

            //buf->buf    = tmp;
            //buf->buflen = DEFAULT_BUFFER_SIZE;

		    if (CCoreSocket::PostSend(clientobj, sendobj) != NO_ERROR)
			{
				//OutputDebugString("CResourceMgr::SendBufferObj 257\n");
				FreeBufferObj(sendobj);
            
	            error = SOCKET_ERROR;
		         
			}

	return error;
}

void CResourceMgr::AddSocketList(SOCKET_OBJ **head ,SOCKET_OBJ *obj)
{

	
	EnterCriticalSection(&SocketListSection);
	

	SOCKET_OBJ *end=NULL, 
               *ptr=NULL;

    // Find the end of the list
    ptr = *head;
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
        *head = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	LeaveCriticalSection(&SocketListSection);

}

void CResourceMgr::HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error)
{

}

void CResourceMgr::HandleSocketMgr(SOCKET_OBJ *sock)
{

}

/*
void CResourceMgr::DoDisconnect(SOCKET_OBJ *sock)
{
	sock->SocketType = ST_FOR_DISCONNECT;

	PostQueuedCompletionStatus (CompletionPort,0,(u_long ) sock,0);


}*/
void CResourceMgr::DelSocketList(SOCKET_OBJ **head ,SOCKET_OBJ *obj)
{
	
	EnterCriticalSection(&SocketListSection);
	

   // Make sure list isn't empty
    if (*head != NULL)
    {
        // Fix up the next and prev pointers
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;

        if (*head == obj)
            (*head) = obj->next;
    }

	LeaveCriticalSection(&SocketListSection);

}


// CCoreSocket
//////////////////////////////////////////////////////////////////////

HRESULT CCoreSocket::Startup()
{
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  return iResult;
}

void CCoreSocket::Cleanup()
{
	WSACleanup();
}

int CCoreSocket::PostRecv(SOCKET_OBJ *sock, BUFFER_OBJ *recvobj)
{

	WSABUF  wbuf;
    DWORD   bytes,
            flags;
    int     rc;

	InterlockedIncrement((long *)&sock->OutstandingOps);

    recvobj->operation = OP_READ;

    wbuf.buf = recvobj->buf;
    wbuf.len = recvobj->buflen;

    flags = 0;

    EnterCriticalSection(&sock->SockCritSec);

    // Assign the IO order to this receive. This must be performned within
    //    the critical section. The operation of assigning the IO count and posting
    //    the receive cannot be interupted.
    //recvobj->IoOrder = sock->IoCountIssued;
    //sock->IoCountIssued++;

    if (sock->Protocol == IPPROTO_TCP)
    {
        rc = WSARecv(
                sock->s,
               &wbuf,
                1,
               &bytes,
               &flags,
               &recvobj->ol,
                NULL
                );
    }
    else
    {
        rc = WSARecvFrom(
                sock->s,
               &wbuf,
                1,
               &bytes,
               &flags,
                (SOCKADDR *)&sock->addr,
               &sock->addrlen,
               &recvobj->ol,
                NULL
                );
    }

    

    if (rc == SOCKET_ERROR)
    {
        int err = WSAGetLastError();

		if (err != WSA_IO_PENDING)
        {
            _cprintf("PostRecv: WSARecv* failed: %d\n", err);
            InterlockedDecrement((long *)&sock->OutstandingOps);
			LeaveCriticalSection(&sock->SockCritSec);

			if (sock->SocketType == 0)
			{
				_cprintf("Bind sock err");
				throw "Bind sock err";
			}
			return SOCKET_ERROR;
        }
    }

    // Increment outstanding overlapped operations
    
	LeaveCriticalSection(&sock->SockCritSec);

	
	//PostQueuedCompletionStatus(
	//if (WaitForSingleObject(recvobj->TimeoutEvent, INFINITE) == WAIT_OBJECT_0)
	//{
	//	printf("TimeOut Signal\r\n");
	//}


    return NO_ERROR;
}



int CCoreSocket::PostSend(SOCKET_OBJ *sock, BUFFER_OBJ *sendobj)
{
    WSABUF  wbuf;
    DWORD   bytes;
    int     rc;

	InterlockedIncrement((long *)&sock->OutstandingOps);
    sendobj->operation = OP_WRITE;

    wbuf.buf = sendobj->buf;
    wbuf.len = sendobj->buflen;

    EnterCriticalSection(&sock->SockCritSec);

    // Incrmenting the last send issued and issuing the send should not be
    //    interuptable.
    //sock->LastSendIssued++;

    if (sock->Protocol == IPPROTO_TCP)
    {
        rc = WSASend(
                sock->s,
               &wbuf,
                1,
               &bytes,
                0,
               &sendobj->ol,
                NULL
                );
    }
    else
    {
        rc = WSASendTo(
                sock->s,
               &wbuf,
                1,
               &bytes,
                0,
                (SOCKADDR *)&sock->addr,
                sock->addrlen,
               &sendobj->ol,
                NULL
                );
    }

    

    if (rc == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf("PostSend: WSASend* failed: %d\n", WSAGetLastError());
            InterlockedDecrement((long *)&sock->OutstandingOps);
			LeaveCriticalSection(&sock->SockCritSec);
			return SOCKET_ERROR;
        }
    }

    // Increment the outstanding operation count
    //InterlockedIncrement((long *)&sock->OutstandingOps);

	LeaveCriticalSection(&sock->SockCritSec);
    return NO_ERROR;
}

SOCKET CCoreSocket::Connect(int protocol , char* RemoteIp , int port , int timeout)
{
    SOCKET s;
	sockaddr_in saRemote;



	int rc = NO_ERROR;

	if (protocol == IPPROTO_UDP)
	{
        s =  socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ;
		//saRemote.sin_family = AF_INET;
		//saRemote.sin_port = htons(port);
		//saRemote.sin_addr.s_addr = htonl(INADDR_ANY);
  
		//bind(s, (SOCKADDR *) &saRemote, sizeof(saRemote));

    }
	else
    {
		s =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

		WSAEVENT ConnectEvent =  WSACreateEvent();
		rc = WSAEventSelect(s , ConnectEvent , FD_CONNECT);

		saRemote.sin_family = AF_INET;
		saRemote.sin_port =  htons(port);
		saRemote.sin_addr.s_addr = inet_addr(RemoteIp);

		rc = connect(s,(SOCKADDR*) &saRemote,sizeof(saRemote));
		if (rc == SOCKET_ERROR)
		{

			
			int LastError = WSAGetLastError();
				if ( LastError !=  WSAEWOULDBLOCK)
				{
					//#ifdef _CONSOLEDBG
					printf("socket Connect failed: %d\n",LastError);
					//#endif
					WSACloseEvent(ConnectEvent);
					//closesocket(s);

					shutdown(s,SD_BOTH);

					//LINGER       lingerStruct;   
					//lingerStruct.l_onoff  = 1;   
					//lingerStruct.l_linger =  0;   
					//setsockopt(s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

					closesocket(s);
            
					return SOCKET_ERROR;
				}
			}

			WSANETWORKEVENTS NetworkEvents;
			int RecvCode = WSAWaitForMultipleEvents(1,&ConnectEvent, false, timeout, false);
			if (RecvCode == WSA_WAIT_EVENT_0)
			{

		
				RecvCode = WSAEnumNetworkEvents(s, ConnectEvent, &NetworkEvents);

				if  (RecvCode == SOCKET_ERROR)
				{
               //WSACloseEvent(ConnectEvent);
               
				
					rc = SOCKET_ERROR;
               
				}
				else
				{


					if ((NetworkEvents.lNetworkEvents & FD_CONNECT) && (NetworkEvents.iErrorCode[FD_CONNECT_BIT] == 0))
					{
						// clear select
						//Result  :=   NO_ERROR;
						//Result := PostRecv(buffobj,OP,0);
						// return NO_ERROR;
						rc = NO_ERROR;
					}
					else
					{
						rc = SOCKET_ERROR;
					}

				}

			}
			else if  (RecvCode == WSA_WAIT_TIMEOUT)
			{
				rc =  SOCKET_ERROR;
          //exit;
			} else if (RecvCode == WSA_WAIT_FAILED)
			{
				int LastError = WSAGetLastError();
           
				#ifdef _CONSOLEDBG
				_cprintf("WSA_WAIT_FAILED failed: %d\n",LastError);
				#endif

				rc =  SOCKET_ERROR;
					
          
			}



			WSAEventSelect(s , ConnectEvent , 0);
			WSACloseEvent(ConnectEvent);

	
	//}

	if (rc != NO_ERROR)
	{
		

		shutdown(s,SD_BOTH);

	//	LINGER       lingerStruct;   
//		lingerStruct.l_onoff  = 1;   
//		lingerStruct.l_linger =  0;   
//		setsockopt(s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		closesocket(s);

		s = SOCKET_ERROR;
	}

	return s;
}

void CCoreSocket::IpReverse(char *IP)
{

		int len = strlen(IP);
		char Zone1[4];
		char Zone2[4];
		char Zone3[4];
		char Zone4[4];

		memset(Zone1,0,4);
		memset(Zone2,0,4);
		memset(Zone3,0,4);
		memset(Zone4,0,4);

		int ZoneIdx = 0;
		int SavePos = 0;

		for(int i= 0 ; i < len ; i++)
		{
			if (IP[i] == '.')
			{
			
				if (ZoneIdx == 0)
				{					
					strncpy(Zone4,IP + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 1)
				{
					strncpy(Zone3,IP + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 2)
				{
					strncpy(Zone2,IP + SavePos, i - SavePos);
					strncpy(Zone1,IP + i + 1, len-i);
					 
					break;
				}
			 

				ZoneIdx ++;
				SavePos = i + 1;

			}
		}

		strcpy(IP,Zone1);
		strcat(IP,".");
		strcat(IP,Zone2);
		strcat(IP,".");
		strcat(IP,Zone3);
		strcat(IP,".");
		strcat(IP,Zone4);
	 

}

int CCoreSocket::PostConnect(SOCKET_OBJ *sock, BUFFER_OBJ *connobj)
{
/*
	DWORD   bytes=0;
    int     rc;

    connobj->operation = OP_CONNECT; 
	

    rc = sock->lpfnConnectEx(
            sock->s,
            (SOCKADDR *)&sock->addr,
            sock->addrlen,
            NULL,//connobj->buf,
            0,//connobj->buflen,
           &bytes,
           &connobj->ol
            );
    if (rc == FALSE)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            fprintf(stderr, "PostConnect: ConnectEx failed: %d\n",
                    WSAGetLastError());
            return SOCKET_ERROR;
        }
    }

    // Increment the outstanding overlapped count for this socket
    InterlockedIncrement((long *)&sock->OutstandingOps);

    //printf("POST_CONNECT: op %d\n", sock->OutstandingOps);
*/
    return NO_ERROR;


}


int CCoreSocket::PostAccept(SOCKET_OBJ *sock, BUFFER_OBJ *acceptobj)
{
   DWORD   bytes;
    int     rc;

	
    InterlockedIncrement((long *)&sock->OutstandingOps);	
	acceptobj->operation = OP_ACCEPT;

	
    // Create the client socket for an incoming connection
    acceptobj->sclient = socket(sock->af, SOCK_STREAM, IPPROTO_TCP);
	
    if (acceptobj->sclient == INVALID_SOCKET)
    {
        fprintf(stderr, "PostAccept: socket failed: %d\n", WSAGetLastError());
        return -1;
    }

	EnterCriticalSection(&sock->SockCritSec);
    /*
    rc = sock->lpfnAcceptEx(
            sock->s,
            acceptobj->sclient,
            acceptobj->buf,
            acceptobj->buflen - ((sizeof(sockaddr_in) + 16) * 2),
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
           &bytes,
           &acceptobj->ol
            );*/

	rc = sock->lpfnAcceptEx(
            sock->s,
            acceptobj->sclient,
            acceptobj->buf,
            0,
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
           &bytes,
           &acceptobj->ol
            );

    if (rc == FALSE)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf("PostAccept: AcceptEx failed: %d\n",
                    WSAGetLastError());

			InterlockedDecrement((long *)&sock->OutstandingOps);
            LeaveCriticalSection(&sock->SockCritSec);
			return SOCKET_ERROR;
        }
    }

    // Increment the outstanding overlapped count for this socket
    

	LeaveCriticalSection(&sock->SockCritSec);
    return NO_ERROR;
}

/*
int CCoreSocket::PrintAddress(SOCKADDR *sa, int salen)
{

    char    host[1025],
            serv[32];
    int     hostlen = 1025,
            servlen = 32,
            rc;

    rc = getnameinfo(
            sa,
            salen,
            host,
            hostlen,
            serv,
            servlen,
            1025 | 32
            );
    if (rc != 0)
    {
        fprintf(stderr, "%s: getnameinfo failed: %d\n", __FILE__, rc);
        return rc;
    }

    // If the port is zero then don't print it
    if (strcmp(serv, "0") != 0)
    {
        if (sa->sa_family == AF_INET)
            printf("[%s]:%s", host, serv);
        else
            printf("%s:%s", host, serv);
    }
    else
        printf("%s", host);

    return NO_ERROR;
}
*/
// CHEAP 記憶體處理
//////////////////////////////////////////////////////////////////////
 
void *CHeap::GetHeap(int size, HANDLE* ResourceHeap )
{
  // char logstr[50];
  // wsprintf(logstr,"GetHeap +%d\n",size);
  // OutputDebugString(logstr);

  
  
   //|| HEAP_GENERATE_EXCEPTIONS
   void *HeapChar = HeapAlloc(*ResourceHeap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS  , size);   
   if (HeapChar == NULL)
   { 
	    OutputDebugString("HeapAlloc NULL!");
		throw ;//"HeapAlloc NULL!";		
   } 
   
    
   
   return HeapChar;
}

int CHeap::GetHeapSize(void *buf, HANDLE* ResourceHeap)
{

	unsigned int size = HeapSize(*ResourceHeap,0,buf);
	return size;
}

void CHeap::FreeHeap(void *obj, HANDLE* ResourceHeap)
{
  
  //char tmp[50];
  //wsprintf(tmp,"DelHeap -%d\n",GetHeapSize(obj,ResourceHeap));
  //OutputDebugString(tmp);
	
   
   
   if (!HeapFree(*ResourceHeap, 0, obj))
   {
       OutputDebugString("HeapFree Error!\n");
	   throw;// "HeapFree Error";
   } 

   obj = NULL;

}

unsigned __stdcall SocketMgrThread(LPVOID lpParam)
{

	CResourceMgr *ResourceMgr = NULL;
    ResourceMgr = (CResourceMgr *) lpParam;

//	ResourceMgr->SockMgrEvent = WSACreateEvent();
     
	int rc = 0;
	 
    while (1)
    {
 	 
		//Sleep(5000);
		rc = WaitForSingleObject(ResourceMgr->SockMgrEvent,5000);

		if (rc == WAIT_TIMEOUT)
		{

				time_t   now;
				time(&now);

				//Add 1
				//ResourceMgr->SocketTimeout = 45;

			 
				if (ResourceMgr->SocketTimeout > 0)
				{
					SOCKET_OBJ *ptrs = ResourceMgr->m_SocketList;

					//EnterCriticalSection(&ResourceMgr->SocketListSection);
					//會造成 deadlock
					while (ptrs)
					{	  
						
						if (ptrs->SocketType == ST_FOR_NORMAL)
						{
							if (difftime(now,ptrs->LastUseTime) > ResourceMgr->SocketTimeout)// > 30 secs
							{
								
								//closesocket(ptrs->s);
								//ptrs->s = INVALID_SOCKET;	
								
								CRITICAL_SECTION     m_SockCritSec = ptrs->SockCritSec;
								SOCKET_OBJ *Nextptrs = ptrs->next;

								EnterCriticalSection(&m_SockCritSec);								
								ResourceMgr->HandleSocketMgr(ptrs);
								LeaveCriticalSection(&m_SockCritSec);

								ptrs = Nextptrs;

								continue;
							

							}
						}				   
						
						ptrs = ptrs->next;
					}

				//	LeaveCriticalSection(&ResourceMgr->SocketListSection);
				}
		}
		else
		{
			break;
		}
	}
	
	 
	 //WSACloseEvent(ResourceMgr->SockMgrEvent);
	 //ResourceMgr->SockMgrEvent = NULL;

	 _endthreadex(0);
     return 0;
}

// SetThreadName
//////////////////////////////////////////////////////////////////////
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = szThreadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
   }
   __except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}
// CompletionThread
//////////////////////////////////////////////////////////////////////
unsigned __stdcall CompletionThread(LPVOID lpParam)
{
    SOCKET_OBJ  *sockobj=NULL;          // Per socket object for completed I/O
    BUFFER_OBJ  *bufobj=NULL;           // Per I/O object for completed I/O
    OVERLAPPED  *lpOverlapped=NULL;     // Pointer to overlapped structure for completed I/O
    HANDLE       CompletionPort;        // Completion port handle
    DWORD        BytesTransfered;       // Number of bytes transfered
                                  // Flags for completed I/O
    int          rc, 
                 error;

	

        
	CoInitializeEx(NULL,COINIT_MULTITHREADED); //for ado use
	

	CResourceMgr *ResourceMgr = NULL;

    ResourceMgr = (CResourceMgr *) lpParam;
    CompletionPort = ResourceMgr->CompletionPort;

    //while (!ResourceMgr->terminated)
	while (1)
    {
        error = NO_ERROR;
		BytesTransfered = 0;
		sockobj = NULL;
		lpOverlapped = NULL;

        rc = GetQueuedCompletionStatus(
                CompletionPort,
               &BytesTransfered,
                (u_long *)&sockobj,
               &lpOverlapped,
			  // 1000 * 1
                INFINITE
                );

		//一但設定 CompletionWaitTime 
		// PostQueuedCompletionStatus (CompletionPort,0,0,0); 將會失效 

		if (BytesTransfered == 0 && sockobj == NULL && lpOverlapped == NULL)
		{
			OutputDebugString("GetQueuedCompletion Break \r\n");
	 
			break;
		}

		if (rc == FALSE)
		{
		
			error = GetLastError();
			/*if (error == WAIT_TIMEOUT)
			{
				if (ResourceMgr->terminated)
				{
					_cprintf("Timeout exit ...\r\n");
					break;				
				}
				else
				{
					_cprintf("Timeout Continue ...\r\n");
					continue;
				}
			
			}*/
		
		}

		

		

        bufobj = CONTAINING_RECORD(lpOverlapped, BUFFER_OBJ, ol);

        //if (rc == FALSE)
        //{
            // If the call fails, call WSAGetOverlappedResult to translate the
            //    error code into a Winsock error code.

			//error = GetLastError();

            //printf("CompletionThread: GetQueuedCompletionStatus failed: %d\n",
                    //error);

		
            /*rc = WSAGetOverlappedResult(
                    sockobj->s,
                   &bufobj->ol,
                   &BytesTransfered,
                    FALSE,
                   &Flags
                    );
            if (rc == FALSE)
            {
                error = WSAGetLastError();
				printf("CompletionThread: GetQueuedCompletionStatus failed2: %d\n",
                    error);
            }*/

			/*
			ResourceMgr->FreeBufferObj(bufobj);


			if (InterlockedDecrement(&sockobj->OutstandingOps) == 0)
			{
				printf("Freeing socket obj in GetOverlappedResult\n");
			
				int sockhandle = sockobj->s;

				//OnBeforeDisconnect(sock);
				WSAEVENT WaitEvent = sockobj->ConnectWaitEvent;

				
				ResourceMgr->FreeSocketObj(sockobj);
				//ResourceMgr->OnDisconnect(sockhandle);
				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
					WSACloseEvent(WaitEvent);
				}

			}*/
       // } 


	/*if (!ResourceMgr->terminated && sockobj->OutstandingOps == 0) 
	{
		_cprintf("Oh no\n");
		//DebugBreak();

		RaiseException(1,         // exception code 
                0,                    // continuable exception 
                0, NULL);             // no arguments 

		//throw;

		
	}*/
		
		//if (!ResourceMgr->terminated && sockobj->OutstandingOps > 0) // Handle the IO operation
	//	if (!ResourceMgr->terminated)
		if (error == ERROR_OPERATION_ABORTED)
		{	
			
			//ResourceMgr->FreeBufferObj(bufobj);	

			//WSAEVENT WaitEvent = sockobj->ConnectWaitEvent;			
			//ResourceMgr->FreeSocketObj(sockobj);
			
			//WSASetEvent(WaitEvent);
			_cprintf("ERROR_OPERATION_ABORTED...\r\n");
			//break; //先處理為先離開
			//throw "ERROR_OPERATION_ABORTED";
			continue;
			//break;
		
		}
		else
		{
		//	EnterCriticalSection(&sockobj->SockCritSec);
			int outs = sockobj->OutstandingOps;
		//	LeaveCriticalSection(&sockobj->SockCritSec);

			if (outs == 0)
			{
				_cprintf("sockobj->OutstandingOps 0...\r\n");
				throw "sockobj->OutstandingOps 0";
			}
			else
			{
	
			  	ResourceMgr->HandleIo(sockobj, bufobj, CompletionPort, BytesTransfered, error);
			  
			
			}
		}
		
    }


	//


	CoUninitialize();
    
    _endthreadex(0);
    return 0;
}

unsigned __stdcall ConntectThread(LPVOID lpParam)
{
    
	
    //ConnectThreadData *ThreadData =  (ConnectThreadData *)  lpParam ;
	ConnectThread_Data *ThreadData = (ConnectThread_Data *) lpParam;
	//CSMTPClient *smtp = (CSMTPClient *) ThreadData->parent; 
	CBaseClient *base = (CBaseClient *) ThreadData->parent; 


	//char hostIP[20];
	//hostIP[0] =  0 ; 

	HANDLE ProcessHeap = GetProcessHeap();
	//SMTPMailData *userdata = (SMTPMailData *) CHeap::GetHeap(sizeof(SMTPMailData), &ProcessHeap);

	//if (userdata != NULL)
	//{
		//smtp->ReadArgumentFile(userdata, ThreadData->filearg);
		//userdata->Result = ThreadData->SendResult;
		//userdata->feml = fopen(ThreadData->fileml, "rb");		
		//strcpy(userdata->HostDomainName,ThreadData->HostDomainName);

		//char Email[255];
		//memset(Email,0,255);		
		
		//smtp->GetRcptToBySeq(userdata, Email, 0);

		//printf("To Email : %s\r\n",Email);

		//找到 domain
		//char *Domain = NULL;
		//Domain = strstr(Email,"@");
		//if (Domain != NULL)
		//{
		//	Domain++; //shift right		
	
	if (ThreadData->RemoteDomain[0] != 0)
	{
			CDnsClient dns;	 
			int rc = NO_ERROR;

			HANDLE ch = dns.Resolve(ThreadData->DnsServerIP,(char *)ThreadData->RemoteDomain,qtMX,ThreadData->RemoteIp,&rc);	 
			WaitForSingleObject(ch,1000 * 5);

			CloseHandle(ch);
			
			printf("Dns for Resolve : %s  , IP: %s\r\n",ThreadData->RemoteDomain,ThreadData->RemoteIp);
		//}
	//}

	}

	if (ThreadData->RemoteIp[0] != 0 )
	{
		
		
		base->HandleConnect(ThreadData->waitevent,ThreadData->sock,ThreadData->RemoteIp,ThreadData->RemotePort,ThreadData->Timeout,ThreadData->ReturnCode,ThreadData->sock->UserData);

		if (*ThreadData->ReturnCode == SOCKET_ERROR)
		{
			//fclose(userdata->feml);
			//delete userdata->MailFrom;
			//delete userdata->RcptTo;

			//CHeap::FreeHeap(userdata->MailFrom,&ProcessHeap);
			//CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);

			base->OnThreadConnectErr(ThreadData->sock);
			SetEvent(ThreadData->waitevent);
		}

		goto end;
	}
	else
	{
		base->OnThreadConnectErr(ThreadData->sock);
		SetEvent(ThreadData->waitevent);
	}
	/*else
	{
		fclose(userdata->feml);	
		CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);

		CHeap::FreeHeap(userdata,&ProcessHeap);

		strcpy(ThreadData->SendResult,"DNS Lookup Error");
		*ThreadData->ReturnCode = SOCKET_ERROR;

		SetEvent(ThreadData->waitevent);
		
		goto end;
	}*/

end:
	
	//Sleep(10000);
	//WaitForSingleObject(ThreadData->waitevent,INFINITE);

	CHeap::FreeHeap(ThreadData,&ProcessHeap);    
	CloseHandle(GetCurrentThread());


	_endthreadex(0);
    return 0;
}

 
// CompletionThread
//////////////////////////////////////////////////////////////////////
CBaseServer::CBaseServer(int Protocol , int ListenPort)
{
    m_Protocol = Protocol;
    m_ListenPort = ListenPort;
	SocketMgr = NULL;
	CPUNum =0;
	CompletionPort = NULL;
}

CBaseServer::~CBaseServer()
{

	//terminated = true;


	SetEvent(SockMgrEvent);
	WaitForSingleObject(SocketMgr,INFINITE);
	CloseHandle(SockMgrEvent);

	SOCKET_OBJ *ptrs = m_SocketList;			

	while (ptrs)
	{							
		shutdown(ptrs->s,SD_BOTH);

		//LINGER       lingerStruct;   
		//lingerStruct.l_onoff  = 1;   
		//lingerStruct.l_linger =  0;   
		//setsockopt(ptrs->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		//closesocket(s);

		
		closesocket(ptrs->s);
		
		ptrs->s = INVALID_SOCKET;
		
		  
		ptrs = ptrs->next;
	}
	

	HANDLE  ThreadHandle[MAX_COMPLETION_THREAD_COUNT];

	for(int i=0 ; i < CPUNum ; i++)
	{
		ThreadHandle[i] = CompThreads[i];
		PostQueuedCompletionStatus (CompletionPort,0,0,0);
	}

	//ThreadHandle[CPUNum] = SocketMgr;	

	WaitForMultipleObjects(CPUNum,ThreadHandle,TRUE,INFINITE);

	for(int j=0 ; j < CPUNum ; j++)
	{
		CloseHandle(ThreadHandle[j]);
	}

	if (CompletionPort != NULL)
	CloseHandle(CompletionPort);

 

}

int CBaseServer::StartServer()
{

	HANDLE hrc;
	unsigned threadID = 0;
	
	SOCKET_OBJ      *sockobj=NULL;
                    //*ListenSockets=NULL;

	int rc;
	sockaddr_in sa;


    CCoreSocket::Startup();



	sa.sin_family = AF_INET;//寫死
	sa.sin_port = htons(m_ListenPort);
	sa.sin_addr.S_un.S_addr = INADDR_ANY;


	

	

	GetSystemInfo(&sysinfo);

	

    if (sysinfo.dwNumberOfProcessors > MAX_COMPLETION_THREAD_COUNT)
    {
        sysinfo.dwNumberOfProcessors = MAX_COMPLETION_THREAD_COUNT;
    }

	CPUNum = sysinfo.dwNumberOfProcessors * DEFAULT_COMPLETION_CPU;
	//CPUNum = sysinfo.dwNumberOfProcessors ;

	 // Create the completion port used by this server
    CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)NULL, CPUNum);
    if (CompletionPort == NULL)
    {
        fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
        return -1;
    }

      // Create the worker threads to service the completion notifications
    for(int i=0; i < (int) CPUNum  ;i++)
    {
        	
		
		CompThreads[i] = (HANDLE) _beginthreadex(NULL, 0, CompletionThread, (LPVOID) this, 0, &threadID);
        
		SetThreadName(threadID,"BThread1");
		
		if (CompThreads[i] == NULL)
        {
            fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
            return -1;
        }

	}

	// Create socket Msg Thread
	 

	SocketMgr = (HANDLE) _beginthreadex(NULL, 0, SocketMgrThread, (LPVOID) this, 0, &threadID);

	SetThreadName(threadID,"BThread2");

 
		sockobj = GetSocketObj(INVALID_SOCKET, sa.sin_family,m_Protocol,ST_FOR_ACCEPT);

        // create the socket
		if (sockobj->Protocol == IPPROTO_UDP)
		{		
				sockobj->s = socket(sockobj->af, SOCK_DGRAM, sockobj->Protocol);
		}
		else
		{		
			sockobj->s = socket(sockobj->af, SOCK_STREAM, sockobj->Protocol);
			//sockobj->MainListenSock = sockobj->s;
		}
        
        if (sockobj->s == INVALID_SOCKET)
        {
            fprintf(stderr,"socket failed: %d\n", WSAGetLastError());
            return -1;
        }

        // Associate the socket and its SOCKET_OBJ to the completion port
        hrc = CreateIoCompletionPort((HANDLE)sockobj->s, CompletionPort, (u_long)sockobj, 0);
        if (hrc == NULL)
        {
            fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
            return -1;
        }
		 // bind the socket to a local address and port
        rc = bind(sockobj->s, (SOCKADDR*) &sa, sizeof(sa));
        if (rc == SOCKET_ERROR)
        {
            fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
            return -1;
        }

		if (sockobj->Protocol == IPPROTO_TCP)
        {
            BUFFER_OBJ *acceptobj=NULL;
            GUID        guidAcceptEx = WSAID_ACCEPTEX,
                        guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
            DWORD       bytes;

            // Need to load the Winsock extension functions from each provider
            //    -- e.g. AF_INET and AF_INET6. 
            rc = WSAIoctl(
                    sockobj->s,
                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                   &guidAcceptEx,
                    sizeof(guidAcceptEx),
                   &sockobj->lpfnAcceptEx,
                    sizeof(sockobj->lpfnAcceptEx),
                   &bytes,
                    NULL,
                    NULL
                    );
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed: %d\n",
                        WSAGetLastError());
                return -1;
            }
            rc = WSAIoctl(
                    sockobj->s,
                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                   &guidGetAcceptExSockaddrs,
                    sizeof(guidGetAcceptExSockaddrs),
                   &sockobj->lpfnGetAcceptExSockaddrs,
                    sizeof(sockobj->lpfnGetAcceptExSockaddrs),
                   &bytes,
                    NULL,
                    NULL
                    );
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER faled: %d\n",
                        WSAGetLastError());
                return -1;
            }

            // For TCP sockets, we need to "listen" on them
            rc = listen(sockobj->s, DEFAULT_ACP_OVERLAPPED_COUNT);
			
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
                return -1;
            }

            // Keep track of the pending AcceptEx operations
            //sockobj->PendingAccepts = (BUFFER_OBJ **)HeapAlloc(
            //        GetProcessHeap(), 
             //       HEAP_ZERO_MEMORY, 
            //        (sizeof(BUFFER_OBJ *) * DEFAULT_OVERLAPPED_COUNT));
			sockobj->PendingAccepts = (BUFFER_OBJ **) CHeap::GetHeap((sizeof(BUFFER_OBJ *) * DEFAULT_ACP_OVERLAPPED_COUNT),&ResourceHeap);
            if (sockobj->PendingAccepts == NULL)
            {
                fprintf(stderr, "HeapAlloc failed: %d\n", GetLastError());
                throw;
            }

            // Post the AcceptEx(s)
            for(int j=0; j < DEFAULT_ACP_OVERLAPPED_COUNT ;j++)
            {
                sockobj->PendingAccepts[j] = acceptobj = GetBufferObj(DEFAULT_BUFFER_SIZE);

				CCoreSocket::PostAccept(sockobj, acceptobj);
				
            }
            //
            // Maintain a list of the listening socket structures
            //
			/*
            if (ListenSockets == NULL)
            {
                ListenSockets = sockobj;
            }
            else
            {
                sockobj->next = ListenSockets;
                ListenSockets = sockobj;
            }*/
        }
        else
        {
            //UDP 
			
			BUFFER_OBJ *recvobj=NULL;
            DWORD       bytes=0;
            int         optval=0;

            // Turn off UDP errors resulting from ICMP messages (port/host unreachable, etc)
            /*
			optval = 0;
            rc = WSAIoctl(
                    sockobj->s,
                    SIO_UDP_CONNRESET,
                   &optval,
                    sizeof(optval),
                    NULL,
                    0,
                   &bytes,
                    NULL,
                    NULL
                    );
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "WSAIoctl: SIO_UDP_CONNRESET failed: %d\n", 
                        WSAGetLastError());
            }*/

            // For UDP, simply post some receives
            for(i=0; i < DEFAULT_OVERLAPPED_COUNT ;i++)
            {
                recvobj = GetBufferObj(DEFAULT_BUFFER_SIZE);

                CCoreSocket::PostRecv(sockobj, recvobj);
            }
        }
    
    //WaitForSingleObject(m_ServerWaitEvent, INFINITE);

    /*
	int interval = 0;
    while (1)
    {
        rc = WSAWaitForMultipleEvents(
                sysinfo.dwNumberOfProcessors ,
                CompThreads,
                TRUE,
                5000,
                FALSE
                );
        if (rc == WAIT_FAILED)
        {
            fprintf(stderr, "WSAWaitForMultipleEvents failed: %d\n", WSAGetLastError());
            break;
        }
        else if (rc == WAIT_TIMEOUT)
        {
            interval++;

            //PrintStatistics();

            if (interval == 12)
            {
                SOCKET_OBJ  *listenptr=NULL;
                int          optval,
                optlen;

                // For TCP, cycle through all the outstanding AcceptEx operations
                //   to see if any of the client sockets have been connected but
                //   haven't received any data. If so, close them as they could be
                //   a denial of service attack.
                listenptr = ListenSockets;
                while (listenptr)
                {
                    for(i=0; i < DEFAULT_OVERLAPPED_COUNT ;i++)
                    {
                        optlen = sizeof(optval);
                        rc = getsockopt(
                                listenptr->PendingAccepts[i]->sclient,
                                SOL_SOCKET,
                                SO_CONNECT_TIME,
                                (char *)&optval,
                                &optlen
                                       );
                        if (rc == SOCKET_ERROR)
                        {
                            fprintf(stderr, "getsockopt: SO_CONNECT_TIME failed: %d\n",
                                    WSAGetLastError());
                            return -1;
                        }
                        // If the socket has been connected for more than 5 minutes,
                        //    close it. If closed, the AcceptEx call will fail in the
                        //    completion thread.
                        if ((optval != 0xFFFFFFFF) && (optval > 300))
                        {
                            closesocket(listenptr->PendingAccepts[i]->sclient);
                        }
						else
						{
						
							printf("Pending Accept (%d) optval : %d \n",listenptr->PendingAccepts[i]->sclient,optval);
						}
                    }
                    listenptr = listenptr->next;
                }
                interval = 0;
            }
        }
    }
    */

	return 0;
}


int CBaseServer::OnAfterAccept(SOCKET_OBJ* sock)
{
  	return 	SendLn(sock,"HELLO");
}

int CBaseServer::OnConnect(sockaddr* RemoteIp)
{
    printf("RemoteIP: %s \n",inet_ntoa(((struct sockaddr_in *) RemoteIp)->sin_addr));

	return NO_ERROR;
	//return SOCKET_ERROR;
}

int CBaseServer::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf , DWORD BytesRead )
{
    if (BytesRead < DEFAULT_BUFFER_SIZE)
	{
		//char tmp[50];
		//itoa(BytesRead,tmp,10);
		//strcat(tmp,"\r\n");
		//OutputDebugString(tmp);  
		buf->buf[BytesRead] = 0;

	}

		return NO_ERROR; 
}


int CBaseServer::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{


  return NO_ERROR;

}

void CBaseServer::OnDisconnect(int SockHandle)
{
	printf("Socket Disconnect : %d\n",SockHandle);
}

void CBaseServer::FreeSocketProc(SOCKET_OBJ* sock)
{
			    int sockhandle = sock->s;		
				WSAEVENT WaitEvent = sock->ConnectWaitEvent;
				OnBeforeDisconnect(sock);
				
				FreeSocketObj(sock);
				OnDisconnect(sockhandle);

				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
					WSACloseEvent(WaitEvent);
				}



}

void CBaseServer::HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error)
{
    SOCKET_OBJ *clientobj=NULL;     // New client object for accepted connections
    BUFFER_OBJ *recvobj=NULL,       // Used to post new receives on accepted connections
               *sendobj=NULL;       // Used to post new sends for data received
    BOOL        bCleanupSocket;
    //char       *tmp;
    int         i;

	//_CrtMemState s1, s2, s3;


    if (error != 0)
        printf("OP = %d; Error = %d\n", buf->operation, error);

    bCleanupSocket = FALSE;		
	EnterCriticalSection(&sock->SockCritSec);	

    if ((error != NO_ERROR) && (sock->Protocol == IPPROTO_TCP))
    {
        // An error occured on a TCP socket, free the associated per I/O buffer
        // and see if there are any more outstanding operations. If so we must
        // wait until they are complete as well.
        //
		if (buf->operation == OP_ACCEPT)
		{
			//ACCEPT 異常直接在放出
			//closesocket(sock->s);
			//sock->s = INVALID_SOCKET;

		

			shutdown(buf->sclient,SD_BOTH);

			//LINGER       lingerStruct;   
			//lingerStruct.l_onoff  = 1;   
			//lingerStruct.l_linger =  0;   
			//setsockopt(buf->sclient,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

			//closesocket(s);

			closesocket(buf->sclient);
			buf->sclient  = INVALID_SOCKET;

			InterlockedDecrement((long *)&sock->OutstandingOps);
			CCoreSocket::PostAccept(sock, buf); //do something then decres counter
			
		
		}
		else
		{
			//OutputDebugString("CBaseServer::HandleIo 958\n");
			FreeBufferObj(buf);


			if (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)
			{
				printf("Freeing socket obj in GetOverlappedResult\n");
			
				int sockhandle = sock->s;

				WSAEVENT WaitEvent = sock->ConnectWaitEvent;
				OnBeforeDisconnect(sock);

				LeaveCriticalSection(&sock->SockCritSec);
				FreeSocketObj(sock);
				OnDisconnect(sockhandle);

				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
					WSACloseEvent(WaitEvent);
				}

				
				return;
			}
		}


		LeaveCriticalSection(&sock->SockCritSec);
        return;
    }

    

	//維護 timer
	time(&sock->LastUseTime);

	/*if (sock->SocketType == ST_FOR_DISCONNECT)
	{
		closesocket(sock->s);
		sock->s = INVALID_SOCKET;

		sock->SocketType  = ST_FOR_NORMAL;

		LeaveCriticalSection(&sock->SockCritSec);
        return;
	
	}*/

	/*if (buf == NULL) 
	{
		OutputDebugString("NULL Buffer Object \r\n");
		LeaveCriticalSection(&sock->SockCritSec);
        return;
	
	} */

    if (buf->operation == OP_ACCEPT)
    {
        HANDLE            hrc;
        sockaddr *LocalSockaddr=NULL,
                         *RemoteSockaddr=NULL;
        int               LocalSockaddrLen,
                          RemoteSockaddrLen;

		

        // Update counters
        //InterlockedExchangeAdd(&gBytesRead, BytesTransfered);
        //InterlockedExchangeAdd(&gBytesReadLast, BytesTransfered);

        // Print the client's addresss
        /*
		sock->lpfnGetAcceptExSockaddrs(
                buf->buf,
                buf->buflen - ((sizeof(sockaddr) + 16) * 2),
                sizeof(sockaddr) + 16,
                sizeof(sockaddr) + 16,
                (SOCKADDR **)&LocalSockaddr,
               &LocalSockaddrLen,
                (SOCKADDR **)&RemoteSockaddr,
               &RemoteSockaddrLen
                );*/

		sock->lpfnGetAcceptExSockaddrs(
                buf->buf,
                0,
                sizeof(sockaddr) + 16,
                sizeof(sockaddr) + 16,
                (SOCKADDR **)&LocalSockaddr,
               &LocalSockaddrLen,
                (SOCKADDR **)&RemoteSockaddr,
               &RemoteSockaddrLen
                );

		//wsprintf(RemoteIp,"%s",inet_ntoa(((struct sockaddr_in *) RemoteSockaddr)->sin_addr));
		//No error 為要接受
		if (RemoteSockaddr == NULL) throw "NULL RemoteSockaddr";

		if (OnConnect(RemoteSockaddr) == NO_ERROR)
		{

			

			// Get a new SOCKET_OBJ for the client connection
			clientobj = GetSocketObj(buf->sclient, sock->af,sock->Protocol,ST_FOR_NORMAL);
			
			char *ipstr = inet_ntoa(((struct sockaddr_in *) RemoteSockaddr)->sin_addr);
			int iplen = strlen(ipstr);
			memcpy(clientobj->RemoteIp,ipstr,iplen);
			clientobj->RemoteIp[iplen] = 0;			

			
			// Associate the new connection to our completion port
			hrc = CreateIoCompletionPort(
                (HANDLE)buf->sclient,
                CompPort,
                (u_long) clientobj,
                0
                );
			if (hrc == NULL)
			{
				fprintf(stderr, "CompletionThread: CreateIoCompletionPort failed: %d\n",
                    GetLastError());

				LeaveCriticalSection(&sock->SockCritSec);
				return;
			}

		 

            // Get a BUFFER_OBJ to echo the data received with the accept back to the client

			//資料 > 0 再做 echo

			/************************
			因為 predata size = 0 所以在 OP_ACCEPT 應該不會拿到 data

			if (BytesTransfered > 0)
			{
				sendobj = GetBufferObj(clientobj, BytesTransfered);

				// Copy the buffer to the sending object
				memcpy(sendobj->buf, buf->buf, BytesTransfered);

				if (CCoreSocket::PostSend(clientobj, sendobj) != NO_ERROR)
				{
					FreeBufferObj(sendobj);
					error = SOCKET_ERROR;
				}

			}
			*/



			if (error == NO_ERROR)
			{
		   
			
				
				EnterCriticalSection(&clientobj->SockCritSec);
				
				//Send Hello
				//char Hello[7];
				//sprintf(Hello,"%s%c%c","HELLO",13,10);			
				//char *Hello = "HELLO";
				//char *ln = char(13);
				//strcat(Hello,ln);

			

				//sendobj = GetBufferObj(clientobj, strlen(Hello)+2);
				//memcpy(sendobj->buf, Hello, strlen(Hello));


				
				//if (CCoreSocket::PostSend(clientobj, sendobj) != NO_ERROR)
				if (OnAfterAccept(clientobj) == NO_ERROR)
				{
					//FreeBufferObj(sendobj);
					//error = SOCKET_ERROR;
			
					// Now post some receives on this new connection
					for(i=0; i < DEFAULT_OVERLAPPED_COUNT ;i++)
					{
						recvobj = GetBufferObj(DEFAULT_BUFFER_SIZE);

						if (CCoreSocket::PostRecv(clientobj, recvobj) != NO_ERROR)
						{
							// If for some reason the send call fails, clean up the connection
							//clientobj->bClosing = TRUE;

							//_CrtMemCheckpoint( &s1 );

							//OutputDebugString("CCoreSocket::PostRecv 1100\n");
							FreeBufferObj(recvobj);

							//_CrtMemCheckpoint( &s2 );

							//if ( _CrtMemDifference( &s3, &s1, &s2 ) )
							//		  _CrtMemDumpStatistics( &s3 );
	
							error = SOCKET_ERROR;		       

							break;
						}

					}
				}
			
				LeaveCriticalSection(&clientobj->SockCritSec);
			 
			}
        
        }
		else
		{

			
		   shutdown(buf->sclient,SD_BOTH);

		//	LINGER       lingerStruct;   
		//	lingerStruct.l_onoff  = 1;   
		//	lingerStruct.l_linger =  0;   
		//	setsockopt(buf->sclient,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

			
		   closesocket(buf->sclient);
		   buf->sclient = INVALID_SOCKET;
		}

		// Re-post the AcceptEx
		  CCoreSocket::PostAccept(sock, buf);
		 
		
	
		
    }
    else if ((buf->operation == OP_READ) && (error == NO_ERROR))
    {
        //
        // Receive completed successfully
        //
        //if ((BytesTransfered > 0) || (sock->Protocol == IPPROTO_UDP))
		if (BytesTransfered > 0)
        {
            //InterlockedExchangeAdd(&gBytesRead, BytesTransfered);
            //InterlockedExchangeAdd(&gBytesReadLast, BytesTransfered);

			/*
            // Create a buffer to send
            sendobj = GetBufferObj(sock, DEFAULT_BUFFER_SIZE);

            if (sock->Protocol == IPPROTO_UDP)
            {
                memcpy(&sendobj->addr, &buf->addr, buf->addrlen);
            }

            // Swap the buffers (i.e. buffer we just received becomes the send buffer)
            tmp              = sendobj->buf;
            sendobj->buflen  = BytesTransfered;
            sendobj->buf     = buf->buf;
            

          

		    if (CCoreSocket::PostSend(sock, sendobj) != NO_ERROR)
			{
				FreeBufferObj(sendobj);
            
	            error = SOCKET_ERROR;
		         
			}*/
			 if (OnDataRead(sock,buf,BytesTransfered) == NO_ERROR)
			 {
 
				// Post another receive
				if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
				{
                    // In the event the recv fails, clean up the connection
                    //OutputDebugString("CCoreSocket::PostRecv 1193\n");
					FreeBufferObj(buf);

                    error = SOCKET_ERROR;
				}
			 }
			 else
			 {
				 //OutputDebugString("CCoreSocket::PostRecv 1201\n");
				 FreeBufferObj(buf);
				 error = SOCKET_ERROR;
			 }

			/*

            InsertPendingSend(sock, sendobj);

            if (DoSends(sock) != NO_ERROR)
            {
                error = SOCKET_ERROR;
            }
            else
            {
                // Post another receive
                if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
                {
                    // In the event the recv fails, clean up the connection
                    FreeBufferObj(buf);
                    error = SOCKET_ERROR;
                }
            }*/
        }
        else
        {
            //printf("Got 0 byte receive\n");

            // Graceful close - the receive returned 0 bytes read
			//EnterCriticalSection(&clientobj->SockCritSec);
            //sock->bClosing = TRUE;

            // Free the receive buffer
			//OutputDebugString("CCoreSocket::PostRecv 1234\n");
            FreeBufferObj(buf);

			/*

            if (DoSends(sock) != NO_ERROR)
            {
                printf("0: cleaning up in zero byte handler\n");
                error = SOCKET_ERROR;
            }*/

            // If this was the last outstanding operation on socket, clean it up
            //if ((sock->OutstandingOps == 0) && (sock->OutOfOrderSends == NULL))
			//if (sock->OutstandingOps == 0)
           // {
               // printf("1: cleaning up in zero byte handler\n");
               // bCleanupSocket = TRUE;
            //}

		//	LeaveCriticalSection(&clientobj->SockCritSec);
        }
    }
    /*else if ((buf->operation == OP_READ) && (error != NO_ERROR) && (sock->Protocol == IPPROTO_UDP))
    {
        // If for UDP, a receive completes with an error, we ignore it and re-post the recv
        if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
        {
            error = SOCKET_ERROR;
        }
    }*/
    else if (buf->operation == OP_WRITE)
    {
        // Update the counters
        //InterlockedExchangeAdd(&gBytesSent, BytesTransfered);
        //InterlockedExchangeAdd(&gBytesSentLast, BytesTransfered);

		//OutputDebugString("OP_WRITE 1270\n");
		int rc = OnDataWrite(sock,buf,BytesTransfered);
		if (rc != IO_NO_FREEBUFFER || rc == SOCKET_ERROR)
		{
			FreeBufferObj(buf);
		}

		if (rc == SOCKET_ERROR)
			 error = SOCKET_ERROR;

		/*
        if (DoSends(sock) != NO_ERROR)
        {
            printf("Cleaning up inside OP_WRITE handler\n");
            error = SOCKET_ERROR;
        }*/
    }

   // if (error != NO_ERROR)
    //{
        //sock->bClosing = TRUE;
    //}

    //
    // Check to see if socket is closing
    //
    if ( (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)   )//&&
         //(sock->OutOfOrderSends == NULL) )
    {
        bCleanupSocket = TRUE;
    }
    //else
    //{
        
	    //OutputDebugString("In\n");
	    /*
		if (DoSends(sock) != NO_ERROR)
        {
            bCleanupSocket = TRUE;
        }*/
    //}

    

    if (bCleanupSocket)
    {
        int sockhandle = sock->s;

		shutdown(sock->s,SD_BOTH);

		//LINGER       lingerStruct;   
		//lingerStruct.l_onoff  = 1;   
		//lingerStruct.l_linger =  0;   
		//setsockopt(sock->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		//closesocket(s);

		closesocket(sock->s);
        sock->s = INVALID_SOCKET;
		WSAEVENT WaitEvent = sock->ConnectWaitEvent;

		OnBeforeDisconnect(sock);
        LeaveCriticalSection(&sock->SockCritSec);		
		FreeSocketObj(sock);
		OnDisconnect(sockhandle);

				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
					WSACloseEvent(WaitEvent);
				}
    }
	else
	{
		LeaveCriticalSection(&sock->SockCritSec);
	}

    return;
}

void CBaseServer::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}

//////////////////////////////////////////////////////////
//// CBaseClientXP

CBaseClientXP::CBaseClientXP(int Protocol ,char* RemoteIp ,  int RemotePort)
{
	CCoreSocket::Startup();

	m_Protocol = Protocol;
	strcpy(m_RemoteIp, RemoteIp);
	m_RemotePort = RemotePort;
}

CBaseClientXP::~CBaseClientXP()
{

}


HANDLE CBaseClientXP::Connect()
{
/*
	HANDLE hrc;
	
	SOCKET_OBJ      *sockobj=NULL;
                    //*ListenSockets=NULL;

	int rc;
	sockaddr_in sa;
	sockaddr_in saRemote;

	

	sa.sin_family = AF_INET;//寫死
	sa.sin_port = htons(8000); 
	sa.sin_addr.S_un.S_addr = INADDR_ANY;
 

	saRemote.sin_family = AF_INET;//寫死
	saRemote.sin_port = htons(m_RemotePort);
	saRemote.sin_addr.S_un.S_addr = inet_addr(m_RemoteIp);


	

	 // Create the completion port used by this server
    CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)NULL, 0);
    if (CompletionPort == NULL)
    {
        fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
        return NULL;
    }

	GetSystemInfo(&sysinfo);

    if (sysinfo.dwNumberOfProcessors > MAX_COMPLETION_THREAD_COUNT)
    {
        sysinfo.dwNumberOfProcessors = MAX_COMPLETION_THREAD_COUNT;
    }

      // Create the worker threads to service the completion notifications
    for(int i=0; i < (int) sysinfo.dwNumberOfProcessors  ;i++)
    {
        CompThreads[i] = (HANDLE) _beginthreadex(NULL, 0, CompletionThread, (LPVOID) this, 0, NULL);
        if (CompThreads[i] == NULL)
        {
            fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
            return NULL;
        }

		break ; // client 暫時只用一個 cpu

	}

		sockobj = GetSocketObj(INVALID_SOCKET, sa.sin_family,m_Protocol,ST_FOR_NORMAL);

		memcpy(&sockobj->addr,&saRemote,sizeof(saRemote));
		sockobj->addrlen = sizeof(saRemote);

			

        // create the socket
        sockobj->s = socket(sockobj->af, SOCK_STREAM, sockobj->Protocol);
        if (sockobj->s == INVALID_SOCKET)
        {
            fprintf(stderr,"socket failed: %d\n", WSAGetLastError());
            return NULL;
        }

        // Associate the socket and its SOCKET_OBJ to the completion port
        hrc = CreateIoCompletionPort((HANDLE)sockobj->s, CompletionPort, (u_long)sockobj, 0);
        if (hrc == NULL)
        {
            fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
            return NULL;
        }
		 // bind the socket to a local address and port
        rc = bind(sockobj->s, (SOCKADDR*) &sa, sizeof(sa));
        if (rc == SOCKET_ERROR)
        {
           fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
           return NULL;
        }

		if (sockobj->Protocol == IPPROTO_TCP)
        {
            BUFFER_OBJ *connobj=NULL;
            GUID        guidConnectEx = WSAID_CONNECTEX;                        
            DWORD       bytes;

            // Need to load the Winsock extension functions from each provider
            //    -- e.g. AF_INET and AF_INET6. 
            rc = WSAIoctl(
                    sockobj->s,
                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                   &guidConnectEx,
                    sizeof(guidConnectEx),
                   &sockobj->lpfnConnectEx,
                    sizeof(sockobj->lpfnConnectEx),
                   &bytes,
                    NULL,
                    NULL
                    );
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "WSAIoctl: SIO_GET_EXTENSION_FUNCTION_POINTER failed: %d\n",
                        WSAGetLastError());
                return NULL;
            }

			connobj = GetBufferObj(DEFAULT_BUFFER_SIZE); 

		

			//sa.sin_port = htons(m_RemotePort);
		

			 
			CCoreSocket::PostConnect(sockobj, connobj);
		}



	return CompThreads[0];*/

return NULL;

}

void CBaseClientXP::HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error)
{
    /*SOCKET_OBJ *clientobj=NULL;     // New client object for accepted connections
    BUFFER_OBJ *recvobj=NULL,       // Used to post new receives on accepted connections
               *sendobj=NULL;       // Used to post new sends for data received
    BOOL        bCleanupSocket;
    //char       *tmp;
    int         i;
	int         rc;

	//_CrtMemState s1, s2, s3;


    if (error != 0)
        printf("OP = %d; Error = %d\n", buf->operation, error);

    bCleanupSocket = FALSE;
	

    if ((error != NO_ERROR) && (sock->Protocol == IPPROTO_TCP))
    {
        // An error occured on a TCP socket, free the associated per I/O buffer
        // and see if there are any more outstanding operations. If so we must
        // wait until they are complete as well.
        //
	
	
			//OutputDebugString("CBaseServer::HandleIo 958\n");
			FreeBufferObj(buf);


			if (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)
			{
				printf("Freeing socket obj in GetOverlappedResult\n");
			
				int sockhandle = sock->s;
				WSAEVENT WaitEvent = sock->ConnectWaitEvent;

				OnBeforeDisconnect(sock);
				FreeSocketObj(sock);
				OnDisconnect(sockhandle);

				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
				//	WSACloseEvent(WaitEvent);
				}
			}
		
        return;
    }

    EnterCriticalSection(&sock->SockCritSec);

	//維護 timer
	time(&sock->LastUseTime);

	if ((buf->operation == OP_CONNECT) && (error == NO_ERROR))
	{
		int     optval=1;
		
		    rc = setsockopt(
                    sock->s,
                    SOL_SOCKET,
                    SO_UPDATE_CONNECT_CONTEXT,
                    (char *)&optval,
                    sizeof(optval)
                    );
            if (rc == SOCKET_ERROR)
            {
                fprintf(stderr, "setsockopt: SO_UPDATE_CONNECT_CONTEXT failed: %d\n",
                        WSAGetLastError());
            }

           // sock->bConnected = TRUE;

            // Post the specified number of receives on the succeeded connection
            for(i=0; i < DEFAULT_OVERLAPPED_COUNT ;i++)
            {
                recvobj = GetBufferObj(DEFAULT_BUFFER_SIZE);

                if (CCoreSocket::PostRecv(sock, recvobj) != NO_ERROR)
                {
                    FreeBufferObj(recvobj);
                    bCleanupSocket = TRUE;
                    break;
                }
            }

			SendLn(sock,"GET / \r\n");
			FreeBufferObj(buf);
	}
	else    
    if ((buf->operation == OP_READ) && (error == NO_ERROR))
    {
        //
        // Receive completed successfully
        //
        //if ((BytesTransfered > 0) || (sock->Protocol == IPPROTO_UDP))
		if (BytesTransfered > 0)
        {
            //InterlockedExchangeAdd(&gBytesRead, BytesTransfered);
            //InterlockedExchangeAdd(&gBytesReadLast, BytesTransfered);

		
			 if (OnDataRead(sock,buf,BytesTransfered) == NO_ERROR)
			 {
 
				// Post another receive
				if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
				{
                    // In the event the recv fails, clean up the connection
                    //OutputDebugString("CCoreSocket::PostRecv 1193\n");
					FreeBufferObj(buf);

                    error = SOCKET_ERROR;
				}
			 }
			 else
			 {
				 //OutputDebugString("CCoreSocket::PostRecv 1201\n");
				 FreeBufferObj(buf);
				 error = SOCKET_ERROR;
			 }

		
            
        }
        else
        {
            //printf("Got 0 byte receive\n");

            // Graceful close - the receive returned 0 bytes read
			//EnterCriticalSection(&clientobj->SockCritSec);
            //sock->bClosing = TRUE;

            // Free the receive buffer
			//OutputDebugString("CCoreSocket::PostRecv 1234\n");
            FreeBufferObj(buf);

		

            // If this was the last outstanding operation on socket, clean it up
            //if ((sock->OutstandingOps == 0) && (sock->OutOfOrderSends == NULL))
			//if (sock->OutstandingOps == 0)
            //{
             //   printf("1: cleaning up in zero byte handler\n");
             //   bCleanupSocket = TRUE;
           // }

		//	LeaveCriticalSection(&clientobj->SockCritSec);
        }
    }
   
    else if (buf->operation == OP_WRITE)
    {
        // Update the counters
        //InterlockedExchangeAdd(&gBytesSent, BytesTransfered);
        //InterlockedExchangeAdd(&gBytesSentLast, BytesTransfered);

		//OutputDebugString("OP_WRITE 1270\n");
        FreeBufferObj(buf);

	
    }

    if (error != NO_ERROR)
    {
        //sock->bClosing = TRUE;
    }

    //
    // Check to see if socket is closing
    //
    if ( (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)  )//&&
         //(sock->OutOfOrderSends == NULL) )
    {
        bCleanupSocket = TRUE;
    }
    //else
    //{
        
	    //OutputDebugString("In\n");
	   
    //}

    LeaveCriticalSection(&sock->SockCritSec);

    if (bCleanupSocket)
    {
        int sockhandle = sock->s;
		closesocket(sock->s);
        sock->s = INVALID_SOCKET;
		WSAEVENT WaitEvent = sock->ConnectWaitEvent;

		OnBeforeDisconnect(sock);
        FreeSocketObj(sock);
		OnDisconnect(sockhandle);

				if (WaitEvent != NULL)
				{
					WSASetEvent(WaitEvent);
					//WSACloseEvent(WaitEvent);
				}
    }

    return;*/
}

int CBaseClientXP::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf , DWORD BytesRead )
{
    if (BytesRead < DEFAULT_BUFFER_SIZE)
	{
		//char tmp[50];
		//itoa(BytesRead,tmp,10);
		//strcat(tmp,"\r\n");
		//OutputDebugString(tmp);  
		buf->buf[BytesRead] = 0;

		printf("%s",buf->buf);

	}

	

		return NO_ERROR; 
}


void CBaseClientXP::OnDisconnect(int SockHandle)
{

	printf("Socket Disconnect : %d\n",SockHandle);
}

void CBaseClientXP::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}


//////////////////////////////////////////////////////////
//// CBaseClient

CBaseClient::CBaseClient()
{
	//terminated = false;
	CompletionPort =  NULL;
	CompThreads[0] = NULL;
	SocketMgr = NULL;

	CCoreSocket::Startup();

	iniClient();

}

CBaseClient::~CBaseClient()
{

	
	if (WSASetEvent(SockMgrEvent) == false)
	{
		int rc = WSAGetLastError();
		printf("SetEvent Error : %d",rc);		
		throw;
	}
	
	printf("Wait SockMgr \r\n");
	WaitForSingleObject(SocketMgr,INFINITE);
	printf("Terminate SockMgr \r\n");
	

	SOCKET_OBJ *ptrs = m_SocketList;			

	while (ptrs)
	{							
		shutdown(ptrs->s,SD_BOTH);

	//	LINGER       lingerStruct;   
	//	lingerStruct.l_onoff  = 1;   
	//	lingerStruct.l_linger =  0;   
	//	setsockopt(ptrs->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		

		closesocket(ptrs->s);
		ptrs->s = INVALID_SOCKET;
		
		  
		ptrs = ptrs->next;
	}
	//terminated = true;

	

	PostQueuedCompletionStatus (CompletionPort,0,0,0);	
	WaitForSingleObject(CompThreads[0],INFINITE);
	CloseHandle(CompThreads[0]);
	

	//HANDLE  ThreadHandle[2];
	//ThreadHandle[0] = CompThreads[0];
	//ThreadHandle[1] = SocketMgr;	

	//if (ThreadHandle[0] != NULL && ThreadHandle[1] != NULL)
	//	WaitForMultipleObjects(2,ThreadHandle,TRUE,INFINITE);

	//CloseHandle(ThreadHandle[0]);
	//CloseHandle(ThreadHandle[1]);
	CloseHandle(CompletionPort);
	
	CCoreSocket::Cleanup();
}

int CBaseClient::iniClient()
{


	//CCoreSocket::Startup();

	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)NULL, 1);
    if (CompletionPort == NULL)
    {
        fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
		//throw "CreateIoCompletionPort failed";
        return -1;
    }
   
	unsigned threadID = 0;
	CompThreads[0] = (HANDLE) _beginthreadex(NULL, 0, CompletionThread, (LPVOID) this, 0, &threadID);
    if (CompThreads[0] == NULL)
    {
            fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
			throw "CreatThread failed";
            return -1;
    }

	SetThreadName(threadID,"BThread3");

	 

		// Create socket Msg Thread
	SocketMgr = (HANDLE) _beginthreadex(NULL, 0, SocketMgrThread, (LPVOID) this, 0, &threadID);

	SetThreadName(threadID,"BThread4");

 

	return 0;
}

void CBaseClient::HandleConnect(HANDLE hand , SOCKET_OBJ *sockobj , char* RemoteIp ,  int RemotePort, int timeout ,int *ReturnCode,void* UserData )
{
	
	*ReturnCode = NO_ERROR;

	HANDLE WaitHandle  = hand;
	
	SOCKET s = CCoreSocket::Connect(sockobj->Protocol,RemoteIp,RemotePort,timeout);

	if (s != SOCKET_ERROR)
	{
		sockaddr_in saRemote;

		saRemote.sin_family = AF_INET;//寫死
		saRemote.sin_port = htons(RemotePort);
		saRemote.sin_addr.S_un.S_addr = inet_addr(RemoteIp);


		sockobj->s = s;
		//SOCKET_OBJ  *sockobj = GetSocketObj(s, saRemote.sin_family,Protocol,ST_FOR_NORMAL);

		memcpy(&sockobj->addr,&saRemote,sizeof(saRemote));
		sockobj->addrlen = sizeof(saRemote);

		sockobj->ConnectWaitEvent = WaitHandle;

		if (UserData != NULL)
			sockobj->UserData = UserData;

		HANDLE hrc = CreateIoCompletionPort((HANDLE)sockobj->s, CompletionPort, (u_long)sockobj, 0);
        if (hrc == NULL)
        {
            throw; //throw first

			//FreeSocketObj(sockobj);
			//fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
			//*ReturnCode = SOCKET_ERROR;
			//SetEvent(sockobj->ConnectWaitEvent);
            //return ;
        }

		

		BUFFER_OBJ *connobj = GetBufferObj(DEFAULT_BUFFER_SIZE);

		/*
		if (CCoreSocket::PostRecv(sockobj, connobj) != NO_ERROR)
		{
					FreeBufferObj(connobj);
					//WSACloseEvent(sockobj->ConnectWaitEvent);
					//SetEvent(sockobj->ConnectWaitEvent);
					*ReturnCode = SOCKET_ERROR;
					return ;

		}

		OnConnected(sockobj);*/


		connobj->operation = OP_CONNECT;
		InterlockedIncrement((long *)&sockobj->OutstandingOps);
		PostQueuedCompletionStatus(CompletionPort,DEFAULT_BUFFER_SIZE,(u_long)sockobj,&connobj->ol);

		return ;

	}
	else
	{
		*ReturnCode = SOCKET_ERROR;
		//SetEvent(WaitHandle);
		return ;
	}
		
	
}

HANDLE CBaseClient::ThreadConnect(char* DnsServerIP , char* RemoteDomain , char* RemoteIp ,  int RemotePort,int timeout,int *ReturnCode , void* UserData)
{
	WSAEVENT waitevent = WSACreateEvent();

	HANDLE ProcessHeap = GetProcessHeap();
	ConnectThread_Data* ThreadData = (ConnectThread_Data *) CHeap::GetHeap(sizeof(ConnectThread_Data), &ProcessHeap);

	if (RemoteIp[0] == 0)
	{
		strcpy(ThreadData->DnsServerIP,DnsServerIP);
		strcpy(ThreadData->RemoteDomain,RemoteDomain);
	}
	else
	{
		strcpy(ThreadData->RemoteIp,RemoteIp);
	}

	ThreadData->ReturnCode = ReturnCode;
	ThreadData->waitevent = waitevent;
	ThreadData->RemotePort = RemotePort;
	ThreadData->parent = this;
	ThreadData->Timeout = timeout;



	ThreadData->sock = GetSocketObj(INVALID_SOCKET, AF_INET,IPPROTO_TCP,ST_FOR_NORMAL);

	
	ThreadData->sock->UserData = UserData;

	unsigned threadID = 0;

	HANDLE ch = (HANDLE) _beginthreadex(NULL, 0, ConntectThread, (LPVOID) ThreadData, 0, &threadID);
	if (ch == NULL)
	{
				fprintf(stderr, "CreatThread failed: %d\n", GetLastError());
				throw "CreatThread failed";
            
	}

	SetThreadName(threadID,"BThread5");

	 
	return waitevent;
	

}

HANDLE CBaseClient::Connect(int Protocol , char* RemoteIp ,  int RemotePort, int timeout ,int *ReturnCode,void* UserData )
{
	
	*ReturnCode = NO_ERROR;

	//HANDLE WaitHandle = CreateEvent(NULL,false,false,NULL);

	HANDLE WaitHandle  = WSACreateEvent();
	//_cprintf("WaitHandle : %x \n",WaitHandle);
	//if (WaitHandle == WSA_INVALID_EVENT)
	//{
	//	int rc = WSAGetLastError();
	//	_cprintf("WSA_INVALID_EVENT : %d \n",rc);
	//	throw;
	//}
	
	SOCKET s = CCoreSocket::Connect(Protocol,RemoteIp,RemotePort,timeout);

	if (s != SOCKET_ERROR)
	{
		sockaddr_in saRemote;

		saRemote.sin_family = AF_INET;//寫死
		saRemote.sin_port = htons(RemotePort);
		saRemote.sin_addr.S_un.S_addr = inet_addr(RemoteIp);


		SOCKET_OBJ  *sockobj = GetSocketObj(s, saRemote.sin_family,Protocol,ST_FOR_NORMAL);

		memcpy(&sockobj->addr,&saRemote,sizeof(saRemote));
		sockobj->addrlen = sizeof(saRemote);

		sockobj->ConnectWaitEvent = WaitHandle;

		if (UserData != NULL)
			sockobj->UserData = UserData;

		HANDLE hrc = CreateIoCompletionPort((HANDLE)sockobj->s, CompletionPort, (u_long)sockobj, 0);
        if (hrc == NULL)
        {
            throw; //throw first
			//FreeSocketObj(sockobj);
			//fprintf(stderr, "CreateIoCompletionPort failed: %d\n", GetLastError());
			//*ReturnCode = SOCKET_ERROR;
			//SetEvent(sockobj->ConnectWaitEvent);
            //return WaitHandle;
        }

		

		BUFFER_OBJ *connobj = GetBufferObj(DEFAULT_BUFFER_SIZE); 

		

		if (CCoreSocket::PostRecv(sockobj, connobj) != NO_ERROR)
		{
					FreeBufferObj(connobj);
					//WSACloseEvent(sockobj->ConnectWaitEvent);
					SetEvent(WaitHandle);
					*ReturnCode = SOCKET_ERROR;
					return WaitHandle;

		}
		else
		{

			OnConnected(sockobj);
		}

		return WaitHandle;

	}
	else
	{
		*ReturnCode = SOCKET_ERROR;
		SetEvent(WaitHandle);
		return WaitHandle;
	}
		
	
}

void CBaseClient::OnBeforeDisconnect(SOCKET_OBJ* sock)
{

}

void CBaseClient::HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error)
{
    SOCKET_OBJ *clientobj=NULL;     // New client object for accepted connections
    BUFFER_OBJ *recvobj=NULL,       // Used to post new receives on accepted connections
               *sendobj=NULL;       // Used to post new sends for data received
    BOOL        bCleanupSocket;
    //char       *tmp;
    //int         i;
	//int         rc;

	//_CrtMemState s1, s2, s3;

	
	EnterCriticalSection(&sock->SockCritSec);

    if (error != 0)
        _cprintf("OP = %d; Error = %d\n", buf->operation, error);

    bCleanupSocket = FALSE;
	

    //if ((error != NO_ERROR) && (sock->Protocol == IPPROTO_TCP))
	if ((error != NO_ERROR))
    {
        // An error occured on a TCP socket, free the associated per I/O buffer
        // and see if there are any more outstanding operations. If so we must
        // wait until they are complete as well.
        //
	
	
			//OutputDebugString("CBaseServer::HandleIo 958\n");
			FreeBufferObj(buf);


			if (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)
			{
				_cprintf("Freeing socket obj in GetOverlappedResult\n");
			
				int sockhandle = sock->s;

				//OnBeforeDisconnect(sock);
				WSAEVENT WaitEvent = sock->ConnectWaitEvent;
				OnBeforeDisconnect(sock);
				LeaveCriticalSection(&sock->SockCritSec);
				FreeSocketObj(sock);
				OnDisconnect(sockhandle);
				WSASetEvent(WaitEvent);
				//WSACloseEvent(WaitEvent);

			}
			else
			{
				LeaveCriticalSection(&sock->SockCritSec);
			}
		
        return;
    }




    

	//維護 timer
	time(&sock->LastUseTime);


    if ((buf->operation == OP_READ) && (error == NO_ERROR))
    {
        //
        // Receive completed successfully
        //
        //if ((BytesTransfered > 0) || (sock->Protocol == IPPROTO_UDP))
		if (BytesTransfered > 0)
        {
            //InterlockedExchangeAdd(&gBytesRead, BytesTransfered);
            //InterlockedExchangeAdd(&gBytesReadLast, BytesTransfered);

			/*
            // Create a buffer to send
            sendobj = GetBufferObj(sock, DEFAULT_BUFFER_SIZE);

            if (sock->Protocol == IPPROTO_UDP)
            {
                memcpy(&sendobj->addr, &buf->addr, buf->addrlen);
            }

            // Swap the buffers (i.e. buffer we just received becomes the send buffer)
            tmp              = sendobj->buf;
            sendobj->buflen  = BytesTransfered;
            sendobj->buf     = buf->buf;
            

          

		    if (CCoreSocket::PostSend(sock, sendobj) != NO_ERROR)
			{
				FreeBufferObj(sendobj);
            
	            error = SOCKET_ERROR;
		         
			}*/
			 if (OnDataRead(sock,buf,BytesTransfered) == NO_ERROR)
			 {
 
				// Post another receive
				if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
				{
                    // In the event the recv fails, clean up the connection
                    //OutputDebugString("CCoreSocket::PostRecv 1193\n");
					FreeBufferObj(buf);

                    error = SOCKET_ERROR;					
				}
			 }
			 else
			 {
				 //OutputDebugString("CCoreSocket::PostRecv 1201\n");
				 FreeBufferObj(buf);
				 error = SOCKET_ERROR;				 
			 }

			/*

            InsertPendingSend(sock, sendobj);

            if (DoSends(sock) != NO_ERROR)
            {
                error = SOCKET_ERROR;
            }
            else
            {
                // Post another receive
                if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
                {
                    // In the event the recv fails, clean up the connection
                    FreeBufferObj(buf);
                    error = SOCKET_ERROR;
                }
            }*/
        }
        else
        {
            //printf("Got 0 byte receive\n");

            // Graceful close - the receive returned 0 bytes read
			//EnterCriticalSection(&clientobj->SockCritSec);
            //sock->bClosing = TRUE;

            // Free the receive buffer
			//OutputDebugString("CCoreSocket::PostRecv 1234\n");
            FreeBufferObj(buf);

			/*

            if (DoSends(sock) != NO_ERROR)
            {
                printf("0: cleaning up in zero byte handler\n");
                error = SOCKET_ERROR;
            }*/

            // If this was the last outstanding operation on socket, clean it up
            //if ((sock->OutstandingOps == 0) && (sock->OutOfOrderSends == NULL))
			//if (sock->OutstandingOps == 0)
            //{
               // _cprintf("1: cleaning up in zero byte handler\n");
                //bCleanupSocket = TRUE;
            //}

		//	LeaveCriticalSection(&clientobj->SockCritSec);
        }
    }
    else if ((buf->operation == OP_READ) && (error != NO_ERROR) && (sock->Protocol == IPPROTO_UDP))
    {
        // If for UDP, a receive completes with an error, we ignore it and re-post the recv
        if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
        {
            error = SOCKET_ERROR;
        }
    }
    else if (buf->operation == OP_WRITE)
    {
        // Update the counters
        //InterlockedExchangeAdd(&gBytesSent, BytesTransfered);
        //InterlockedExchangeAdd(&gBytesSentLast, BytesTransfered);

		//OutputDebugString("OP_WRITE 1270\n");
        
		//if (OnDataWrite(sock,buf,BytesTransfered) != IO_NO_FREEBUFFER)
		//	FreeBufferObj(buf);

		int rc = OnDataWrite(sock,buf,BytesTransfered);
		if (rc != IO_NO_FREEBUFFER || rc == SOCKET_ERROR)
		{
			FreeBufferObj(buf);
		}

		if (rc == SOCKET_ERROR)
		{
			 error = SOCKET_ERROR;

			 _cprintf("sock->OutstandingOps : %d \n",sock->OutstandingOps);

		}

		/*
        if (DoSends(sock) != NO_ERROR)
        {
            printf("Cleaning up inside OP_WRITE handler\n");
            error = SOCKET_ERROR;
        }*/
    }
	else if (buf->operation == OP_CONNECT)
    {

		if (CCoreSocket::PostRecv(sock, buf) != NO_ERROR)
		{
					FreeBufferObj(buf);
					error = SOCKET_ERROR;
		}
		else
		{
			OnConnected(sock);
		}

		

	}

    //if (error != NO_ERROR)
    //{
        //sock->bClosing = TRUE;
    //}

    //
    // Check to see if socket is closing
    //
    //if ( (InterlockedDecrement((long *)&sock->OutstandingOps) == 0)  && (sock->bClosing) )//&&
         //(sock->OutOfOrderSends == NULL) )
    if ( (InterlockedDecrement((long *)&sock->OutstandingOps) == 0))
	{
        bCleanupSocket = TRUE;
    }
    //else
    //{
        
	    //OutputDebugString("In\n");
	    /*
		if (DoSends(sock) != NO_ERROR)
        {
            bCleanupSocket = TRUE;
        }*/
    //}

   
    if (bCleanupSocket)
    {
        
		WSAEVENT WaitEvent = sock->ConnectWaitEvent;
		int sockhandle = sock->s;

		shutdown(sock->s,SD_BOTH);

	//	LINGER       lingerStruct;   
	//	lingerStruct.l_onoff  = 1;   
	//	lingerStruct.l_linger =  0;   
	//	setsockopt(sock->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

		closesocket(sock->s);
        sock->s = INVALID_SOCKET;
		OnBeforeDisconnect(sock);
        LeaveCriticalSection(&sock->SockCritSec);
		FreeSocketObj(sock);
		OnDisconnect(sockhandle);
		WSASetEvent(WaitEvent);
		//WSACloseEvent(WaitEvent);
    }
	else
	{
		 LeaveCriticalSection(&sock->SockCritSec);
	}

    return;
}

int CBaseClient::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{


  return NO_ERROR;

}

int CBaseClient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf , DWORD BytesRead )
{
    if (BytesRead < DEFAULT_BUFFER_SIZE)
	{
		//char tmp[50];
		//itoa(BytesRead,tmp,10);
		//strcat(tmp,"\r\n");
		//OutputDebugString(tmp);  
		buf->buf[BytesRead] = 0;

		//printf("%s",buf->buf);

	}

	

		return NO_ERROR; 
}


void CBaseClient::OnDisconnect(int SockHandle)
{

	_cprintf("Socket Disconnect : %d\n",SockHandle);
 
}



int CBaseClient::OnConnected(SOCKET_OBJ* sock)
{

	//SendBufferObj(
    //SendLn(sock,"GET \\ \r\n");
	return NO_ERROR;
}

void CBaseClient::OnThreadConnectErr(SOCKET_OBJ* sock)
{
//	return NO_ERROR;
}

int CBaseClient::SendBuffer(SOCKET_OBJ* clientobj,char* buf,int len)
{
	int rc = 0;
	int sendlen = len;

	BUFFER_OBJ *sendobj = GetBufferObj(sendlen);
	memcpy(sendobj->buf, buf, sendlen);	

	//memcpy(&sendobj->addr,&saRemote,sizeof(saRemote));
	//sendobj->addrlen = sizeof(saRemote);


	if (rc = CCoreSocket::PostSend(clientobj,sendobj) != NO_ERROR)
	{
				 FreeBufferObj(sendobj);
				 //error = SOCKET_ERROR;
	}

    

	return rc;


}

//////////////////////////////////////////////////////
////////// CDNSclient ///////////////////////////////

CDnsClient::CDnsClient()
{
	//CBaseClient::iniClient();
	FID=FBitCode=FQDCount=0;
	//m_QueryResult[0] = 0;
	SocketTimeout = 30;

}
CDnsClient::~CDnsClient()
{

}

int CDnsClient::OnConnected(SOCKET_OBJ* sock)
{
	
	CBaseClient::OnConnected(sock);
	 
	DnsData *userdata = (DnsData *) sock->UserData;

	char SendQuery[255];

	//SetQr(0);
    //SetOpCode(0); //標準查詢
	//SetRd(true);
	FID = 0;
	FBitCode = 256;
	FQDCount = 1; //一次查一個 domain

	char DomainStr[255];

	if (userdata->QueryType == qtPTR)
	{
	    //反轉 + arpr

		int len = strlen(userdata->QueryString);
		char Zone1[4];
		char Zone2[4];
		char Zone3[4];
		char Zone4[4];

		memset(Zone1,0,4);
		memset(Zone2,0,4);
		memset(Zone3,0,4);
		memset(Zone4,0,4);

		int ZoneIdx = 0;
		int SavePos = 0;

		for(int i= 0 ; i < len ; i++)
		{
			if (userdata->QueryString[i] == '.')
			{
			
				if (ZoneIdx == 0)
				{					
					strncpy(Zone4,userdata->QueryString + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 1)
				{
					strncpy(Zone3,userdata->QueryString + SavePos, i - SavePos);
					//ZoneIdx ++;
				}
				else if (ZoneIdx == 2)
				{
					strncpy(Zone2,userdata->QueryString + SavePos, i - SavePos);
					strncpy(Zone1,userdata->QueryString + i + 1, len-i);
					 
					break;
				}
			 

				ZoneIdx ++;
				SavePos = i + 1;

			}
		}

		strcpy(DomainStr,Zone1);
		strcat(DomainStr,".");
		strcat(DomainStr,Zone2);
		strcat(DomainStr,".");
		strcat(DomainStr,Zone3);
		strcat(DomainStr,".");
		strcat(DomainStr,Zone4);
		strcat(DomainStr,".");	  
		strcat(DomainStr,"in-addr.arpa");
	}
	else
	{
		strcpy(DomainStr,userdata->QueryString);
	}

	
	ConvertDomainStr(DomainStr);

	char FillValue = '0';
	int FillLen = 0;
	

	FillValue = (FID & 0xFF00) >> 8;	 
	memcpy(SendQuery,&FillValue,1);
	FillLen++;

	FillValue = (FID & 0x00FF) ;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen++;

	FillValue = (FBitCode & 0xFF00) >> 8;	 
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen++;

	FillValue = (FBitCode & 0x00FF)  ;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen++;

	FillValue = (FQDCount & 0xFF00) >> 8;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen++;

	FillValue = (FQDCount & 0x00FF)  ;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen++;

	for (int i = 0 ; i < 6 ; i ++)
	{
		FillValue = 0;	
		memcpy(SendQuery+FillLen,&FillValue,1);
		FillLen++;
	}
	 	
	memcpy(SendQuery+FillLen,DomainStr,strlen(DomainStr));
	FillLen = FillLen + strlen(DomainStr);

	FillValue = 0;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen = FillLen + 1;

	FillValue = (userdata->QueryType & 0xFF00) >> 8;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen = FillLen + 1;

	FillValue = (userdata->QueryType & 0x00FF)  ;	
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen = FillLen + 1;

	FillValue = 0;
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen = FillLen + 1;

	FillValue = 1;
	memcpy(SendQuery+FillLen,&FillValue,1);
	FillLen = FillLen + 1; 


	 
	SendBuffer(sock,SendQuery,FillLen);
	
	
	return NO_ERROR;

	 
}

int CDnsClient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	
	CBaseClient::OnDataRead(sock,buf,BytesRead);
	
	DnsData *userdata = (DnsData *) sock->UserData;
	
	int APos = 12;
	char Result[1024]; 

	int NsType = 0;
	int Pref = 0;

	int LowestPref = 65535;
	char SaveDomain[255];

	
	int DataLen = BytesRead;
	ParseStr(buf->buf,&APos,DataLen,Result);
	APos = APos + 4;

	char RRDomain[255];
	char LastMxIp[255];

	LastMxIp[0] = 0;

	

	while (APos < DataLen)
	{
		ParseDNS(buf->buf,&APos,&NsType,&Pref,RRDomain,DataLen,Result);

		//printf("%d->%s %s\n",NsType,RRDomain,Result);

		if (userdata->QueryType == qtMX)
		{
		   if (NsType == qtMX)
		   {
			   if (Pref < LowestPref )
			   {
				   LowestPref = Pref;
				   strcpy(SaveDomain,Result);
			   }

			  //printf("%d\n",Pref);
		   } else if (NsType == qtA)
		   {
		   
			   if (strcmp(RRDomain,SaveDomain) == 0)
			   {
			     //printf("\nLowest: %d %s %s\n\n",LowestPref,SaveDomain,Result);
				 strcpy(userdata->QueryResult,Result);
			   }

			   strcpy(LastMxIp,Result);
		   
		   }
		
		}
		else if (userdata->QueryType == qtA)
		{
		
		 	if (NsType == qtA)
			{     

				//if (strcmp(RRDomain,m_Domain) == 0)
				//{
				  //printf("\n%s\n\n",Result);
				  strcpy(userdata->QueryResult,Result);
				  break;
				  //直接取第一筆
				//}
			}

		}
		else if (userdata->QueryType == qtPTR)
		{
		
		 	if (NsType == qtPTR)
			{     

			
				  strcpy(userdata->QueryResult,Result);
				  break;
				  
				
			}

		}
		
	
	}


	if (userdata->QueryResult[0] == 0 && userdata->QueryType ==  qtMX)
	{
		strcpy(userdata->QueryResult,LastMxIp);
	}
	//printf("%d\n",bobj->bytes);

	//this->terminated = true;
	//GetQueryResult
	shutdown(sock->s,SD_BOTH);

	//	LINGER       lingerStruct;   
	//	lingerStruct.l_onoff  = 1;   
	//	lingerStruct.l_linger =  0;   
	//	setsockopt(sock->s,SOL_SOCKET,SO_LINGER,(char*)&lingerStruct,sizeof(lingerStruct));   

	 

	closesocket(sock->s);
	sock->s = INVALID_SOCKET;

	return IO_NOTPOSTRECV;

}



HANDLE CDnsClient::Resolve(char* DnsServer , char *Domain , int QueryType  , char* QueryResult, int *ReturnCode)
{
//	 return NULL;
 


	HANDLE ProcessHeap = GetProcessHeap();
	DnsData *userdata =  (DnsData *) CHeap::GetHeap(sizeof(DnsData),&ProcessHeap);


	

	if (userdata != NULL)
	{
		strcpy(userdata->QueryString , Domain);
		userdata->QueryType = QueryType;
		userdata->QueryResult = QueryResult;
	}




	return Connect(IPPROTO_UDP,DnsServer,53,1500,ReturnCode,userdata);
	
	 
}

void CDnsClient::ParseStr(char *DomainStr , int* index ,int DataLen, char* ResultStr)
{

    int len = 0; 
	int SaveIdx = 0;

	char tmpstr[255];
	ResultStr[0] = 0;
 
	do 
	{
		tmpstr[0] = 0;
		len = *(DomainStr + *index);

		//有壓縮的資料
		while ((len & 0xC0) == 0xC0 ) 
		{
		   if (SaveIdx == 0) SaveIdx = (*index)+1;

		   char tmpchar = char(len & 0x3F); 

		   WORD newidx = 0;
		   newidx = (tmpchar << 8) & 0xFF00 | *(DomainStr + *index + 1) & 0x00FF;
		   //newidx ++;

		   *index = newidx;		
		   len = *(DomainStr + *index);
		}

		if (len > 0)
		{
		    strncpy(tmpstr,DomainStr + *index + 1,len);
			tmpstr[len] = 0;

			*index = *index + len + 1;
		}

		strcat(ResultStr,tmpstr);
		strcat(ResultStr,".");
	
	} 
	while (*(DomainStr + *index) != 0 && *index < DataLen);

	int ResLen = strlen(ResultStr);
	if (ResultStr[ResLen-1] == '.')
		ResultStr[ResLen-1] = 0;

	if (SaveIdx > 0) *index = SaveIdx; // restore original Idx +1
    *index = *index + 1; // set to first char of next item in  the resource


	

}


void CDnsClient::ParseDNS(char *DomainStr , int* index , int *NSType, int* Prefer, char* RRDomain, int DataLen, char* ResultStr)
{

	char RRName[255];
	char tmpstr[1024];
	RRDomain[0] = 0; 

	ParseStr(DomainStr,index,DataLen,RRName);

	WORD RR_Type =  (*(DomainStr + *index) << 8) & 0xFF00 | *(DomainStr + *index + 1) & 0x00FF;
	WORD RR_Class = (*(DomainStr + *index + 2) << 8) & 0xFF00 | *(DomainStr + *index + 3) & 0x00FF;
	int RR_TTL =  (*(DomainStr + *index + 4) << 24) & 0xFF000000 |
		          (*(DomainStr + *index + 5) << 16) & 0x00FF0000 |
				  (*(DomainStr + *index + 6) << 8) & 0x0000FF00 |
				  (*(DomainStr + *index + 7) ) & 0x000000FF;

	WORD RD_Length = (*(DomainStr + *index + 8) << 8) & 0xFF00 | *(DomainStr + *index + 9) & 0x00FF;

	*NSType = RR_Type;
 

	if (RR_Type == qtMX)
	{
	 
		int tmplen = *index + 9  + RD_Length + 1 ;
		memcpy(tmpstr,DomainStr,tmplen);

		WORD Preference = (*(DomainStr + *index + 10) << 8) & 0xFF00 | *(DomainStr + *index + 11) & 0x00FF;
 
		int i = *index + 12;
		ParseStr(tmpstr,&i,tmplen,ResultStr);

		*Prefer = Preference;	
	
	} else if (RR_Type == qtA)
	{
	   if (RD_Length > 0)
	   {
		   WORD ip1 = (*(DomainStr + *index + 10) & 0xFF00) << 8 | *(DomainStr + *index + 10) & 0x00FF;
		   WORD ip2 = (*(DomainStr + *index + 11) & 0xFF00) << 8 | *(DomainStr + *index + 11) & 0x00FF;
		   WORD ip3 = (*(DomainStr + *index + 12) & 0xFF00) << 8 | *(DomainStr + *index + 12) & 0x00FF;
		   WORD ip4 = (*(DomainStr + *index + 13) & 0xFF00) << 8 | *(DomainStr + *index + 13) & 0x00FF;
		   
		   sprintf(ResultStr,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
		                                   
	   }
            
        
        strcpy(RRDomain,RRName);

	} else if (RR_Type == qtNS || RR_Type == qtPTR)
	{
		int tmplen = *index + 9  + RD_Length + 1 ;
		memcpy(tmpstr,DomainStr,tmplen);

		int i = *index + 10;
		ParseStr(tmpstr,&i,tmplen,ResultStr);
	
	} else if (RR_Type == qtSOA)
	{
	    int tmplen = *index + 9  + RD_Length + 1 ;
		memcpy(tmpstr,DomainStr,tmplen);

		int i = *index + 10;

		char FMNAME[255];
		ParseStr(tmpstr,&i,tmplen,FMNAME);

		*index = i;
		char FRNAME[255];
		ParseStr(tmpstr,index,tmplen,FRNAME);

		int FSerial =  (*(tmpstr + *index) << 24) & 0xFF000000 |
					   (*(tmpstr + *index + 1) << 16) & 0x00FF0000 |
				       (*(tmpstr + *index + 2) << 8) & 0x0000FF00 |
				       (*(tmpstr + *index + 3) ) & 0x000000FF;


		*index = *index + 4;
		int FRefresh =  (*(tmpstr + *index) << 24) & 0xFF000000 |
					   (*(tmpstr + *index + 1) << 16) & 0x00FF0000 |
				       (*(tmpstr + *index + 2) << 8) & 0x0000FF00 |
				       (*(tmpstr + *index + 3) ) & 0x000000FF;

		*index = *index + 4;
		int FRetry =  (*(tmpstr + *index) << 24) & 0xFF000000 |
					   (*(tmpstr + *index + 1) << 16) & 0x00FF0000 |
				       (*(tmpstr + *index + 2) << 8) & 0x0000FF00 |
				       (*(tmpstr + *index + 3) ) & 0x000000FF;


		*index = *index + 4;
		int FExpire =  (*(tmpstr + *index) << 24) & 0xFF000000 |
					   (*(tmpstr + *index + 1) << 16) & 0x00FF0000 |
				       (*(tmpstr + *index + 2) << 8) & 0x0000FF00 |
				       (*(tmpstr + *index + 3) ) & 0x000000FF;

		*index = *index + 4;
		int FMinimumTTL =  (*(tmpstr + *index) << 24) & 0xFF000000 |
					   (*(tmpstr + *index + 1) << 16) & 0x00FF0000 |
				       (*(tmpstr + *index + 2) << 8) & 0x0000FF00 |
				       (*(tmpstr + *index + 3) ) & 0x000000FF;


	}

	*index = *index + RD_Length + 10;
}

void CDnsClient::ConvertDomainStr(char *Domain)
{

    char tmpchar[255] ;

	int len=strlen(Domain);
	int lencount=0;
	int reppos = 0;

	ZeroMemory(tmpchar,255);

     

	for(int i= 0 ; i < len ; i++)
	{
		if ( *(Domain+i) == '.')
		{
		     
			 //*(Domain+i) = lencount;
			 tmpchar[reppos] = lencount;
			 strncpy(tmpchar+reppos+1,Domain+reppos,lencount);

			 reppos = i+1;

			 //strnset((Domain+i),lencount,1);
			 lencount = 0;
		}
		else
		{
		    lencount ++;
		}
	}

	//補最後
	if (reppos > 0)
	{
	 tmpchar[reppos] = lencount;
	 strncpy(tmpchar+reppos+1,Domain+reppos,lencount);
	}

	//tmpchar[len+1] = '/0';



	strcpy(Domain,tmpchar);

	//return tmpchar;
 
}

