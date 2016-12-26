// Checker2.cpp: implementation of the CChecker2 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Checker2.h"
#include "../../SpamDog/Swfiledb/DB.h"
#include "../../SpamDog/Swparser/MailParser.h"
#include <stdlib.h>



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CChecker2::CChecker2(CRITICAL_SECTION*  DBSection)
{

	mDBSection = DBSection;

	memset(&m_tlist,0,sizeof(m_tlist));
	memset(&m_tlist2,0,sizeof(m_tlist2));
	memset(&m_tlistRes,0,sizeof(m_tlistRes));

	BodySource = NULL;
	TextSource = NULL;
	//HeaderSource = NULL;
	//m_MailContent = NULL;

	Header_From = NULL;
	Header_Subject = NULL;
	
	
	Header_Receieved_Count = 0;

	IndexDbPath[0]=0;
	overflowDbPath[0]=0;
	datadbPath[0]=0;

	GetCurrentPath();
}

CChecker2::~CChecker2()
{

	ClearList(&m_tlist);
	ClearList(&m_tlist2);
	ClearList(&m_tlistRes);

	if (TextSource != NULL) delete [] TextSource;
 
}

bool CChecker2::bPassKey(char * KeyStr ,int startpos  )
{
 
	bool brtn = false;
 
	 
    //長度要符合
	if (startpos == 3 && _strnicmp(KeyStr - startpos, "gif",3) == 0 )
	{
		brtn = true;
	} else if (startpos == 4 && _strnicmp(KeyStr - startpos, "aspx",4) == 0)
	{
	    brtn = true;
	} else if (startpos == 5 && _strnicmp(KeyStr - startpos, "shtml",5) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos , "jpg",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos , "htm",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 4 && _strnicmp(KeyStr - startpos , "html",4) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "css",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "com",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "www",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 2 && _strnicmp(KeyStr - startpos, "w3",2) == 0)
	{
	    brtn = true;
	} else if (startpos == 4 && _strnicmp(KeyStr - startpos, "info",4) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "org",3) == 0)
	{ 
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "php",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "net",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "asp",3) == 0)
	{
	    brtn = true;
	} else if (startpos == 3 && _strnicmp(KeyStr - startpos, "idv",3) == 0)
	{
	    brtn = true;
	}else if (startpos == 2 && _strnicmp(KeyStr - startpos, "tw",2) == 0)
	{
	    brtn = true;
	} else if (startpos == 5 && _strnicmp(KeyStr - startpos, "image",5) == 0)
	{
	    brtn = true;
	}  else if (startpos == 6 && _strnicmp(KeyStr - startpos, "images",6) == 0)
	{
	    brtn = true;
	} else if (startpos == 7 && _strnicmp(KeyStr - startpos, "cgi-bin",7) == 0)
	{
	    brtn = true;
	}else if (startpos == 5 && _strnicmp(KeyStr - startpos, "hinet",5) == 0)
	{
	    brtn = true;
	}


	return brtn;

}

void CChecker2::ClearList(TokenList *list)
{

	Token *ptr = NULL;
	ptr = list->m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		delete ptr;
	 
		ptr = nextptr;		
	  
	}

	list->m_TokenList = NULL;
	list->size = 0;

}

void CChecker2::TrimHTML(char* InStr, char* OutBuffer)
{
	int BodyLen = strlen(InStr); 

	if (BodyLen <= 0 ) 
	{		
		return ;
	}

	int WritePos = 0;	 
	bool bIgnore = false;	 
	bool bIgnoretag = false;
	bool bContinue = false;
	
	for (int i = 0 ; i < BodyLen ; i++)
	{
		if (InStr[i] == '<')
		{
			bIgnore = true;
			if (strncmp(InStr+i+1,"!--",3) == 0)
			{
				bIgnoretag = true;
			}
		}
		else if (InStr[i] == '>')
		{
			if (bIgnoretag)
			{
				if (strncmp(InStr+i-2,"--",2) == 0)
				{
					bIgnoretag = false;
					bIgnore = false;
				}
			}
			else
			{
				bIgnore = false;
			}
		} 
		else if (!bIgnore)
		{
			 
				//中文
				OutBuffer[WritePos] = InStr[i];
				WritePos ++;
		}	
	}

	//防止 overflow
	if (BodyLen < WritePos)
	{
	  OutBuffer[WritePos] = 0;
	}
	else
	{
	  OutBuffer[BodyLen-1] = 0;
	}


}

