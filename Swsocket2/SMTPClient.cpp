// SMTPClient.cpp: implementation of the CSMTPClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SMTPClient.h"
#include <conio.h>
#include <process.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// ConntectThread
//////////////////////////////////////////////////////////////////////



CSMTPClient::CSMTPClient()
{
	SocketTimeout = 120;
}

CSMTPClient::~CSMTPClient()
{
	
}

/*

void CSMTPClient::ReadArgumentFile(SMTPMailData *userdata, char *filearg)
{
	HANDLE ProcessHeap = GetProcessHeap();

	FILE *farg = fopen(filearg, "r");

	char line[255]={0};
	char mailaddress[255]={0};

	if(farg==NULL) 
	{
		fprintf(stderr, "error to open file!\n");		
		throw;
		return;
	}	

	// get mail from

	if(fgets(userdata->MailFrom, sizeof(userdata->MailFrom), farg)!=NULL)
	{
	 
		
		if (userdata->MailFrom[strlen(userdata->MailFrom)-1]=='\n') 
			userdata->MailFrom[strlen(userdata->MailFrom)-1] = 0;
	//	ParseEmailAddress(line, mailaddress);
		//userdata->MailFrom = new char[strlen(line)+1];
		//userdata->MailFrom = (char *) CHeap::GetHeap(strlen(line)+1,&ProcessHeap);
		//strcpy(userdata->MailFrom, line);
	}
	else
	{
		throw;
	}

	// get rcpt to
	int count = 1;
	userdata->RcptToCount = 0;
	userdata->RcptToSeq = 0;
	userdata->RcptTo = (char *) CHeap::GetHeap(count,&ProcessHeap);
	strcpy(userdata->RcptTo, "");
	char *temp;

	while(fgets(line, sizeof(line), farg)!=NULL)
	{
		if (line[strlen(line)-1]=='\n') 
			line[strlen(line)-1] = 0;
		//ParseEmailAddress(line, mailaddress);
		strcpy(mailaddress,line);
		count = count + strlen(mailaddress) + 1;
		temp = userdata->RcptTo;
		userdata->RcptTo = (char *) CHeap::GetHeap(count,&ProcessHeap);
		strcpy(userdata->RcptTo, temp);		
		strcat(userdata->RcptTo, mailaddress);
		strcat(userdata->RcptTo, "\t");

		userdata->RcptToCount++;

		CHeap::FreeHeap(temp,&ProcessHeap);
		///delete temp;
	}
	fclose(farg);

}

  */


HANDLE CSMTPClient::Send(char *hostIP,
			        char *hostDomain,
					char *MailFrom,
					char *RcptTo,
					char *EmailFileName,
					unsigned int MailPos,
					unsigned int MailLen,
					char *SendResult,
					int *ReturnCode)
{


	HANDLE ProcessHeap = GetProcessHeap();
	SMTPMailData *userdata = (SMTPMailData *) CHeap::GetHeap(sizeof(SMTPMailData), &ProcessHeap);

	userdata->HostDomainName = hostDomain;
	userdata->MailFrom = MailFrom;
	userdata->RcptTo = RcptTo;
	userdata->Result = SendResult;
	userdata->ReturnCode = ReturnCode;
	//userdata->MailPos = MailPos;
	userdata->MailLen = MailLen;

	userdata->MailFp = fopen(EmailFileName,"rb");

	if (userdata->MailFp != NULL)
	{
		fseek(userdata->MailFp,MailPos,SEEK_SET);	
	}
	else
	{
	
		return NULL;
	}


	HANDLE hand = Connect(IPPROTO_TCP,hostIP,25,3000,ReturnCode,userdata);
	
	if (*ReturnCode == SOCKET_ERROR)
	{
			fclose(userdata->MailFp);
	
			//CHeap::FreeHeap(userdata->MailFrom,&ProcessHeap);
			//CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);
	}

	return hand;


}

/*
HANDLE CSMTPClient::Send(char *hostIP,char *hostDomain, char *fileml, char *filearg, char *SendResult,int *ReturnCode)
{

	HANDLE ProcessHeap = GetProcessHeap();
	SMTPMailData *userdata = (SMTPMailData *) CHeap::GetHeap(sizeof(SMTPMailData), &ProcessHeap);

	if (userdata != NULL)
	{
		ReadArgumentFile(userdata, filearg);
		userdata->Result = SendResult;
		userdata->ReturnCode = ReturnCode;
		userdata->feml = fopen(fileml, "rb");		
		strcpy(userdata->HostDomainName,hostDomain);
	}

	//printf("%s\n", userdata->MailFrom);
	//printf("%s\n", userdata->RcptTo);

	HANDLE hand = Connect(IPPROTO_TCP,hostIP,25,3000,ReturnCode,userdata);
	
	if (*ReturnCode == SOCKET_ERROR)
	{
			fclose(userdata->feml);
	
			//CHeap::FreeHeap(userdata->MailFrom,&ProcessHeap);
			CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);
	}

	return hand;

}
*/

