// POPclient.cpp: implementation of the CPOPclient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "POPclient.h"
#include <stdlib.h>
#include "Checker2.h"
#include "../../SpamDog/Swparser/MailParser.h"
#include <conio.h>


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//// CPOPclient
//////////////////////////////////////////////////////////////////////////////

CPOPclient::CPOPclient()
{

 

}

HANDLE CPOPclient::CheckMail(char* HostIp, char* LoginId,char* Password , char *Result ,int *ReturnCode)
{
	//m_LoginId = LoginId;
	//m_Password = Password;
	
	//return   CBaseClient::Connect(IPPROTO_TCP,HostIp,110,timeout,eventtimeout);

	HANDLE rh = 0;
	HANDLE ProcessHeap = GetProcessHeap();	
	POPClientData *userdata = (POPClientData *) CHeap::GetHeap(sizeof(POPClientData), &ProcessHeap);

	if (userdata != NULL)
	{
		userdata->Result = Result;
		strcpy(userdata->LoginId , LoginId);
		strcpy(userdata->Password , Password);
		userdata->ReturnCode = ReturnCode;

		rh = ThreadConnect("" , "" , HostIp ,  110,10 * 1000,ReturnCode , userdata);
	}

	return rh;
}

void CPOPclient::OnBeforeDisconnect(SOCKET_OBJ* sock)
{
	CBaseClient::OnBeforeDisconnect(sock);
	POPClientData *userdata = (POPClientData *)sock->UserData; 

	if (userdata->fp != NULL)
	{
		fclose(userdata->fp);
		userdata->fp = NULL;
	}
 
	
}