void CChecker2::AddTokenList(TokenList* list,Token *obj)
{
    //search for dup
	Token *dupptr = NULL;
	dupptr = list->m_TokenList;
	while (dupptr)
	{	  
	    
	    Token *nextptr = dupptr->next;

		if (strcmp(dupptr->TokenData , obj->TokenData) == 0)
		{
			//Found Dup
			delete obj;
			return;
		}
	 
		dupptr = nextptr;		
	  
	}

	
	
	Token *end=NULL, 
               *ptr=NULL;

    // Find the end of the list
    ptr = list->m_TokenList;
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
        list->m_TokenList = obj;
    }
    else
    {
        // Put new object at the end 
        end->next = obj;
        obj->prev = end;
    }

	list->size++;
}

Token* CChecker2::FindTokenStr(TokenList *list,char *AStr)
{
	Token *ptr = NULL;

	ptr = list->m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		 if (strcmp(ptr->TokenData , AStr) == 0)
		 {
			return ptr;
		 }
	 
		ptr = nextptr;		
	  
	}

	return NULL;

}

bool CChecker2::AddTokenStr(TokenList *list,char *AStr)
{


		
		Token *atoken = new Token;
		memset(atoken,0,sizeof(Token));

		if (strlen(AStr) <= sizeof(atoken->TokenData))
		{
					  _strlwr(AStr);
					  strcpy(atoken->TokenData,AStr);
					  AddTokenList(list,atoken);

					  return true;
		
		}
		else
		{

			delete atoken;
			return false;
		}

}
void CChecker2::DelTokenStr(TokenList *list,char *AStr)
{
	Token *atoken = FindTokenStr(list,AStr);

	if (atoken != NULL) DelTokenList(list,atoken);

}

/*
void CChecker2::ParseHeader()
{
	unsigned int LineBegin = 0;
	char *tmpline=NULL;
	char *FoundPos=NULL;
	int HeaderPos = 0;
	
	//split header and body and get mail from , subject
	if (m_MailContent == NULL) return;
	int len = strlen(m_MailContent);

	for (int i=0 ; i < len ; i++)
	{
	
	
		if (m_MailContent[i] == char(13) 
			 && m_MailContent[i+1] == char(10)
			 && m_MailContent[i+2] != char(0x20)
			 && m_MailContent[i+2] != char(0x09))
		{


			if ( m_MailContent[i+2] == char(13) 
			 && m_MailContent[i+3] == char(10))
			{
				//header end;
				BodySource = m_MailContent+i+4;
				TextSource = new char[strlen(BodySource) +1];
				TrimHTML(BodySource,TextSource);
				break;
			}
			else
			{
				tmpline = new char[i+1-LineBegin];
				memcpy(tmpline,m_MailContent+LineBegin,i-LineBegin);
				tmpline[i-LineBegin] = 0;

				FoundPos = strstr(tmpline,":");
				if(FoundPos ==  NULL) continue;

				HeaderPos = (int) (FoundPos - tmpline);
				tmpline[HeaderPos] = 0;

				if (_stricmp(tmpline,"From") == 0)
				{
					tmpline[HeaderPos] = ':';
				//	delete m_MailHeader->From;
				//	m_MailHeader->From = new char[strlen(tmpline)-HeaderPos-1];
					//memcpy(m_MailHeader->From,tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1); 
					strncpy(Header_From,tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1);
					//printf("From:%s\n",header_From);
				} 
				
				else if (_stricmp(tmpline,"Subject") == 0)
				{
				    tmpline[HeaderPos] = ':';
					//delete m_MailHeader->Subject;
					//m_MailHeader->Subject = new char[strlen(tmpline)-HeaderPos-1];
					//memcpy(m_MailHeader->Subject,tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1); 
					//printf("Subject:%s\n",header_Subject);

					strncpy(Header_Subject,tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1);
				}
				
				else if (_stricmp(tmpline,"Received") == 0)
				{
					tmpline[HeaderPos] = ':';

				
					//m_MailHeader->Received = new char[strlen(tmpline)-HeaderPos-1];
					//memcpy(m_MailHeader->Received,tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1); 			

					//have to debug
					if (Header_Receieved_Count < sizeof(Header_Received) / sizeof(Header_Received[0]))
					{
						strncpy(Header_Received[Header_Receieved_Count],tmpline + HeaderPos+2 , strlen(tmpline)-HeaderPos-1);
						Header_Receieved_Count++;
					}
 
				}


				delete tmpline;
				LineBegin = i + 2;
					
			}


		}
	
	}

}*/