/*

HANDLE CSMTPClient::ThreadDnsSend(char *DnsIP, char *hostDomain, char *fileeml, char *filearg, char *SendResult,int *ReturnCode)
{
	

	HANDLE rh = 0;
	HANDLE ProcessHeap = GetProcessHeap();	
	SMTPMailData *userdata = (SMTPMailData *) CHeap::GetHeap(sizeof(SMTPMailData), &ProcessHeap);

	if (userdata != NULL)
	{
		//ThreadData->sock->UserData = userdata;
		userdata->Result = SendResult;
		userdata->ReturnCode = ReturnCode;
		ReadArgumentFile(userdata, filearg);
		//userdata->Result = ThreadData->SendResult;
		userdata->feml = fopen(fileeml, "rb");		
		strcpy(userdata->HostDomainName,hostDomain);

		char Email[255];
		memset(Email,0,255);		
		
		GetRcptToBySeq(userdata, Email, 0);

		printf("To Email : %s\r\n",Email);

		//§ä¨ì domain
		char *Domain = NULL;
		Domain = strstr(Email,"@");
		if (Domain != NULL)
		{
			Domain++; //shift right	
			//strcpy(ThreadData->RemoteDomain,Domain);
			rh = ThreadConnect(DnsIP , Domain , "" ,  25,10 * 1000,ReturnCode , userdata);

		}
		else
		{
			throw "Domain failed";

		}
	}
	

	return rh;


}

HANDLE CSMTPClient::DnsSend(char *DnsIP,char *hostDomain, char *fileml, char *filearg, char *SendResult,int *ReturnCode)
{

	char hostIP[20];
	hostIP[0] =  0 ; 

	HANDLE ProcessHeap = GetProcessHeap();
	SMTPMailData *userdata = (SMTPMailData *) CHeap::GetHeap(sizeof(SMTPMailData), &ProcessHeap);

	if (userdata != NULL)
	{
		ReadArgumentFile(userdata, filearg);
		userdata->Result = SendResult;
		userdata->ReturnCode = ReturnCode;
		userdata->feml = fopen(fileml, "rb");		
		strcpy(userdata->HostDomainName,hostDomain);

		char Email[255];
		memset(Email,0,255);		
		
		GetRcptToBySeq(userdata, Email, 0);

		printf("To Email : %s\r\n",Email);

		//§ä¨ì domain
		char *Domain = NULL;
		Domain = strstr(Email,"@");
		if (Domain != NULL)
		{
			Domain++; //shift right		
			
			CDnsClient dns;	 
			int rc = NO_ERROR;

			HANDLE ch = dns.Resolve(DnsIP,(char *)Domain,qtMX,hostIP,&rc);	 
			WaitForSingleObject(ch,1000 * 5);

			CloseHandle(ch);
			
			printf("Dns for Resolve : %s  , IP: %s\r\n",Domain,hostIP);
		}
	}

	//printf("%s\n", userdata->MailFrom);
	//printf("%s\n", userdata->RcptTo);

	if (hostIP[0] != 0 )
	{
		HANDLE hand = Connect(IPPROTO_TCP,hostIP,25,3000,ReturnCode,userdata);

		if (*ReturnCode == SOCKET_ERROR)
		{
			fclose(userdata->feml);
			//delete userdata->MailFrom;
			//delete userdata->RcptTo;

			//CHeap::FreeHeap(userdata->MailFrom,&ProcessHeap);
			CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);
		}

		return hand;
	}
	else
	{
		fclose(userdata->feml);
		//delete userdata->MailFrom;
		//delete userdata->RcptTo;
		//CHeap::FreeHeap(userdata->MailFrom,&ProcessHeap);
		CHeap::FreeHeap(userdata->RcptTo,&ProcessHeap);

		CHeap::FreeHeap(userdata,&ProcessHeap);
		
		WSAEVENT waitevent = WSACreateEvent();
		SetEvent(waitevent);

		strcpy(SendResult,"DNS Lookup Error");
		*ReturnCode = SOCKET_ERROR;
		
		return waitevent;
	}

	
}
*/