int  CPOPclient::OnDataRead(SOCKET_OBJ* sock ,BUFFER_OBJ* buf, DWORD BytesRead )
{

    CBaseClient::OnDataRead(sock,buf,BytesRead);

	printf("OnDataRead: %s\n", buf->buf);

	POPClientData *userdata = (POPClientData *)sock->UserData; 

	char RtnCode[3];	
	char ParmStr[255];
	char SendCmd[1024];

	int pos1 = 0;
	char* findpos = NULL;

	SendCmd[0] = 0;

	if ((userdata->lastcmd != P_RETR) && (userdata->lastcmd != P_FIRSTRETR))
	{

		strncpy(RtnCode,buf->buf,3);
		RtnCode[3] = 0;
	 
		if (BytesRead - 4 <= sizeof(ParmStr) && BytesRead - 4 > 0)
		{
			strncpy(ParmStr,buf->buf+4, BytesRead - 4);
			ParmStr[BytesRead - 4] = 0;
		}
		else
		{		
			OutputDebugString("size error\n");
			//printf("%s \n",bobj->buf);

			throw "size Error";
		}


		if (strcmp(RtnCode,"+OK") == 0)
		{
			switch (userdata->lastcmd)
			{
				case NONE :
					userdata->lastcmd = P_USER;		
					sprintf(SendCmd,"USER %s\r\n",userdata->LoginId);		
					//SendLn(bobj->Socket,SendCmd);	
					SendBuffer(sock, SendCmd, strlen(SendCmd));
					printf("send ->%s\n", SendCmd);
					break;
				
				case P_USER :
					userdata->lastcmd = P_PASS;	
					sprintf(SendCmd,"PASS %s\r\n",userdata->Password);		
					SendBuffer(sock, SendCmd, strlen(SendCmd));	
					printf("send ->%s\n", SendCmd);
					break;

				case P_PASS:
					userdata->lastcmd = P_STAT;		
					sprintf(SendCmd,"STAT \r\n");		
					SendBuffer(sock, SendCmd, strlen(SendCmd));
					printf("send ->%s\n", SendCmd);
					break;  
				
				case P_STAT:

					//取得信件數
					findpos = strstr(ParmStr," ");
					pos1 = (int) (findpos - ParmStr); 
					//pos1 = pos(ParmStr," ",0);
					if (pos1 >= 0)
					{					
						ParmStr[pos1] = 0;
						userdata->TotalMail = atoi(ParmStr);
					}

					if (userdata->index < userdata->TotalMail && userdata->TotalMail > 0)
					{

						userdata->index ++;
						userdata->lastcmd = P_LIST;
						sprintf(SendCmd,"LIST %d \r\n",userdata->index);		
						SendBuffer(sock, SendCmd, strlen(SendCmd));		
						printf("send ->%s\n", SendCmd);
					}
					else
					{
						userdata->lastcmd = P_QUIT;	
						sprintf(SendCmd,"QUIT \r\n");		
						SendBuffer(sock, SendCmd, strlen(SendCmd));
						printf("send ->%s\n", SendCmd);
					
					}

				
					break;  

				case P_LIST:

					if (userdata->index <= userdata->TotalMail && userdata->TotalMail > 0)
					{
						 
						userdata->lastcmd = P_UIDL;	
						sprintf(SendCmd,"UIDL %d\r\n",userdata->index);		
						SendBuffer(sock, SendCmd, strlen(SendCmd));				
						printf("send ->%s\n", SendCmd);
				

					}
					else
					{
						userdata->lastcmd = P_QUIT;	
						sprintf(SendCmd,"QUIT \r\n");		
						SendBuffer(sock, SendCmd, strlen(SendCmd));
						printf("send ->%s\n", SendCmd);
					
					}

					break;

				case P_UIDL:

					if (userdata->index <= userdata->TotalMail && userdata->TotalMail > 0)
					{
						
						//取得UIDL			
						findpos = strstr(ParmStr," ");
						pos1 = (int) (findpos - ParmStr);

						 
						if (pos1 >= 0 && strlen(ParmStr)-2-pos1-1 > 0)
						{					
							strncpy(userdata->UIDL,ParmStr+pos1+1,strlen(ParmStr)-2-pos1-1);
							userdata->UIDL[strlen(ParmStr)-2-pos1-1] = 0;
						}
												
						userdata->lastcmd = P_FIRSTRETR;
						sprintf(SendCmd,"RETR %d \r\n",userdata->index);		
						SendBuffer(sock, SendCmd, strlen(SendCmd));
						printf("send ->%s\n", SendCmd);
						
						//system("cls");



						//special case for UIDL has /
						char tmp[255];

						//b64code
						CMailCodec cm;
						char *out = NULL;
						unsigned int size=0;
						
						
						out = cm.Base64Encode(userdata->UIDL,strlen(userdata->UIDL));

						strcpy(tmp,out);

						


						//findpos = strstr(userdata->UIDL,"/"); 
						//if (findpos == NULL)
						//{
							sprintf(tmp,"c:\\pop\\%s.eml",out);
							cm.FreeBase64Decode(out);
						//}
						//else
						//{
						//	char tmp2[255];
					//		strcpy(tmp2,userdata->UIDL);
					//		tmp2[(int) (findpos - userdata->UIDL)] = ' ';
					//		sprintf(tmp,"c:\\pop\\%s.eml",tmp2);

					//	}


						
						



						userdata->fp = fopen(tmp,"w+b");
						if (userdata->fp == NULL) 
							throw;

					}
					else
					{
						userdata->lastcmd = P_QUIT;	
						sprintf(SendCmd,"QUIT \r\n");		
						SendBuffer(sock, SendCmd, strlen(SendCmd));
						printf("send ->%s\n", SendCmd);
					
					}

					break;


				case P_RETR:
					break;
				
				case P_QUIT:
					break;

				case P_DELE:
					break;

				default:
			 		userdata->lastcmd ++;		
					sprintf(SendCmd,"QUIT \r\n");		
					SendBuffer(sock, SendCmd, strlen(SendCmd));	
					printf("send ->%s\n", SendCmd);
					break;  
			}
		
		}
		else
		{
				//指令異常			
			    *userdata->ReturnCode = SOCKET_ERROR;
				strcpy(userdata->Result,buf->buf);
				userdata->lastcmd = P_QUIT;	
				sprintf(SendCmd,"QUIT \r\n");		
				SendBuffer(sock, SendCmd, strlen(SendCmd));
				
				printf("send ->%s\n", SendCmd);

			
					
		}

	}
	else 
	{
	 
	 
		 int offset = 0;
		 if (userdata->lastcmd == P_FIRSTRETR)
		 {
		 

			 
			 	strncpy(RtnCode,buf->buf,3);
				RtnCode[3] = 0;
	 
				if (strcmp(RtnCode,"+OK") == 0)
				{

					findpos = strstr(buf->buf,"\r\n");
					pos1 = (int) (findpos - buf->buf);
					//pos1 = pos(bobj->buf,"\r\n",0);
					if (pos1 >= 0)
					{					
						offset = pos1 + 2;
					}
				}

				userdata->lastcmd = P_RETR;
		 
		 }
		
		 
		 if (//*(bobj->buf + bobj->bytes - 5) == char(13) &&
		     //*(bobj->buf + bobj->bytes - 4) == char(10) &&
		     *(buf->buf + BytesRead - 3) == '.' &&
		     *(buf->buf + BytesRead - 2) == char(13) &&
			 *(buf->buf + BytesRead - 1) == char(10))
		  { 

		       buf->buf[BytesRead-5] = 0;
			   fputs(buf->buf + offset,userdata->fp);
			   //fwrite(buf->buf + offset,1,BytesRead - offset ,  userdata->fp);

			   //fflush(userdata->fp);

			   ///scan spam
			   fseek(userdata->fp,0,SEEK_END);
			   int FileSize=ftell(userdata->fp);
			   fseek(userdata->fp,0,SEEK_SET);

			   char* mailContent = new char[FileSize+1];
			   char* ParseContent = new char[FileSize+1];
			   char From[255];
			   char Subject[255];

			   ParseContent[0] = 0;
				

			   fread(mailContent,sizeof(char),FileSize,userdata->fp);   
			   mailContent[FileSize] = 0;

			   CEmail mailp;
			   MailData *mdata = mailp.GetMailData(mailContent);

			   

			   MailHeader *mheader = mailp.GetMailHeader(mdata->mailHeader);

			   strcpy(From,mheader->From);
			   strcpy(Subject,mheader->Subject);

			   if (mheader->Content_Type->boundary[0] != 0)
			   {
				//代表有 boundary

				   MailBoundary* m_MailBoundary = 
				   mailp.GetBoundary(mdata->mailBody,mheader->Content_Type->boundary);
				   
				   MailBoundary* ptr =  m_MailBoundary;

					char tmpc[500];
					int BoundCount = 0;
					while (ptr)
					{
						if (ptr->IsRealData)
						{
							BoundCount ++ ;	
						
							strncpy(tmpc,ptr->BoundContent,499);

							if (stricmp(ptr->BoundHeader->Content_Transfer_Encoding,"base64") == 0)
							{
							
								CMailCodec cm;
								char *out = NULL;
								unsigned int size=0;
								
								cm.Base64Decode(ptr->BoundMail->mailBody,&out,&size);

								//printf("%s\n",out);
								strcat(ParseContent,out);

								cm.FreeBase64Decode(out);
							
							}
							else if (stricmp(ptr->BoundHeader->Content_Transfer_Encoding,"quoted-printable") == 0)
							{
							
									CMailCodec cm;
									char *out = cm.QPDecode(ptr->BoundMail->mailBody);

									//printf("%s\n",out);
									strcat(ParseContent,out);

									cm.FreeQPDecode(out);
							
							}
							else
							{
									
									strcat(ParseContent,ptr->BoundMail->mailBody);
							
							}

							//tmpc[499] = 0;
							//printf("%s\n",tmpc);					
							//printf("level : %d %s=>%s+++++++++++++++++++++++++++++++++++++++++\n\n\n",ptr->level,ptr->BoundHeader->Content_Type->text,ptr->BoundHeader->Content_Transfer_Encoding);


						}
 
						ptr = ptr->next;	
					}

					mailp.FreeBoundary(m_MailBoundary);


			   
			   }
			   else
			   {
			   
				   if (stricmp(mheader->Content_Transfer_Encoding,"base64") == 0)
				   {
					   			CMailCodec cm;
								char *out = NULL;
								unsigned int size=0;
								
								cm.Base64Decode(mdata->mailBody,&out,&size);

								//printf("%s\n",out);
								strcat(ParseContent,out);

								cm.FreeBase64Decode(out);
				   }
				   else if (stricmp(mheader->Content_Transfer_Encoding,"quoted-printable") == 0)
				   {
								CMailCodec cm;
								char *out = cm.QPDecode(mdata->mailBody);

								//printf("%s\n",out);
								strcat(ParseContent,out);

								cm.FreeQPDecode(out);
				   
				   }
				   else
				   {
					   strcat(ParseContent,mdata->mailBody);
					   
				   
				   }
					   
			   
			   }

			   

			   mailp.FreeMailHeader(mheader);
			   mailp.FreeMailData(mdata);
    
   

			   //fclose(fp);

			  
			  
			   delete mailContent;


			   //printf("%s",ParseContent);

			   CChecker2 cc(NULL);

			   double rate = cc.GetSpamRate(From,Subject,ParseContent,NULL);		   
			   //cc.UpdateSpamRate(0,1);

			   printf("Subject: %s , SpamRate: %f\n",Subject,rate); 

			   //Beep(1000,100);

			   delete ParseContent; 
			   
			   //--End Scan Spam
			   fclose(userdata->fp);
			   
			   //process spam mail
			   if (rate > 0.5)
			   {
					char tmp[255];

					CMailCodec cm;
					char *out = NULL;
					unsigned int size=0;
						
						
					out = cm.Base64Encode(userdata->UIDL,strlen(userdata->UIDL));

					//strcpy(tmp,out);

					char RenameToMail[MAX_PATH];
					sprintf(RenameToMail,"c:\\pop\\Spam-%s.eml",out);
					

					//sprintf(tmp,"%s.eml",userdata->UIDL);

				    
					//strcpy(RenameToMail,"c:\\pop\\Spam-");
					//strcpy(RenameToMail,tmp);
					

					DeleteFile(RenameToMail);
					sprintf(tmp,"c:\\pop\\%s.eml",out);
					MoveFile(tmp,RenameToMail);

					cm.FreeBase64Decode(out);
			   
			   }

			   userdata->fp = NULL;

			   //Delete Mail

			   //if (rate > 0.5)
			   
			   bool DeleteMail = false;

			   if (rate > 0.5 )
			   {				   
				   printf("\r\nDelete This Mail (y/n/x) ? ");
				   char input = getch();
				   if (input == 'y')
				   {
					   DeleteMail = true;
					   sprintf(SendCmd,"DELE %d \r\n",userdata->index);		
					   SendBuffer(sock, SendCmd, strlen(SendCmd));						
					   printf("send ->%s\n", SendCmd);
						
					   
					   userdata->index ++;
					   userdata->lastcmd = P_LIST;
				   }
				   else if (input == 'x')
				   {
				   
					   //誤判更正
				   
				   }				

			   }
			   
			   if (!DeleteMail)
			   {
				   if (userdata->index < userdata->TotalMail && userdata->TotalMail > 0)
				   {
											
							userdata->index ++;
							userdata->lastcmd = P_LIST;
							sprintf(SendCmd,"LIST %d \r\n",userdata->index);		
							SendBuffer(sock, SendCmd, strlen(SendCmd));						
							printf("send ->%s\n", SendCmd);
					}
					else
					{
							userdata->lastcmd = P_QUIT;	
							sprintf(SendCmd,"QUIT \r\n");		
							SendBuffer(sock, SendCmd, strlen(SendCmd));
							printf("send ->%s\n", SendCmd);
						
					}
			   }
				
		  }
		 else
		 {		 
			
			 fputs(buf->buf + offset,userdata->fp);		 
			    //fwrite(buf->buf + offset,1,BytesRead - offset ,  userdata->fp);
		 }

		  
	}
 
	 


	




	return NO_ERROR;
}