void CChecker2::DelTokenList(TokenList* list,Token *obj)
{

   // Make sure list isn't empty
    if (list->m_TokenList != NULL)
    {
        // Fix up the next and prev pointers
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;

        if (list->m_TokenList == obj)
            (list->m_TokenList) = obj->next;

		delete obj;
    }

}
int CChecker2::ParseNumKey(TokenList* list , char *Source )
{
 
	if (Source == NULL) return 0;

	int AddCount = 0;
	int  save2 = -1 ,save3 = 0;
	int i = 0;
	int StrLen = strlen(Source);
	char NumBuff[50];
	int NumCount = 0;
	memset(NumBuff,0,50);

	//此方法全型數字不試用 //限制數量 20 

	while( i <= StrLen && m_tlist.size <= 20)
	{

		if ((Source[i] & 0xFF) <= 128 )
		{
		if (save2 == -1)
		{
			
			//if (strncmp(Source + i , CutStr,CutStrLen) == 0)			
			if (isdigit(*(Source + i)) )
			{				
				save2 = i ;
				NumBuff[NumCount] = *(Source + i);
				NumCount++;
				//i += CutStrLen;				
				save3 = 0;		 				
			}
			
		}
		else
		{
		
			 
			if (isdigit(*(Source + i)) && NumCount < 20)
			{
			
				NumBuff[NumCount] = *(Source + i);
				NumCount++;
			
			} else
			//等待字尾 , 使用次數多可以改用 map table
			if (*(Source + i) == ' ' ||			
				*(Source + i) == 13 ||
				i   == StrLen ||
				NumCount >= 20
				)            
			{
			
				 
				if (NumCount >= 4 && NumCount <= 20)
				{
					
					//std::string mstr;
					//mstr.append(save2 + Source, tmpi - save2 );
				
					//mstr = lowercase(mstr);
					NumBuff[NumCount]  = 0;
					

					//KeyList->insert(KeyList->end(),NumBuff);
					//Token *atoken = new Token;
					//memset(atoken,0,sizeof(Token));

					//if (strlen(NumBuff) <= sizeof(atoken->TokenData))
					//{
					 // strcpy(atoken->TokenData,NumBuff);
					  //AddTokenList(list,atoken);

					if (list->size >= MaxKeyNum) break;

					if (AddTokenStr( list ,  NumBuff))
						AddCount++;
					//}
					//else
					//{

					//	delete atoken;
					//}
					
				}

				NumCount = 0;				 
				save2 = -1;
			}


		
		}
		}


	 i++;
	}


	return AddCount;
}

void CChecker2::GetCurrentPath()
{

	 char path_buffer[_MAX_PATH];
	 char drive[_MAX_DRIVE];
	 char dir[_MAX_DIR];
	 char fname[_MAX_FNAME];
	 char ext[_MAX_EXT];
	 
	 HINSTANCE hInstance = GetModuleHandle(NULL);
     GetModuleFileName(hInstance, path_buffer, MAX_PATH);

	 _splitpath( path_buffer, drive, dir, fname, ext );	

	   strcpy(IndexDbPath,drive);
	   strcat(IndexDbPath,dir);   
	   
	   strcpy(overflowDbPath,IndexDbPath);
	   strcpy(datadbPath,IndexDbPath);

	   strcat(IndexDbPath,"Srvdb1.db");
	   strcat(overflowDbPath,"Srvdb2.db");
	   strcat(datadbPath,"Srvdb3.db");

}

