/////////////////////////////////////////////////////////////////////////////
// CoreClass

#if !defined CORECLASS__INCLUDED
#define CORECLASS__INCLUDED
//#undef _WIN32_WINNT

//#define _INC_WINDOWS
#include <winsock2.h>
#include <mswsock.h>
#include <stdio.h>
#include <time.h>

#ifdef DLLDIR_EX
   #define DLLDIR  __declspec(dllexport)   // export DLL information
#else   
   #define DLLDIR  __declspec(dllimport)   // import DLL information    
#endif 

 

#define DEFAULT_BUFFER_SIZE         4096   // default buffer size
#define DEFAULT_OVERLAPPED_COUNT    5      // Number of overlapped recv per socket
#define DEFAULT_ACP_OVERLAPPED_COUNT    15      // Number of overlapped recv per socket
#define MAX_COMPLETION_THREAD_COUNT 32     // Maximum number of completion threads allowed
#define DEFAULT_COMPLETION_CPU		4

typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // must be 0x1000
   LPCSTR szName; // pointer to name (in user addr space)
   DWORD dwThreadID; // thread ID (-1=caller thread)
   DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;


typedef struct _BUFFER_OBJ
{
    WSAOVERLAPPED        ol;

    SOCKET               sclient;       // Used for AcceptEx client socket

    char                *buf;           // Buffer for recv/send/AcceptEx
    int                  buflen;        // Length of the buffer

    int                  operation;     // Type of operation issued

#define OP_ACCEPT       0                   // AcceptEx
#define OP_READ         1                   // WSARecv/WSARecvFrom
#define OP_WRITE        2                   // WSASend/WSASendTo
#define OP_CONNECT		3



   

	//HANDLE 		TimeoutEvent;

    //ULONG                IoOrder;       // Order in which this I/O was posted

   // struct _BUFFER_OBJ  *next;

} BUFFER_OBJ;

typedef struct _SOCKET_OBJ
{
    SOCKET               s;              // Socket handle


    int                  af,             // Address family of socket (AF_INET, AF_INET6)
                         bClosing;       // Is the socket closing?
    //volatile 
    volatile  long     OutstandingOps; // Number of outstanding overlapped ops on 
                                         //    socket

    BUFFER_OBJ         **PendingAccepts; // Pending AcceptEx buffers 
                                         //   (used for listening sockets only)

    //ULONG                LastSendIssued, // Last sequence number sent
    //                     IoCountIssued;  // Next sequence number assigned to receives
    //BUFFER_OBJ          *OutOfOrderSends;// List of send buffers that completed out of order

	int Protocol;
    // Pointers to Microsoft specific extensions. These are used by listening
    //   sockets only
    LPFN_ACCEPTEX        lpfnAcceptEx; //server use
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs; //server use
	//LPFN_CONNECTEX       lpfnConnectEx; //client use

    CRITICAL_SECTION     SockCritSec;    // Protect access to this structure
	int SocketType;
	time_t LastUseTime;

	sockaddr     addr;
    int                  addrlen;

	//Data
	unsigned char LastCmd;
	void *UserData;	
	//FILE *fp;
//	char TempFileName[MAX_PATH];

	char RemoteIp[16];

	WSAEVENT ConnectWaitEvent;

    struct _SOCKET_OBJ  *prev,*next; 
} SOCKET_OBJ;

typedef struct _ConnectThread_Data
{
	char DnsServerIP[16];
	char RemoteDomain[255];
	char RemoteIp[16];
	int RemotePort;
	//int Protocol; udp 不需要
	//char *SendResult;
	int *ReturnCode;
	WSAEVENT waitevent;
	int Timeout;
	void *parent;
	SOCKET_OBJ *sock;
} ConnectThread_Data;

class DLLDIR CBaseClass
{
  public:
	CBaseClass();	// standard constructor
	virtual ~CBaseClass();

	
};