int CSMTPClient::OnDataWrite(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesSent)
{
	CBaseClient::OnDataWrite(sock, buf, BytesSent);
	//printf("on data write\n");
	if(sock->LastCmd==SC_DATARET)
	{
		int readsize;
		char data[DEFAULT_BUFFER_SIZE];
		SMTPMailData *maildata = (SMTPMailData *)sock->UserData;

		unsigned int FpReadSize = 0;

		if (maildata->MailWrote < maildata->MailLen)
		{
				if (maildata->MailLen - maildata->MailWrote  > sizeof(data))
				{				
					FpReadSize = sizeof(data);
				}
				else
				{
					FpReadSize = maildata->MailLen - maildata->MailWrote;
				}
		}
		else
		{
				FpReadSize = 0;
			
		}

		if((0 != (readsize = fread(data, sizeof(char), FpReadSize, maildata->MailFp)) ) && FpReadSize > 0)
		{			
			
			maildata->MailWrote += readsize;
			if (readsize > buf->buflen) 
			{
				 _cprintf("......Buffer Couldn't Re-Send\n");
				SendBuffer(sock, data, readsize);		
			}
			else
			{
				memcpy(buf->buf,data,readsize);
				buf->buflen = readsize;

				CResourceMgr::SendBuffer(sock,buf);

				return IO_NO_FREEBUFFER;			
			
			}
			
		 
		}
		else
		{
			// send <CR><LF>.<CR><LF>
			char cmd[10];
			int cmdlen=0;

			strcpy(cmd, "\r\n.\r\n");
			sock->LastCmd = SC_DATADOT;

			cmdlen = strlen(cmd);

			if (cmdlen > buf->buflen) 
			{
				 _cprintf("......Buffer Couldn't Re-Send\n");
				SendBuffer(sock, cmd, cmdlen);
			}
			else
			{
				memcpy(buf->buf,cmd,cmdlen);
				buf->buflen = cmdlen;

				CResourceMgr::SendBuffer(sock,buf);

				//SendBuffer(sock, cmd, strlen(cmd));
				//_cprintf("send ->%s\n", cmd);
				return IO_NO_FREEBUFFER;
			
			}


			
		
			

		}

	} 
	else if(sock->LastCmd==SC_QUIT)
	{

		_cprintf("SC_QUIT OnDataWrite\n");
		return SOCKET_ERROR;
	}

	return NO_ERROR;
}

void CSMTPClient::OnThreadConnectErr(SOCKET_OBJ* sock)
{

	CBaseClient::OnBeforeDisconnect(sock);

	HANDLE ProcessHeap = GetProcessHeap();

	SMTPMailData *maildata = (SMTPMailData *)sock->UserData;

	fclose(maildata->MailFp);
	
	//CHeap::FreeHeap(maildata->RcptTo,&ProcessHeap);

}

void CSMTPClient::OnBeforeDisconnect(SOCKET_OBJ* sock)
{
	CBaseClient::OnBeforeDisconnect(sock);

	

	HANDLE ProcessHeap = GetProcessHeap();

	SMTPMailData *maildata = (SMTPMailData *)sock->UserData;

	if(sock->LastCmd==SC_QUIT)
		maildata->Result[0]=0;

	fclose(maildata->MailFp);
	//delete maildata->MailFrom;
	//delete maildata->RcptTo;

//	CHeap::FreeHeap(maildata->MailFrom,&ProcessHeap);
	//CHeap::FreeHeap(maildata->RcptTo,&ProcessHeap);

	
	
}