void CChecker2::GetSpamData(TokenList* list )
{

	CDB db(mDBSection,IndexDbPath,overflowDbPath,datadbPath,0,1024 * 10);

	Token *ptr = NULL;
	ptr = list->m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		CheckDataFile df;
		memset(&df,0,sizeof(df));

		SResult sres = db.SelectKey(ptr->TokenData);

		if (sres.FindPosInKey == -1 && sres.FindPosInOvr == -1)
		{
			//找不到資料 			
			ptr->SearchRes = -1;	  
		}
		else
		{   
			//讀出資料
			db.SelectData(sres.DataFilePos,(char *) &df , sizeof(df));  
			
			ptr->SearchRes = sres.DataFilePos;
			ptr->OkRate = df.OkRate;
			ptr->SpamRate = df.SpamRate;
			ptr->SpamCount = df.SpamCount;
			ptr->Status = df.Status;
			ptr->UsedCount = df.UsedCount;
		}      
	 
		ptr = nextptr;		
	  
	}

 


}

double CChecker2::GetSpamRate(char* From ,char* Subject, char *MailBody , char *SourceIp)
{
    
	//m_MailContent = MailContent;

	if (From == NULL || Subject == NULL ||  MailBody == NULL) return 0.5;

	BodySource = MailBody;
	//HeaderSource = MailHeader;
	Header_From = From;
	Header_Subject = Subject;

	TextSource = new char[strlen(BodySource) +1];

	TextSource[0] = 0;
    TrimHTML(BodySource,TextSource);

	//ParseHeader();

	//Source Ip  先不處理優先判讀 , 並直接加入 result list , 不做任何 gram
	if (SourceIp != NULL)
	{
		/*Token *atoken = new Token;
		memset(atoken,0,sizeof(Token));

		strcpy(atoken->TokenData,SourceIp);
		AddTokenList(&m_tlist,atoken);		*/
		
		AddTokenStr(&m_tlistRes,SourceIp);
	}

	if (ParseKey(&m_tlist,BodySource,"://") == 0)
	{
		//如果空白在解 www.
		if (ParseKey(&m_tlist,BodySource,"www.") == 0)
		{
			//如果還空白用純文字內容
			if(TextSource != NULL)
			{
				//先以 :// 解開
				if(ParseKey(&m_tlist,TextSource,"://") == 0)
				{
					//如果空白在解 www.
					if(ParseKey(&m_tlist,TextSource,"www.") == 0)
					{
						//拿出殺手賤
						ParseNumKey(&m_tlist,TextSource);
					}
				}
			}
		
		}
	}

	Token *ptr = NULL;
	ptr = m_tlist.m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		//----------------------------------
		int i = 0;
		bool bGetDot = true;
		int save2 = 0;		
		int Size = strlen(ptr->TokenData);
		int MinKeySize = 2;

		while( i <= Size )
		{
			
			if (ptr->TokenData[i] == '.' && bGetDot)
			{	 
				   
					//std::string mstr;
					//if (i - save2 >= 2)
					if (i - save2 >= MinKeySize  && !bPassKey(ptr->TokenData + i,i - save2))
					//if (i - save2 >= 2)
					{
					 
						 char tmpstr[255];
						 memset(tmpstr,0,sizeof(tmpstr));

						 strncpy(tmpstr ,ptr->TokenData + save2 , i - save2); 
						
						 
						 AddTokenStr(&m_tlistRes,tmpstr);
						 //mstr.append(save2 + tmpstdstr.c_str() , i - save2);					 
						//mstr = lowercase(mstr);
						//ResList.insert(ResList.end(),mstr);
						 

						//if (mstrlist.size() > 50)
						//	break;

						 
					}

				    save2 = i+1; 
  				    
			} 
			else if(ptr->TokenData[i] == '/' )
			{
				bGetDot = false;			 
				if (i - save2 > MinKeySize )
				{
					if (!bPassKey(ptr->TokenData + i,i - save2))
					{
				
						 char tmpstr[CheckDefaultKeyDataSize];
						 memset(tmpstr,0,sizeof(tmpstr));

						 strncpy(tmpstr ,ptr->TokenData + save2 , i - save2); 
						
						 AddTokenStr(&m_tlistRes,tmpstr); 

						//	if (mstrlist.size() > 50)
					    //		break;

						 
					}
				}

				/*if (saveDomain > -1)
				{
					  
					  DomainStr.append(saveDomain + tmpstdstr.c_str() , i - saveDomain);
					  
					  if (SimpleCheckDomain((char *)DomainStr.c_str()))
					  {
						DomainStr = lowercase(DomainStr);
						//OutputDebugString(DomainStr.c_str());
						//OutputDebugString("\n");
						DomainList.insert(DomainList.end(),DomainStr);
					  }
					  

					  saveDomain = -1;
				}*/

					

			
					save2 = i+1; 
				

			}
			else  if (i == Size ) 
			{
					//std::string mstr;
					if (i - save2 >= MinKeySize  && !bPassKey(ptr->TokenData + i,i - save2))
					//if (i - save2 >= 2)
					{
					 
						 
						
						 char tmpstr[CheckDefaultKeyDataSize];
						 memset(tmpstr,0,sizeof(tmpstr));

						 strncpy(tmpstr ,ptr->TokenData + save2 , i - save2); 
						
						 AddTokenStr(&m_tlistRes,tmpstr); 

						//mstr.append(save2 + tmpstdstr.c_str() , i - save2);					 
						//mstr = lowercase(mstr);
						//ResList.insert(ResList.end(),mstr);
						 

						//if (mstrlist.size() > 50)
						//	break;

						 
					}
			
			}

		 i++;	 
		}
	 
		//-------------------------------------------
		ptr = nextptr;		
	  
	}



		


	double SpamRate = 0.5;
	if (m_tlistRes.size > 0)
	{
		GetSpamData(&m_tlistRes);	
		SpamRate = CountSpamRate(&m_tlistRes);

		if (SpamRate == 0.5)
		{
			if (Header_From[0] != 0)
			{
				/*Token *atoken = new Token;
				memset(atoken,0,sizeof(Token));

				if (strlen(Header_From) <= sizeof(atoken->TokenData))
				{
					strcpy(atoken->TokenData,Header_From);
					AddTokenList(&m_tlist2,atoken);		
				}
				else
				{
					delete atoken;
				}*/

				AddTokenStr(&m_tlist2,Header_From);
			
			}
			
			if (Header_Subject[0] != 0)
			{
			  GetGramKeys(&m_tlist2,Header_Subject);
			}

			//GetGramKeys(&m_tlist2,TextSource);

			GetSpamData(&m_tlist2);
			SpamRate = CountSpamRate(&m_tlist2);

		
		}


	}


	
	return SpamRate;
}