class DLLDIR CResourceMgr :  public CBaseClass
{
	
	#define ST_FOR_ACCEPT       0                   //Socket Type
	#define ST_FOR_NORMAL       1  
	#define ST_FOR_DELAY		2
//	#define ST_FOR_DISCONNECT	3
	 
	
	
	
private:
	//int SockObjCounter,BufferObjCounter;
protected:
	HANDLE ResourceHeap;
	//long SocketCount;
    

public:
   	CResourceMgr();
	virtual ~CResourceMgr();

	//DWORD  CompletionWaitTime;
//	volatile bool terminated;
//	volatile DWORD CompletionWaitTime;

	int SocketTimeout;
	SOCKET_OBJ *m_SocketList;
	HANDLE           CompletionPort,SockMgrEvent;

	CRITICAL_SECTION     SocketListSection;

	void AddSocketList(SOCKET_OBJ **head ,SOCKET_OBJ *obj);
	void DelSocketList(SOCKET_OBJ **head ,SOCKET_OBJ *obj);

	BUFFER_OBJ *GetBufferObj(int buflen);
	void FreeBufferObj(BUFFER_OBJ *obj);
	SOCKET_OBJ *GetSocketObj(SOCKET s, int af,int Protocol,int SocketType);
	void FreeSocketObj(SOCKET_OBJ *obj);
	void InsertPendingSend(SOCKET_OBJ *sock, BUFFER_OBJ *send);
	int DoSends(SOCKET_OBJ *sock);
//	void DoDisconnect(SOCKET_OBJ *sock);


	//void HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error);
    virtual void HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error);
	virtual void HandleSocketMgr(SOCKET_OBJ *sock);

	int SendLn(SOCKET_OBJ* clientobj,char* SendStr);
	int SendBufferObj(SOCKET_OBJ* clientobj,BUFFER_OBJ* sendbuf, int len);
	int SendBuffer(SOCKET_OBJ* socketobj, BUFFER_OBJ* bufferobj);
	int SendBufferData(SOCKET_OBJ* socketobj,char *DataBuffer , unsigned int len);

	

};

class DLLDIR CBaseServer : public CResourceMgr
{
private:
	
	int m_Protocol;
	int m_ListenPort;
	//HANDLE m_ServerWaitEvent;
	SYSTEM_INFO      sysinfo;

	char RemoteIp[16];
	int CPUNum;

	#define IO_ERROR   -1
	#define IO_OK  1
	#define IO_NOTPOSTRECV  2
	#define IO_NO_FREEBUFFER 3

public:
	CBaseServer(int Protocol , int ListenPort);
	~CBaseServer();

	//SOCKET m_MainListenSock;
	HANDLE           CompThreads[MAX_COMPLETION_THREAD_COUNT];
	HANDLE  SocketMgr;

	virtual void HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error);
	virtual int OnAfterAccept(SOCKET_OBJ* sock); //連線後可以送的第一個 command
	virtual int OnConnect(sockaddr* RemoteIp); //在 配至資源 之前 , 可拒絕 client
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	int StartServer();
	void FreeSocketProc(SOCKET_OBJ* sock);
	
};

/////////////////////////////////////////////////////////////////////////////
// CBaseClientXP

class CBaseClientXP : public CResourceMgr
{
private:
	int m_Protocol;
	int m_RemotePort;	
	char m_RemoteIp[16];
	SYSTEM_INFO      sysinfo;

	

public:
	CBaseClientXP(int Protocol ,char* RemoteIp ,  int RemotePort);
	~CBaseClientXP();
	HANDLE           CompThreads[MAX_COMPLETION_THREAD_COUNT];

	HANDLE Connect();

	virtual void HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error);
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

	
};

/////////////////////////////////////////////////////////////////////////////
// CBaseClient

