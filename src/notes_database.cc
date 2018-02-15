#include "notes_database.h"
#include "DataHelper.h"
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <nsfdb.h>
#include <nif.h>
#include <nsfnote.h>
#include <osmem.h>
#include <osmisc.h>
#include <ctime>
#include <stdio.h>

NAN_METHOD(openDatabase) {
	v8::String::Utf8Value val(info[0]->ToString());
  std::string dbName (*val);
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	DBHANDLE    db_handle;      /* handle of source database */
	
	if (error = NSFDbOpen(dbName.c_str(), &db_handle)) {
			DataHelper::GetAPIError(error,error_text);
			printf("error: %s", error_text);
	}
	USHORT dh = (USHORT) db_handle;
  Local<Number> retval = Nan::New((double) dh);
  info.GetReturnValue().Set(retval); 
}

NAN_METHOD(getDatabaseName) {
	double value = info[0]->NumberValue();
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	DBHANDLE    db_handle;      /* handle of source database */
	
	char       title[NSF_INFO_SIZE] = "";   /* database title */
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;
	char       buffer[NSF_INFO_SIZE] = "";  /* database info buffer */
	if (error = NSFDbInfoGet (db_handle, buffer))
	{	
		NSFDbClose (db_handle);	
	}
	
	NSFDbInfoParse (buffer, INFOPARSE_TITLE, title, NSF_INFO_SIZE - 1);
	
	info.GetReturnValue().Set(
         Nan::New<String>(title).ToLocalChecked()); 
}

NAN_METHOD(closeDatabase) {
 	DBHANDLE    db_handle;      /* handle of source database */
	double value = info[0]->NumberValue();
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;
	NSFDbClose (db_handle);		
}


NAN_METHOD(replicationSummary) {
  STATUS					error = NOERROR;     /* return status from API calls */
  DBHANDLE    db_handle;      /* handle of source database */
  DHANDLE      hReplHist;
  REPLHIST_SUMMARY ReplHist;
  REPLHIST_SUMMARY *pReplHist;
  char        szTimedate[MAXALPHATIMEDATE+1];
  WORD        wLen;
  DWORD       dwNumEntries, i;
  char *pServerName;                    /* terminating NULL not included */
  char szServerName[MAXUSERNAME + 1];
  char *pFileName;                      /* includes terminating NULL */
  char szDirection[10];                 /* NEVER, SEND, RECEIVE */
  Local<Array> replicaRes = New<Array>();
	double value = info[0]->NumberValue();
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;

  error = NSFDbGetReplHistorySummary (db_handle, 0, &hReplHist, &dwNumEntries);
  if (error)
  {
    printf("error getting replica history\n");
  }

  pReplHist = OSLock (REPLHIST_SUMMARY, hReplHist);
  for (i = 0; i < dwNumEntries; i++)
  {
    ReplHist = pReplHist[i];
    error = ConvertTIMEDATEToText (NULL, NULL, &(ReplHist.ReplicationTime),
                                   szTimedate, MAXALPHATIMEDATE, &wLen);
    if (error)
    {
      OSUnlock (hReplHist);
      OSMemFree (hReplHist);      
    }
    szTimedate[wLen] = '\0';
    if (ReplHist.Direction == DIRECTION_NEVER)
      strcpy (szDirection, "NEVER");
    else if (ReplHist.Direction == DIRECTION_SEND)
      strcpy (szDirection, "SEND");
    else if (ReplHist.Direction == DIRECTION_RECEIVE)
      strcpy (szDirection, "RECEIVE");
    else
      strcpy (szDirection, "");

    pServerName = NSFGetSummaryServerNamePtr (pReplHist, i);
    strncpy (szServerName, pServerName, ReplHist.ServerNameLength);
    szServerName[ReplHist.ServerNameLength] = '\0';
    /* FileName will be NULL terminated */
    pFileName = NSFGetSummaryFileNamePtr (pReplHist, i);
   
    Local<Object> resDoc = Nan::New<Object>();
    Nan::Set(resDoc, New<v8::String>("date").ToLocalChecked(), New<v8::String>(szTimedate).ToLocalChecked());	
    Nan::Set(resDoc, New<v8::String>("database").ToLocalChecked(), New<v8::String>(pFileName).ToLocalChecked());
    Nan::Set(resDoc, New<v8::String>("server").ToLocalChecked(), New<v8::String>(szServerName).ToLocalChecked());
    Nan::Set(resDoc, New<v8::String>("direction").ToLocalChecked(), New<v8::String>(szDirection).ToLocalChecked());
    Nan::Set(replicaRes, i, resDoc);
  }
  OSUnlock (hReplHist);
  OSMemFree (hReplHist);
  info.GetReturnValue().Set(replicaRes); 
}