void  CChecker2::UpdateSpamRate(int OKFix, int BadFix)
{
	if (m_tlist.size >0) UpdateListSpamRate(&m_tlist,OKFix,BadFix);
	if (m_tlist2.size >0) UpdateListSpamRate(&m_tlist2,OKFix,BadFix);
	if (m_tlistRes.size >0) UpdateListSpamRate(&m_tlistRes,OKFix,BadFix);

}

void CChecker2::UpdateListSpamRate(TokenList *list , int OKFix, int BadFix)
{

	CDB db(mDBSection,IndexDbPath,overflowDbPath,datadbPath,0,1024 * 10);

	Token *ptr = NULL; 
	ptr = list->m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		CheckDataFile df;
		memset(&df,0,sizeof(df));

			if (BadFix > OKFix)
			{
				ptr->SpamCount ++;
			}
			else if(BadFix < 0 || OKFix > 0) //學習為正常或正常
			{
				ptr->SpamCount = 0;
			}

			ptr->SpamRate = ptr->SpamRate + BadFix;
			ptr->OkRate = ptr->OkRate + OKFix;


		if (ptr->SearchRes > 0 )
		{
			//update					
			df.OkRate = ptr->OkRate;
			df.SpamRate = ptr->SpamRate;
			df.SpamCount = ptr->SpamCount;
			df.Status = ptr->Status;
			df.UsedCount = ptr->UsedCount;

			time_t now;
			time(&now);

			//CTime ct;
			//ct = CTime::GetCurrentTime();
			long tvalue = (long) now;

			df.LastUsedTime = tvalue;

			db.UpdateData((char*) &df , sizeof(df),ptr->SearchRes,0);
		
		}
		else if (strlen(ptr->TokenData) < DefaultKeyDataSize)
		{
			
			df.OkRate = ptr->OkRate;
			df.SpamRate = ptr->SpamRate;
			df.SpamCount = ptr->SpamCount;
			df.Status = ptr->Status;
			df.UsedCount = ptr->UsedCount;

			//CTime ct;
			//ct = CTime::GetCurrentTime();
			time_t now;
			time(&now);

			long tvalue = (long) now;

			df.LastUsedTime = tvalue;

			//insert 
			int datapos = db.InsertData((char *)&df,sizeof(df));

			 
		 

			int res = db.InsertKey(ptr->TokenData,datapos);
			if (res == -1)
			{
				//Insert 失敗 , 處理 data
				/*
		
				   如果先做 select key 就不會進到這邊
				*/
				df.Status = 0; //empty
				db.UpdateData((char *) &df,sizeof(df.Status),datapos,FIELD_OFFSET(CheckDataFile,Status));

			}
			 

		}
	 
		ptr = nextptr;		
	  
	}



}