class DLLDIR CBaseClient : public CResourceMgr
{
private:
	int m_Protocol;	
	//int m_RemotePort;	
	//char m_RemoteIp[16];
	//SYSTEM_INFO      sysinfo;
	int iniClient();

	#define IO_NO_FREEBUFFER 3

	

public:
	CBaseClient();
	~CBaseClient();

	

	HANDLE   CompThreads[MAX_COMPLETION_THREAD_COUNT];
	HANDLE   SocketMgr;

	
	HANDLE Connect(int Protocol , char* RemoteIp ,  int RemotePort,int timeout,int *ReturnCode , void* UserData = NULL);
	void HandleConnect(HANDLE hand , SOCKET_OBJ *sockobj , char* RemoteIp ,  int RemotePort,int timeout,int *ReturnCode , void* UserData = NULL);
	HANDLE ThreadConnect(char* DnsServerIP , char* RemoteDomain , char* RemoteIp ,  int RemotePort,int timeout,int *ReturnCode , void* UserData = NULL);

	virtual void HandleIo(SOCKET_OBJ *sock, BUFFER_OBJ *buf, HANDLE CompPort, DWORD BytesTransfered, DWORD error);
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	
	virtual int OnConnected(SOCKET_OBJ* sock);
	virtual void OnThreadConnectErr(SOCKET_OBJ* sock);

	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

	int SendBuffer(SOCKET_OBJ* clientobj,char* buf,int len);

	
};

typedef struct _DnsData
{

    char QueryString[255];
	int QueryType;
	char *QueryResult;


} DnsData;

class DLLDIR CDnsClient : public CBaseClient
{
private:
		//char* m_Domain;
		//int m_ResType;

		WORD FID,FBitCode,FQDCount;//,FANCount,FNSCount,FARCount;

		//char m_QueryResult[255];
public:
	#define qtMX 15
	#define qtNS  2
	#define qtA   1
	#define qtSOA 6
	#define qtPTR 12

		CDnsClient();
		~CDnsClient();

		HANDLE Resolve(char* DnsServer , char *Domain , int QueryType , char* QueryResult , int *ReturnCode);
		void  ConvertDomainStr(char *Domain);
		void  ParseStr(char *DomainStr , int* index , int DataLen, char* ResultStr);
		void  ParseDNS(char *DomainStr , int* index , int *NSType, int* Prefer, char* RRDomain, int DataLen, char* ResultStr);
		

protected:
	virtual int OnConnected(SOCKET_OBJ* sock);
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );


};


/////////////////////////////////////////////////////////////////////////////
// CCoreSocket
class DLLDIR CCoreSocket : public CBaseClass
{
  public:
    static HRESULT Startup();
	static void Cleanup();

	static int PostRecv(SOCKET_OBJ *sock, BUFFER_OBJ *recvobj);
	static int PostSend(SOCKET_OBJ *sock, BUFFER_OBJ *sendobj);
	static int PostAccept(SOCKET_OBJ *sock, BUFFER_OBJ *acceptobj);
	static int PostConnect(SOCKET_OBJ *sock, BUFFER_OBJ *connobj);


	static SOCKET Connect(int protocol , char* RemoteIp , int port , int timeout);
	static void IpReverse(char *IP);
	//static int PrintAddress(SOCKADDR *sa, int salen);
};

// CHeap Funcion 
//////////////////////////////////////////////////////////////////////
class DLLDIR CHeap  : public CBaseClass
{
public : 
    static void *GetHeap(int size, HANDLE* ResourceHeap);
	static int GetHeapSize(void *buf, HANDLE* ResourceHeap);
    static void FreeHeap(void *obj, HANDLE* ResourceHeap);
};


// Std Funcion 
//////////////////////////////////////////////////////////////////////
unsigned __stdcall CompletionThread(LPVOID lpParam);
unsigned __stdcall SocketMgrThread(LPVOID lpParam);
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);





//DWORD WINAPI CompletionThread(LPVOID lpParam);


#endif