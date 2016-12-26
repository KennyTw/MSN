// Checker2.h: interface for the CChecker2 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHECKER2_H__0E8ED179_60E0_4379_9381_176850101B2F__INCLUDED_)
#define AFX_CHECKER2_H__0E8ED179_60E0_4379_9381_176850101B2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef DLLDIR_EX
   #define DLLDIR  __declspec(dllexport)   // export DLL information
#else   
   #define DLLDIR  __declspec(dllimport)   // import DLL information    
#endif 

#define CheckDefaultKeyDataSize 255
#define MaxKeyNum  50

typedef struct _CheckDataFile
{	

	unsigned int OkRate;
	unsigned int SpamRate;
	unsigned int UsedCount;
    unsigned int SpamCount; //new
	long LastUsedTime;
	char Status; //0: Free ,1 : used , 2 : Ip Data , 3 : Public GateWay , 4 : ENews

} CheckDataFile;

typedef struct _Token
{

	char TokenData[CheckDefaultKeyDataSize];	
	unsigned int OkRate;
	unsigned int SpamRate;
	unsigned int UsedCount;
    unsigned int SpamCount; //new
	char Status; //0: Free ,1 : used , 2 : Ip Data , 3 : Public GateWay , 4 : ENews
	int SearchRes; //Search Res in Key Data File

	struct _Token *next, *prev;

} Token;

typedef struct _TokenList
{
	unsigned int size;
	Token* m_TokenList;

} TokenList;


class DLLDIR CChecker2  
{
private:
	CRITICAL_SECTION*  mDBSection;

	TokenList m_tlist , m_tlist2, m_tlistRes;
	char* BodySource;
	char* TextSource;
	//char* HeaderSource;
	//char* m_MailContent;

	char *Header_From;
	char *Header_Subject;

	char IndexDbPath[MAX_PATH];
	char overflowDbPath[MAX_PATH];
	char datadbPath[MAX_PATH];

	//void ParseHeader();
	void AddTokenList(TokenList* list,Token *obj);
	void DelTokenList(TokenList* list,Token *obj);
	bool AddTokenStr(TokenList *list,char *AStr);
	void DelTokenStr(TokenList *list,char *AStr);
	Token* FindTokenStr(TokenList *list,char *AStr);
//	int GetCheckKeys(TokenList* list , char* Source);
	void GetSpamData(TokenList* list );
	void TrimHTML(char* InStr, char* OutBuffer);
	int ParseKey(TokenList* list , char *Source ,  char* CutStr);
	int ParseNumKey(TokenList* list , char *Source );
	void GetGramKeys(TokenList* list , char* InStr);
	void ClearList(TokenList *list);
	void GetCurrentPath();
	double CountSpamRate(TokenList *list , double LimitRate = 0.5);
	void UpdateListSpamRate(TokenList *list , int OKFix, int BadFix);
	bool bPassKey(char * KeyStr ,int startpos  );
	

	
public:

	
	char Header_Received[10][255];
	int Header_Receieved_Count ;

	double GetSpamRate(char* From ,char *Subject, char *MailBody , char *SourceIp);
	void UpdateSpamRate(int OKFix, int BadFix);
	char GetRecvHeaderIP(int ReceivedIdent, char *IP);

	CChecker2(CRITICAL_SECTION*  DBSection);
	virtual ~CChecker2();

};

#endif // !defined(AFX_CHECKER2_H__0E8ED179_60E0_4379_9381_176850101B2F__INCLUDED_)