int CChecker2::ParseKey(TokenList* list , char *Source ,  char* CutStr)
{
	
	if (Source == NULL) return 0;

	int AddCount  = 0;
	int CutStrLen = strlen(CutStr);

	int  save2 = -1 ,save3 = 0;
	int i = 0;
	int StrLen = strlen(Source);
	while( i <= StrLen )
	{

		if (save2 == -1)
		{
			
			if (strncmp(Source + i , CutStr,CutStrLen) == 0)			
			{
				
				save2 = i + CutStrLen;
				i += CutStrLen;				
				save3 = 0;		 
				
			}
			
		}
		else
		{
		
			 
			if (*(Source + i) == '/')
			{
				save3 = i;
			
			}else
			//等待字尾 , 使用次數多可以改用 map table
			if (*(Source + i) == ' ' ||
				*(Source + i) == '"'  ||
				*(Source + i) == '>' ||
				*(Source + i) == '<' ||
				*(Source + i) == '\'' ||
				*(Source + i) == '?' ||
				*(Source + i) == 13 ||
				(*(Source + i) & 0xFF) > 128 ||
				i   == StrLen
				)            
			{
			
				bool breaktag = false;
				int tmpi = 0;
				if (save3 > 0)
				{
				  tmpi = save3 + 1; // / 保留
				  breaktag = true;
				}
				else
				{
				  tmpi = i;
				}
				
				if (tmpi - save2 > 2)
				{
					
					//std::string mstr;
					//mstr.append(save2 + Source, tmpi - save2 );

					char *tmpstr = new char[tmpi - save2+5];
					memset(tmpstr,0,tmpi - save2+5);
					strncpy(tmpstr,save2 + Source, tmpi - save2);

					if (!breaktag)
					{
						//mstr.append("//",1);
						strcat(tmpstr,"//");
					}
				
					
					/*char *mstr = _strlwr(tmpstr);
					Token *atoken = new Token;
					memset(atoken,0,sizeof(Token));

					if (strlen(mstr) <= sizeof(atoken->TokenData))
					{
					  strcpy(atoken->TokenData,tmpstr);
					  AddTokenList(list,atoken);
					  AddCount ++;
					}
					else
					{

						delete atoken;
					}*/

					//先做 decode
					

					CMailCodec MailCodec;
						//Decode Unicode
					char *DecodeStr = MailCodec.UniDecode((char *) tmpstr);
					//tmpstdstr = DecodeStr;
					strcpy(tmpstr,DecodeStr);
					MailCodec.FreeUniDecode(DecodeStr);

					//Decode QP % style
					DecodeStr = MailCodec.QPDecode((char *) tmpstr,"%");
					//tmpstdstr = DecodeStr;
					strcpy(tmpstr,DecodeStr);

					MailCodec.FreeQPDecode(DecodeStr);

					if (AddTokenStr(list,tmpstr))
						AddCount ++;
					

					delete [] tmpstr;
					

					//KeyList->insert(KeyList->end(),mstr);
					
				}

				 
				 
				save2 = -1;
			}


		
		}


	 i++;
	}


	return AddCount;
}