int CSMTPClient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{
	CBaseClient::OnDataRead(sock,buf,BytesRead);
	_cprintf("OnDataRead: %s\n", buf->buf);

	char code[255];
	char cmd[255];
//	char RcptTo[255];
	char data[DEFAULT_BUFFER_SIZE];
	size_t readsize;

	SMTPMailData *maildata = (SMTPMailData *)sock->UserData;

	//memset(code, 0, 255);	
	//memset(cmd, 0, 255);
	//memset(RcptTo, 0, 255);

	

	ParseResponse(code, buf->buf);

	switch(code[0])
	{
	case '2':
		if(sock->LastCmd==0)
		{
			strcpy(cmd, "HELO ");
			strcat(cmd,maildata->HostDomainName);
			strcat(cmd, "\r\n");

			sock->LastCmd = SC_HELO;
			SendBuffer(sock, cmd, strlen(cmd));
			_cprintf("send ->%s\n", cmd);
		}
		else if(sock->LastCmd==SC_HELO)
		{
			strcpy(cmd, "MAIL FROM: <");
			strcat(cmd, maildata->MailFrom);
			strcat(cmd, ">\r\n");
			sock->LastCmd = SC_MAILFROM;
			SendBuffer(sock, cmd, strlen(cmd));
			_cprintf("send ->%s\n", cmd);
		}
		else if(sock->LastCmd==SC_MAILFROM)
		{			
			strcpy(cmd, "RCPT TO: <");
			//strcat(cmd, GetRcptToBySeq(maildata, RcptTo, maildata->RcptToSeq));
			//maildata->RcptToSeq++;
			strcat(cmd,maildata->RcptTo);
			strcat(cmd, ">\r\n");
			sock->LastCmd = SC_RCPTTO;
			SendBuffer(sock, cmd, strlen(cmd));
			_cprintf("send ->%s\n", cmd);
		}
		else if(sock->LastCmd==SC_RCPTTO)
		{			
			//if(maildata->RcptToSeq<maildata->RcptToCount)
			//{
			/*	strcpy(cmd, "RCPT TO: <");			
				strcat(cmd, GetRcptToBySeq(maildata, RcptTo, maildata->RcptToSeq));			
				maildata->RcptToSeq++;
				strcat(cmd, ">\r\n");
				sock->LastCmd = SC_MAILFROM;
				SendBuffer(sock, cmd, strlen(cmd));
				_cprintf("send ->%s\n", cmd);*/
			//}
			//else // start send data
			//{
				strcpy(cmd, "DATA\r\n");
				sock->LastCmd = SC_DATA;
				SendBuffer(sock, cmd, strlen(cmd));
				_cprintf("send ->%s\n", cmd);
			//}
		}
		else if(sock->LastCmd==SC_DATADOT)
		{
			strcpy(cmd, "QUIT\r\n");
			sock->LastCmd = SC_QUIT;
			SendBuffer(sock, cmd, strlen(cmd));
			_cprintf("send ->%s\n", cmd);
			
			return IO_NOTPOSTRECV;
			
		}
		break;
	case '3':
		if(sock->LastCmd==SC_DATA)
		{
			sock->LastCmd = SC_DATARET;
			_cprintf("start to send data...\n");	

			unsigned int FpReadSize=0;

			if (maildata->MailLen <= sizeof(data))
			{
				FpReadSize = maildata->MailLen;
			}
			else
			{
				FpReadSize = sizeof(data);
			
			}


			 
			if(0 != (readsize = fread(data, sizeof(char), FpReadSize, maildata->MailFp)))
			{
				
				maildata->MailWrote += readsize;
				// send the first data
				SendBuffer(sock, data, readsize);
			}
			
			/*
			static int iaa = 0;
			while(0 != (readsize = fread(data, sizeof(char), sizeof(data), maildata->feml)))
			{
				iaa++;
				printf("%d send\n", iaa);
				SendBuffer(sock, data, readsize);
			}

			strcpy(cmd, "\r\n.\r\n");
			sock->LastCmd = SC_DATADOT;
			SendBuffer(sock, cmd, strlen(cmd));
			printf("send ->%s\n", cmd);
			*/
			
		}
		break;
	default:
		// some error occured
		//if (sizeof(*maildata->Result) < BytesRead)
		//{
		buf->buf[254] = 0;//need fix again	
		strcpy(maildata->Result, buf->buf);
		//}
		//else
		//{
			
		
		//}
	

		// free maildata
		//fclose(maildata->feml);
		//delete maildata->MailFrom;
		//delete maildata->RcptTo;

		*maildata->ReturnCode = SOCKET_ERROR;
		return SOCKET_ERROR;

		/*
		strcpy(cmd, "QUIT\r\n");
		sock->LastCmd = SC_QUIT;
		SendBuffer(sock, cmd, strlen(cmd));
		printf("send ->%s\n", cmd);	
		*/


		break;
	}



	return NO_ERROR;
}

char *CSMTPClient::ParseResponse(char *code, char *response)
{
	int i, len;
	len = strlen(response);
	for(i=0;i<len;i++)
	{
		if(response[i] >= '0' && response[i] <= '9')
		{
			code[i]=response[i];
		}
		else
		{
			code[i]=0;
			break;
		}
	}
	return code;
}

/*
char *CSMTPClient::GetRcptToBySeq(SMTPMailData *maildata, char *RcptTo, int Seq)
{
	strcpy(RcptTo, "");
	if(Seq >= maildata->RcptToCount)
		return RcptTo;

	int i;
	char *start, *end;		
	end = maildata->RcptTo;

	for(i=0;i<=Seq;i++)
	{
		start = end;
		end = strstr(end +1, "\t");
	}

	if(*start=='\t') start = start +1;

	strncpy(RcptTo, start, end - 1 - start + 1);
	return RcptTo;
	
}
*/

/*
char *CSMTPClient::ParseEmailAddress(char *FullAddress, char *EmailAddress)
{
	char *start, *end;
	start = strstr(FullAddress, "<");
	end = strstr(FullAddress, ">");
	memset(EmailAddress, 0, sizeof(EmailAddress));
	
	if(end==NULL || start==NULL) 
	{
		strcpy(EmailAddress, FullAddress);
		return EmailAddress;
	}

	if (end <= start) 
	{
		strcpy(EmailAddress, "");		
	}
	else
	{		
		int len = end - 1 - (start + 1) + 1;
		strncpy(EmailAddress, start + 1, len);		
		EmailAddress[len]=0;		
	}
	return EmailAddress;

}*/