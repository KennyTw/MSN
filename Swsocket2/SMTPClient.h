// SMTPClient.h: interface for the CSMTPClient class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SMTPCLIENT_H__AEEE8A99_BE03_4DFE_AF89_CA7CE8D10E28__INCLUDED_)
#define AFX_SMTPCLIENT_H__AEEE8A99_BE03_4DFE_AF89_CA7CE8D10E28__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CoreClass.h"

typedef struct _SMTPMailData
{
	char *MailFrom;
	char *RcptTo;
	//int  RcptToSeq;
	//int  RcptToCount;
	FILE *MailFp;	
	unsigned int MailLen;
	unsigned int MailWrote;
	char *Result;
	int *ReturnCode;
	char *HostDomainName;

} SMTPMailData;

/*
typedef struct _ConnectThreadData
{
	char DnsIP[16];
	char HostDomainName[255];
	char fileml[MAX_PATH];
	char filearg[MAX_PATH];
	char *SendResult;
	int *ReturnCode;
	WSAEVENT waitevent;
	void *parent;
	SOCKET_OBJ *sock;
} ConnectThreadData;

*/


class DLLDIR CSMTPClient : public CBaseClient
{
#define SC_HELO		1
#define SC_MAILFROM 2
#define SC_RCPTTO	3
#define SC_DATA		4
#define SC_DATARET	5
#define SC_DATADOT	6
#define SC_QUIT		7


private:
	//char * ParseEmailAddress(char *FullAddress, char *EmailAddress);
 

	
public:	

		CSMTPClient();
		~CSMTPClient();

	
		//void ReadArgumentFile(SMTPMailData *userdata, char *filearg);
		//char *GetRcptToBySeq(SMTPMailData *maildata, char *RcptTo, int Seq);

		 
	
		//HANDLE Send(char *hostIP,char *hostDomain, char *fileeml, char *filearg, char *SendResult,int *ReturnCode);
		//HANDLE DnsSend(char *DnsIP, char *hostDomain, char *fileeml, char *filearg, char *SendResult,int *ReturnCode);

		HANDLE Send(char *hostIP,
			        char *hostDomain,
					char *MailFrom,
					char *RcptTo,
					char *EmailFileName,
					unsigned int MailPos,
					unsigned int MailLen,
					char *SendResult,
					int *ReturnCode);

		//HANDLE ThreadDnsSend(char *DnsIP, char *hostDomain, char *fileeml, char *filearg, char *SendResult,int *ReturnCode);
		

protected:	
	virtual int OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead );
	virtual int OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent);
	virtual void OnBeforeDisconnect(SOCKET_OBJ* sock);
	virtual void OnThreadConnectErr(SOCKET_OBJ* sock);

	char *ParseResponse(char *code, char *response);
	


};

#endif // !defined(AFX_SMTPCLIENT_H__AEEE8A99_BE03_4DFE_AF89_CA7CE8D10E28__INCLUDED_)