char CChecker2::GetRecvHeaderIP(int ReceivedIdent, char *IP)
{
	char rc = 0;

	int LocalLanCount = 0;
	
	//取 ip					
				bool IPfound = false;
				for(int k = ReceivedIdent ; k < Header_Receieved_Count ; k ++)
				{
					    if (IPfound) break;

						char TempIP[17];
						memset(TempIP,0,17);
						int DotCount = 0;
						int idx = 0;
						int idxpos = -1;
						int len = strlen(Header_Received[k]);

						//如果最後是 digital 則補上空白
						if (isdigit(Header_Received[k][len-1]))
						{
							Header_Received[k][len] = ' ';
							Header_Received[k][len+1] = 0;
							len++;
						}
						
						
					    for (int i = 0 ; i < len ; i++)
						{
						
							if (isdigit(Header_Received[k][i]) ) //遇防 Seednet 的 Received: from [218.84.134.166] (port=3228 helo=139.175.54.239)
							{
							  if (Header_Received[k][i-1] == '=')
							    break;

								if (idxpos == -1)
									idxpos = i;

								TempIP[idx] = Header_Received[k][i];
								idx ++;
							}
							else if (Header_Received[k][i] == '.' &&  idx > 0)
							{
								TempIP[idx] = Header_Received[k][i];
								idx ++;
								DotCount ++;

								if (i - idxpos > 3 || idx >= 16)
								{
									//不是ip
									idx = 0;
									DotCount = 0;
									idxpos = -1;
									memset(TempIP,0,17);
								}
								else if (DotCount == 2)
								{
									if (strncmp(TempIP,"192.168",7) == 0 || strncmp(TempIP,"10.",3) == 0 ||strncmp(TempIP,"127.0",5) == 0)
									{ 
										
										LocalLanCount++;
										break;
							 
									} 
									else if (strncmp(TempIP,"172.",4) == 0)
									{
										//locallan
										char LocalLanCheckStr[7];
										memset(LocalLanCheckStr,0,7);
										if (idx - 4 - 1 > 0)
										strncpy(LocalLanCheckStr,TempIP + 4,idx - 4 - 1);

										int IpValue = atoi(LocalLanCheckStr);
										if (IpValue >= 16 && IpValue <= 31)
										{													
										    LocalLanCount++; 
											break;										
										}						
									}
								
								}

								idxpos = i+1;
								
							}
							else
							{
								if (DotCount == 3 && TempIP[idx] != '.') //預防 adsl-126.49.155.info.com.ph ([222.126.49.155]) 
								{
									TempIP[idx+1] = 0; //得到 IP 了
									IP[0] = 0; //ip ini	

									

										//第一個有效 IP 產生
										if (strlen(TempIP) > 7)
										{											
											/*DataFileRes mdatafile;
											memset(&mdatafile,0,sizeof(SDataFile));

											mdatafile = db->GetSpamData(TempIP);

											if (mdatafile.SearchRes != -1)
											{
												if (mdatafile.datafile.Status == 3)
												{
													//skip gateway
												}
												else
												{
													strcpy(IP,TempIP);
													IPfound = true;
												//	return 1; //先取第一個就好 for dogid : 904
												
												}
											}
											else
											{*/
												strcpy(IP,TempIP);
												IPfound = true;
												//return 1; //先取第一個就好
											//}

										
											//return 1;
										}
										//else
										//{
						
										    //idx = 0;
											//DotCount = 0;
											//idxpos = -1;
											//memset(TempIP,0,16);
							 
										//}

						
								}
							
									idx = 0;
									DotCount = 0;
									idxpos = -1;
									memset(TempIP,0,17);
							}
						}
				}


	if (LocalLanCount == Header_Receieved_Count)
	{
		rc = 1; 
	}

	return rc;
}

double CChecker2::CountSpamRate(TokenList *list,double LimitRate)
{
	int indentcount = 0;
	double SpamRate = 0.5 , tmpSpamRate = 0,TotalSpamRate = 1, TotalOkRate = 1;
    double DataOkRate = 0.0 , DataSpamRate = 0.0;

	Token *ptr = NULL;
	ptr = list->m_TokenList;
	while (ptr)
	{	  
	    
	    Token *nextptr = ptr->next;

		if (ptr->SearchRes > 0)
		{
			if (ptr->SpamRate != 0 || ptr->OkRate != 0)
			{
			  indentcount++;
			} 

			DataOkRate = ptr->OkRate;
			DataSpamRate = ptr->SpamRate;

			if (DataOkRate < 0 ) DataOkRate = 0;
			if (DataSpamRate < 0) DataSpamRate = 0;		

			if (DataOkRate == 0 && DataSpamRate == 0)
			{
					tmpSpamRate = 0.5;		
			}
			else
			{
				double KeyTotalRate = DataOkRate + DataSpamRate;

				tmpSpamRate = min(((double)DataSpamRate / KeyTotalRate),1);
				tmpSpamRate = tmpSpamRate / (tmpSpamRate + min(((double) DataOkRate / (double)KeyTotalRate),1));
				tmpSpamRate = min(tmpSpamRate,0.99);
				tmpSpamRate = max(tmpSpamRate,0.01);
			}		

			
			 TotalSpamRate = TotalSpamRate * tmpSpamRate;			
			 TotalOkRate = TotalOkRate * (1-tmpSpamRate);	  
		}

		ptr = nextptr;		
	  
	}

    SpamRate = TotalSpamRate / (TotalSpamRate + TotalOkRate);
	 

	//無法辨識不可超過 設定
	if (indentcount  <= (int) ((list->size * LimitRate) + 0.5) || SpamRate == 0.5 )
	{
		SpamRate = 0.5;
	}

	return SpamRate;
}

