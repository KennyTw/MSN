// MSN.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MsnClient.h"
#include <conio.h>


int main(int argc, char* argv[])
{
	CMsnClient *LoginClient = NULL;

Start :
	//µn¤J msn
	    LoginClient = new CMsnClient();

		HANDLE hand = LoginClient->MSNLogin("65.54.52.62",1863);
		WaitForSingleObject(hand , INFINITE);

		char NewIP[15];
		int Newport;

		if (LoginClient->LastStatus == MSN_NEED_RELOGIN)
		{
			strcpy(NewIP,LoginClient->XFRIP);
			Newport = LoginClient->XFRport;
			CloseHandle(hand);

			delete LoginClient;

			LoginClient = new CMsnClient();

			HANDLE hand = LoginClient->MSNLogin(NewIP,Newport);
			//WaitForSingleObject(hand , INFINITE);


		
		}  

	Sleep(1000 * 60); //wait one minute to send
	
    while (1)
	{
		Sleep(5000);
		
		if (!LoginClient->IsConnected) 
		{
			delete LoginClient;
			goto Start;
		}
	 
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		
		char DirSpec[MAX_PATH + 1];  // directory specification
		strcpy(DirSpec,"D:\\MSNMSG\\*.OK");			

		hFind = FindFirstFile(DirSpec, &FindFileData);

		if (hFind != INVALID_HANDLE_VALUE) 
		{
			do 
			{	
				char MsgPath[MAX_PATH + 1];
				char OKPath[MAX_PATH + 1];

				strcpy(MsgPath,"D:\\MSNMSG\\");
				 
				
				strncat(MsgPath,FindFileData.cFileName,strlen(FindFileData.cFileName)-2);

				strcpy(OKPath,MsgPath);

				strcat(MsgPath,"TXT");
				strcat(OKPath,"OK");

                FILE *fp = fopen(MsgPath,"r+b");
				
				fseek(fp,0,SEEK_END);
				int bsize = ftell(fp);

				char *MsgBuff = new char [bsize+1];
				memset(MsgBuff,0,bsize+1);

				fseek(fp,0,SEEK_SET);

				fread(MsgBuff,1,bsize,fp);
				fclose(fp);

					WCHAR*  strA; 
			
					int  i=  MultiByteToWideChar(CP_ACP,0,(char*)  MsgBuff,-1,NULL,0);  						
					strA  =  new  WCHAR[i];  
					MultiByteToWideChar  (CP_ACP,0,(char *) MsgBuff,-1,strA,i);   
					i=  WideCharToMultiByte(CP_UTF8,0,strA,-1,NULL,0,NULL,NULL);  
					char  *strB=new  char[i];  
					WideCharToMultiByte  (CP_UTF8,0,strA,-1,strB,i,NULL,NULL);  

				
				    LoginClient->SendMsg("kenny@ezfly.com",strB);

					delete [] strA ;
					delete [] strB ;

			
				delete MsgBuff;			


					DeleteFile(MsgPath);
					DeleteFile(OKPath);

					Sleep(5000); //wait 5 sec



			} while (FindNextFile(hFind, &FindFileData) != 0) ;
    
      
			FindClose(hFind);		
		
		}

 
	
	}

	getch();


	return 0;
}

