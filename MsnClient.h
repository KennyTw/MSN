// MsnClient.h: interface for the CMsnClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSNCLIENT_H__1D0CEDFD_A7C5_4A83_99DF_E48D643F4758__INCLUDED_)
#define AFX_MSNCLIENT_H__1D0CEDFD_A7C5_4A83_99DF_E48D643F4758__INCLUDED_

#include "./Swsocket2/CoreClass.h"



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef struct _MSNData
{
	 unsigned int TxnId;
	 char ToMsgEmail[50];
	 char SaveMsg[4096];

} MSNData;

typedef struct _MSNParseData
{
    char* Data[20480];	//Data Pointer Array
	unsigned int DataCount;
} MSNParseData;

class CMsnSwBoard; //null class

typedef struct _SwBoardEventList
{
	CMsnSwBoard*  SwObj;
	HANDLE hand;
	struct _SwBoardEventList  *prev,*next; 
	
} SwBoardEventList;


class CMsnClient : public CBaseClient  
{

	#define MSN_NOTKEEP	0
	#define MSN_VER		1
	#define MSN_CVR		2
	#define MSN_USR		3
	#define MSN_USR2	4
	#define MSN_SYN	    5
	#define MSN_CHG		6
	#define MSN_ANS		7
	#define MSN_MSG		8
	#define MSN_QRY		9
	#define	MSN_XFR		10
	#define	MSN_CAL		11

	#define HoldEventCount 2


	#define MSN_NEED_RELOGIN	1

public:
	CMsnClient();
	virtual ~CMsnClient();
	HANDLE MSNLogin(char* IP , int Port);
	void SendMsg(char* ToEmail , char* Msg);

	char XFRIP[15] ;
	int XFRport;

	int LastStatus; 
	bool IsConnected;

	SwBoardEventList *m_SwBoardList;
	int EventListCount;

	HANDLE MsnHandle[MAXIMUM_WAIT_OBJECTS];
	void RemoveSwObjList(HANDLE hand);
	void RenumberList();


protected:
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	
	virtual int OnConnected(SOCKET_OBJ* sock);
	virtual void OnThreadConnectErr(SOCKET_OBJ* sock);

	virtual void OnDisconnect(int SockHandle);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);

	virtual void HandleSocketMgr(SOCKET_OBJ *sock);


private:
	CRITICAL_SECTION     SwBoardSec; 
	
	HANDLE MsnThread;
	HANDLE Connecthand;

	

	void ToParseData(char *AStr , char SplitChar, MSNParseData* aPData);
	void SplitAddrPort(char *AIP , char* IP , int* Port);

	bool GetLoginServerAddres(char* Addr , int Len);

	void InsertSwObjList(CMsnSwBoard** SwObj , HANDLE hand );
	void DestorySwObjList();
	
    

};


unsigned __stdcall MsnSwThread(LPVOID lpParam);

#endif // !defined(AFX_MSNCLIENT_H__1D0CEDFD_A7C5_4A83_99DF_E48D643F4758__INCLUDED_)