void CChecker2::GetGramKeys(TokenList* list , char* InStr)
{

	//(Body[i] & 0xFF) > 128
	if (InStr == NULL) return ;

	char  MultiStr[255] = {0};
	char  SingleStr[255] = {0};

	int len = strlen(InStr);

	for (int i = 0 ; i < len ; i ++)
	{
		if ((InStr[i] & 0xFF) > 128)
		{
			//MultiStr.append(InStr+i,2);
			if (strlen(MultiStr) < 255)
			{
				strncat(MultiStr,InStr+i,2);
				i++;
			}
		}
		else
		{
			if (strlen(SingleStr) < 255)
			{
			  //SingleStr.append(InStr+i,1);
			  strncat(SingleStr,InStr+i,1);
			}
		}

	
	}

	//multi byte
	//const char *tmpp = MultiStr.c_str();
	int multisize = strlen(MultiStr);
	for (int j = 0 ; j < multisize; j +=4)
	{
	
		char tempstr[5];
		memset(tempstr,0,5);
		memcpy(tempstr,MultiStr+j,4);

		//SecResList.insert(SecResList.end(),tempstr);
		//Token *atoken = new Token;
		//memset(atoken,0,sizeof(Token));
		
		//strcpy(atoken->TokenData,tempstr);
		//AddTokenList(list,atoken);	
		if (list->size >= MaxKeyNum) break;
		AddTokenStr(list,tempstr);
	
	}

	//sigle
	int SavePos = 0;
	
	int singlesize = strlen(SingleStr);
	for (int k = 0 ; k < singlesize ; k ++)
	{
	
		if (SingleStr[k] == ' ')
		{
	
			char tempstr[24]; //largest size
			memset(tempstr,0,24);

			if (k - SavePos <= 24  && k - SavePos >= 4)  //最短 4 最長 24
			{
				memcpy(tempstr,SingleStr+SavePos,k - SavePos);
				//SecResList.insert(SecResList.end(),tempstr);
				//Token *atoken = new Token;
				//memset(atoken,0,sizeof(Token));
		
				//strcpy(atoken->TokenData,tempstr);
				//AddTokenList(list,atoken);
				if (list->size >= MaxKeyNum) break;
				AddTokenStr(list,tempstr);
			}
		
			SavePos = k + 1;
		} 
		else if(k + 1  == singlesize)
		{
		
			char tempstr[24]; //largest size
			memset(tempstr,0,24);

			if (k + 1 - SavePos <= 24  && k + 1 - SavePos >= 4)  //最短 4 最長 24
			{
				memcpy(tempstr,SingleStr+SavePos,k + 1 - SavePos);
				//SecResList.insert(SecResList.end(),tempstr);
				//Token *atoken = new Token;
				//memset(atoken,0,sizeof(Token));
		
				//strcpy(atoken->TokenData,tempstr);
				//AddTokenList(list,atoken);
				if (list->size >= MaxKeyNum) break;
				AddTokenStr(list,tempstr);
			}
		
		}
		
		
	
	}

	if (SavePos ==0 && singlesize >= 4 && singlesize <= 24)
	{	
		//SecResList.insert(SecResList.end(),SingleStr);
		//Token *atoken = new Token;
		//memset(atoken,0,sizeof(Token));
		
		//strcpy(atoken->TokenData,SingleStr);
		//AddTokenList(list,atoken);

		 
		AddTokenStr(list,SingleStr);
	}



}