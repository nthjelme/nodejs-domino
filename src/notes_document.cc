#include "notes_document.h"
#include "DataHelper.h"
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <nsfdb.h>
#include <pool.h>
#include <mimeods.h>
#include <ods.h>
#include <mimedir.h>
#include <mime.h>
#include <nif.h>
#include <nsfnote.h>
#include <nsfmime.h>
#include <osmisc.h>
#include <osmem.h>
#include <ctime>
#include <stdio.h>

#define BUFSIZE 185536

char * fileContent; // global to keep mime fileContent.
std::vector<std::string> strVec;

NAN_METHOD(closeNote) {
  //v8::Isolate* isolate = info.GetIsolate();
  NOTEHANDLE   note_handle;
	double value = info[0]->NumberValue();
	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;
	NSFNoteClose(note_handle);	
}

NAN_METHOD(getNoteById) {
	double value = info[0]->NumberValue();
	double note_val(info[1]->NumberValue());
 	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	DBHANDLE    db_handle;      /* handle of source database */
	DWORD noteid = (DWORD) note_val;
	
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;
	if (error = NSFNoteOpenExt(db_handle, noteid,
                                  OPEN_RAW_MIME_PART, &note_handle)) {
		printf("error opening notebyid \n");
			DataHelper::GetAPIError(error,error_text);
			//SetErrorMessage(error_text);
			printf("get note by id, error %s\n",error_text);

	}
	USHORT nh = (USHORT) note_handle;
  Local<Number> retval = Nan::New((double) nh);
  info.GetReturnValue().Set(retval); 
}

NAN_METHOD(getNotesNote) {
  double value = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
  std::string unidStr (*val);
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	DBHANDLE    db_handle;      /* handle of source database */
	UNID temp_unid;
	NOTEID tempID;
	char       title[NSF_INFO_SIZE] = "";   /* database title */
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;

	DataHelper::ToUNID(unidStr.c_str(), &temp_unid);

		if (error = NSFNoteOpenByUNID(
			db_handle,  /* database handle */
			&temp_unid, /* note ID */
			(WORD)0,                      /* open flags */
			&note_handle))          /* note handle (return) */
		{
			DataHelper::GetAPIError(error,error_text);
			//SetErrorMessage(error_text);
			printf("get note by unid, error %s\n",error_text);
			NSFDbClose(db_handle);
			free(error_text);
			return;
		}
		
		// repoen note by by note id
		NSFNoteGetInfo(note_handle, _NOTE_ID, &tempID);
		NSFNoteClose(note_handle);
		if (error = NSFNoteOpenExt(db_handle, tempID,
                                  OPEN_RAW_MIME_PART, &note_handle)) {
			printf("error opening notebyid \n");
			DataHelper::GetAPIError(error,error_text);
			//SetErrorMessage(error_text);
			printf("get note by id, error %s\n",error_text);

		}
		USHORT nh = (USHORT) note_handle;

  	Local<Number> retval = Nan::New((double) nh);
  	info.GetReturnValue().Set(retval); 

  
}

NAN_METHOD(createNotesNote) {
	double value = info[0]->NumberValue();
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	DBHANDLE    db_handle;      /* handle of source database */
	unsigned short db_h = (unsigned short) value;
	db_handle = (DHANDLE)db_h;

	if (error = NSFNoteCreate(db_handle, &note_handle)) {
		printf("error creating document\n");
	}
	USHORT nh = (USHORT) note_handle;
  Local<Number> retval = Nan::New((double) nh);
  info.GetReturnValue().Set(retval);
}

NAN_METHOD(updateNote) {
	double value = info[0]->NumberValue();
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;    

	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;
	
	if (error = NSFNoteUpdate(note_handle, 0)) {
		printf("error updating note\n");
	}
}

NAN_METHOD(getNoteUNID) {
	double value = info[0]->NumberValue();
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	OID tempOID;

	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;
	NSFNoteGetInfo(note_handle, _NOTE_OID, &tempOID);
	char unid_buffer[33];
	snprintf(unid_buffer, 33, "%08X%08X%08X%08X", tempOID.File.Innards[1], tempOID.File.Innards[0], tempOID.Note.Innards[1], tempOID.Note.Innards[0]);
	info.GetReturnValue().Set(  Nan::New<String>(unid_buffer).ToLocalChecked()); 
}

NAN_METHOD(getNoteID) {
	double value = info[0]->NumberValue();
	NOTEHANDLE   note_handle;
	
	DWORD tempID;
	STATUS					error = NOERROR;     /* return status from API calls */
	
	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;
	
	NSFNoteGetInfo(note_handle, _NOTE_ID, &tempID);
	Local<Number> retval = Nan::New((double) tempID);
  info.GetReturnValue().Set(retval); 

}


NAN_METHOD(setItemText) {
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value itemName(info[1]->ToString());
	v8::String::Utf8Value val(info[2]->ToString());
	
	std::string itemStr (*itemName);
	std::string itemValue (*val);	
	nsfItemSetText((unsigned short) note_handle,itemStr.c_str(),itemValue.c_str());
	
}

void nsfItemSetText(unsigned short nHandle, const char * itemName, const char * itemValue) {
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	note_handle = (NOTEHANDLE)nHandle;
	OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, itemValue, MAXWORD, buf, MAXWORD);
	if (error = NSFItemSetText(note_handle, itemName,buf,MAXWORD)) {
		DataHelper::GetAPIError(error, error_text);
		printf("Error writing text item%s\n",error_text);
		free(error_text);
	}
}

std::string nsfItemGetMime(unsigned short nHandle, const char *itemName) {
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	BLOCKID                bidLinksItem;
	DWORD                  dwLinksValueLen;
	BLOCKID                bidLinksValue;
	WORD                   item_type;
	BOOL bMimeDataInFile;
	char pchFileName[512];
	NOTEHANDLE   note_handle;
	note_handle = (NOTEHANDLE)nHandle;
	
	if (error = NSFItemInfo(note_handle, itemName,
		strlen(itemName), &bidLinksItem,
		&item_type, &bidLinksValue,
		&dwLinksValueLen)) {		
		DataHelper::GetAPIError(error, error_text);
		printf("Error getting item info: %s\n",error_text);
		return NULL;
	}

	bMimeDataInFile = NSFIsMimePartInFile(note_handle, bidLinksItem, pchFileName,(WORD)512);
		
		if (bMimeDataInFile) {
			BLOCKID bkItem;
			WORD field_type;
			BLOCKID field_block;
			DWORD field_length;
			
			NSFItemInfo (
            note_handle,        /* note handle */
            "$FILE",        /* field we want */
            (WORD)strlen("$FILE"),    /* length of above */
            &bkItem,            /* full field (return) */
            &field_type,        /* field type (return) */
            &field_block,        /* field contents (return) */
            &field_length);        /* field length (return) */
			error = NSFNoteExtractWithCallback(note_handle, bkItem, NULL, 0,ExtractwithCallback, (void*)0);
			std::string ret;
    	for(const auto &s : strVec) {        
        ret += s;
    	}

			return ret;
			
		} else {
			char *pMime; 
			WORD wPartType;
			DWORD dwFlags;
			WORD wReserved;
			WORD wPartOffset;
			WORD wPartLen;
			WORD wBoundaryOffset;
			WORD wBoundaryLen;
			WORD wHeadersOffset;
			WORD wHeadersLen;
			WORD wBodyOffset;
			WORD wBodyLen;
	
			DHANDLE hPart;
			if (error = NSFMimePartGetPart(bidLinksItem, &hPart)) {
				printf("error getting mime part\n");
			}
			
			if (error = NSFMimePartGetInfoByBLOCKID(bidLinksItem,
   				&wPartType,
   				&dwFlags,
   				&wReserved,
   				&wPartOffset,
   				&wPartLen,
   				&wBoundaryOffset,
	   			&wBoundaryLen,
   				&wHeadersOffset,
   				&wHeadersLen,
   				&wBodyOffset,
   				&wBodyLen)) {
			  printf("errr getting mimepartinfobyblockid\n");
		 		return NULL;
			}
		
			pMime = OSLock(char, hPart);
			char *pText;
			pText = (char *)pMime + wBoundaryLen+wHeadersLen;			
			char *fieldText = (char *)malloc(wBodyLen);		
			memcpy(fieldText, pText, wBodyLen); 
			fieldText[wBodyLen] = '\0';
			OSUnlock(hPart); 
			free(hPart);
			std::string bodyValue = std::string(fieldText);
			free(fieldText);
			return bodyValue;
		}
}

void nsfItemSetMime(unsigned short nHandle, const char * itemName, const char * itemValue, const char *headers) {
	STATUS error;
	              
	WORD PARTLEN = ((strlen(headers)-1)+(strlen(itemValue)-1));
  char *achPart = (char *)malloc(PARTLEN+1);
	NOTEHANDLE   note_handle;
	note_handle = (NOTEHANDLE)nHandle;
  char *item_name = (char *)malloc(strlen(itemName));
	strcpy(item_name,itemName);
  strcpy(achPart, headers);
  strcat(achPart, itemValue);	
  error = NSFMimePartAppend(note_handle,
				    item_name,
				    strlen(item_name),
				    MIME_PART_BODY,
				    MIME_PART_HAS_HEADERS,
				    achPart,
				    PARTLEN);
  if (error) {
		char *error_text = (char *) malloc(sizeof(char) * 200);   
		DataHelper::GetAPIError(error, error_text);
  	printf("Error writeing mime item. %s\n",error_text);
		free(error_text);
  }
	free(item_name);
	free(achPart);

}


NAN_METHOD(getItemValue) {
	double value = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	BLOCKID                bidLinksItem;
	DWORD                  dwLinksValueLen;
	BLOCKID                bidLinksValue;
	WORD                   item_type;
	BOOL bMimeDataInFile;
	char pchFileName[512];
	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;

	if (error = NSFItemInfo(note_handle, itemName.c_str(),
		strlen(itemName.c_str()), &bidLinksItem,
		&item_type, &bidLinksValue,
		&dwLinksValueLen)) {		
		DataHelper::GetAPIError(error, error_text);
		printf("Error getting item info: %s\n",error_text);
		return;
	}

	
	
	if (item_type == TYPE_TEXT) {
		char buf[MAXWORD];
		nsfGetItemText(n_h,itemName.c_str(),buf);
		info.GetReturnValue().Set(
  	Nan::New<String>(buf).ToLocalChecked()); 
		
	} else if (item_type == TYPE_NUMBER) {
		double numberValue = nsfItemGetNumber(n_h, itemName.c_str());
		Local<Number> retval = Nan::New(numberValue);
  	info.GetReturnValue().Set(retval);
	} else if (item_type == TYPE_TEXT_LIST) {
		WORD field_len;
		char field_text[MAXWORD];
		WORD num_entries = NSFItemGetTextListEntries(note_handle,itemName.c_str());
		std::vector<char*> vectorStrValue = std::vector<char*>(0);
		Local<Array> arr = New<Array>();
		for (size_t j = 0; j < num_entries; j++) {
			field_len = NSFItemGetTextListEntry(note_handle,itemName.c_str(), j, field_text, (WORD)(sizeof(field_text) - 1));
			char *buf = (char*)malloc(field_len + 1);
			OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, buf, MAXWORD);
			Nan::Set(arr, j, Nan::New<String>(buf).ToLocalChecked());
		}
		info.GetReturnValue().Set(arr);
		
	} else if (item_type == TYPE_TIME) {
		double dateTimeValue = nsfGetItemDate(n_h,itemName.c_str());
		Local<v8::Date> retval = New<v8::Date>(dateTimeValue).ToLocalChecked();
  	info.GetReturnValue().Set(retval);
	} else if (item_type== TYPE_MIME_PART) {
		std::string mime = nsfItemGetMime(n_h, itemName.c_str());
		/*bMimeDataInFile = NSFIsMimePartInFile(note_handle, bidLinksItem, pchFileName,(WORD)512);
		
		if (bMimeDataInFile) {
			BLOCKID bkItem;
			WORD field_type;
			BLOCKID field_block;
			DWORD field_length;
			
			
			NSFItemInfo (
            note_handle,      
            "$FILE",     
            (WORD)strlen("$FILE"),
            &bkItem,
            &field_type,
            &field_block,
            &field_length);
			error = NSFNoteExtractWithCallback(note_handle, bkItem, NULL, 0,ExtractwithCallback, (void*)0);
			std::string ret;
    	for(const auto &s : strVec) {        
        ret += s;
    	}
		
			info.GetReturnValue().Set(
  		Nan::New<String>(ret).ToLocalChecked()); 
			
		} else {
			char *pMime; 
			WORD wPartType;
			DWORD dwFlags;
			WORD wReserved;
			WORD wPartOffset;
			WORD wPartLen;
			WORD wBoundaryOffset;
			WORD wBoundaryLen;
			WORD wHeadersOffset;
			WORD wHeadersLen;
			WORD wBodyOffset;
			WORD wBodyLen;
	
			DHANDLE hPart;
			if (error = NSFMimePartGetPart(bidLinksItem, &hPart)) {
				printf("error getting mime part\n");
			}
			
			if (error = NSFMimePartGetInfoByBLOCKID(bidLinksItem,
   				&wPartType,
   				&dwFlags,
   				&wReserved,
   				&wPartOffset,
   				&wPartLen,
   				&wBoundaryOffset,
	   			&wBoundaryLen,
   				&wHeadersOffset,
   				&wHeadersLen,
   				&wBodyOffset,
   				&wBodyLen)) {
			  printf("errr getting mimepartinfobyblockid\n");
		 		return;
			}
		
			pMime = OSLock(char, hPart);
			char *pText;
			pText = (char *)pMime + wBoundaryLen+wHeadersLen;			
			char *fieldText = (char *)malloc(wBodyLen);		
			memcpy(fieldText, pText, wBodyLen); 
			fieldText[wBodyLen] = '\0';
			OSUnlock(hPart); 
			free(hPart);
			*/
			info.GetReturnValue().Set(
  		Nan::New<String>(mime).ToLocalChecked()); 
			//free(fieldText);
		
		
	}
}

STATUS LNCALLBACK ExtractwithCallback(const BYTE* byte, DWORD length, void far* pParam)
{	
	char *localstr = (char*)byte;

	if (localstr != NULL) {
		char *mimeContentPart = (char*)malloc(length + 1);
		strncpy(mimeContentPart, localstr, length);
		mimeContentPart[length] = '\0';
		strVec.push_back(mimeContentPart);
	}
	
	return NOERROR;
}

NAN_METHOD(getItemText) {
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemStr (*val);
	
	char buf[MAXWORD];

	nsfGetItemText((unsigned short) note_handle,itemStr.c_str(),buf);
	info.GetReturnValue().Set(
  	Nan::New<String>(buf).ToLocalChecked()); 
}

NAN_METHOD(hasItem) {
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);
	
	v8::Local<v8::Boolean> isPresent = Nan::False();
  if (hasItem((unsigned short)note_handle,itemName.c_str())) {
		isPresent = Nan::True();
	}
	info.GetReturnValue().Set(isPresent);
}

NAN_METHOD(deleteItem) {
	
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);
	STATUS error = NOERROR;
	char *error_text = (char *) malloc(sizeof(char) * 200);
	v8::Local<v8::Boolean> deleted = Nan::True();
	if (error = nsfDeleteItem((unsigned short)note_handle,itemName.c_str())) {
		deleted = Nan::False();
		DataHelper::GetAPIError(error, error_text);
		printf("Error deleting item: %s\n",error_text);
		free(error_text);
	}
	info.GetReturnValue().Set(deleted);
}

NAN_METHOD(setItemDate) {	
	double dNote_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	double unix_time;
	Local<Value> dateValue =info[2];
	if (dateValue->IsDate()) {			
			unix_time = v8::Date::Cast(*dateValue)->NumberValue();
	}
	unsigned short usNHandle = (unsigned short) dNote_handle;
	
	std::string itemName (*val);
	nsfSetItemDate(usNHandle,itemName.c_str(),unix_time);
}

void nsfSetItemDate(unsigned short usNHandle, const char * itemName, double unix_time) {
	NOTEHANDLE note_handle;
	STATUS error = NOERROR;
	char *error_text = (char *) malloc(sizeof(char) * 200);
	TIME tid;
  note_handle = (NOTEHANDLE)usNHandle;
	std::time_t t = static_cast<time_t>(unix_time / 1000);
	struct tm* ltime = localtime(&t);
	tid.year = ltime->tm_year + 1900;
	tid.month = ltime->tm_mon + 1;
	tid.day = ltime->tm_mday;
	tid.hour = ltime->tm_hour;
	tid.minute = ltime->tm_min + 1;
	tid.second = ltime->tm_sec-1;
	tid.zone = 0;
	tid.dst = 0;
	tid.hundredth = 0;

	if (error = TimeLocalToGM(&tid)) {
		DataHelper::GetAPIError(error, error_text);
		printf("Error converting Time Local to GM, %s\n",error_text);
	}

	if (error = NSFItemSetTime(note_handle, itemName, &tid.GM)) {
		DataHelper::GetAPIError(error, error_text);
		printf("Error setting time on item, %s",error_text);
	}
}
NAN_METHOD(setMimeItem) {	
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value itemName(info[1]->ToString());
	v8::String::Utf8Value val(info[2]->ToString());
	v8::String::Utf8Value header(info[3]->ToString());
	
	std::string itemStr (*itemName);
	std::string itemValue (*val);	
	std::string headerValue (*header);
	headerValue.append("\015\012\015\012");
	itemValue.append("\015\012");
	
	nsfItemSetMime((unsigned short) note_handle,itemStr.c_str(),itemValue.c_str(),headerValue.c_str());
}

NAN_METHOD(getMimeItem) {	
	NOTEHANDLE   note_handle;	
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);
	double n_h = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);
	unsigned short nHandle = (unsigned short) n_h;
	//note_handle = (NOTEHANDLE)nHandle;
	if (hasItem(nHandle,itemName.c_str())) {
		std::string mime = nsfItemGetMime(nHandle, itemName.c_str());
		info.GetReturnValue().Set(
	  	Nan::New<String>(mime).ToLocalChecked()); 
	} else {
		printf("can't find mime item by name: %s\n ", itemName.c_str());
		info.GetReturnValue().Set(
	  	Nan::New<String>("").ToLocalChecked()); 
	}

}

NAN_METHOD(appendItemTextList) {	
	double n_h = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	v8::String::Utf8Value iValue(info[2]->ToString());
	
	std::string itemValue (*iValue);	
	std::string itemName (*val);
	
	nsfItemAppendTextList((unsigned short) n_h,itemName.c_str(),itemValue.c_str());
}

NAN_METHOD(setAuthor) {
	NOTEHANDLE   note_handle;	
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);
	double n_h = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	v8::String::Utf8Value iValue(info[2]->ToString());
	
	std::string itemValue (*iValue);	
	std::string itemName (*val);
	
	unsigned short nHandle = (unsigned short) n_h;
	note_handle = (NOTEHANDLE)nHandle;
	if (error = NSFItemAppend(note_handle,ITEM_SUMMARY | ITEM_READWRITERS | ITEM_NAMES,
                               itemName.c_str(), strlen(itemName.c_str()),
                               TYPE_TEXT, itemValue.c_str(),
                               (DWORD) strlen(itemValue.c_str()))) 
	{
		DataHelper::GetAPIError(error, error_text);
		printf("Error settin author on item: %s\n",error_text);
	}
	
}

BOOL hasItem(unsigned short nHandle,const char * itemName) {
	NOTEHANDLE   note_handle;
	BOOL         field_found;
	note_handle = (NOTEHANDLE)nHandle;
	field_found = NSFItemIsPresent (note_handle, itemName, (WORD) strlen (itemName));
	return field_found;
}

STATUS nsfDeleteItem(unsigned short nHandle,const char * itemName) {
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;
	note_handle = (NOTEHANDLE)nHandle;
	error = NSFItemDelete (note_handle, itemName, (WORD) strlen (itemName));
	return error;
}

double nsfItemGetNumber(unsigned short nHandle, const char * itemName) {
	NOTEHANDLE   note_handle;
	NUMBER       number_field;
	BOOL         success;	
	note_handle = (NOTEHANDLE)nHandle;

  success = NSFItemGetNumber (note_handle, itemName, &number_field);
	if (!success) {
		printf("Error getItemNumber for item %s\n",itemName);
	}
	return (double) number_field;
}

void nsfGetItemText(unsigned short nHandle, const char * itemName,char *value) {
	NOTEHANDLE   note_handle;
	char         field_text[MAXWORD];
	WORD         field_len;	
	BOOL         field_found;	
	
	note_handle = (NOTEHANDLE)nHandle;

  field_len = NSFItemGetText ( 
                    note_handle, 
                    itemName,
                    field_text,
                    (WORD) sizeof (field_text));
    
	OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, value, MAXWORD);
}

NAN_METHOD(setItemNumber) {  
  double handle_value = info[0]->NumberValue();
  double value = info[2]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemStr (*val);			
	nsfItemSetNumber(handle_value,itemStr.c_str(),&value);
}

NAN_METHOD(setItemValue) {
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);	
	Local<Value> value = info[2];
	if (value->IsString()) {
		String::Utf8Value itemVal(value->ToString());
		std::string itemValue = std::string(*itemVal);
		nsfItemSetText(note_handle,itemName.c_str(),itemValue.c_str());

	} else if (value->IsArray()) {
		Local<Array> arrVal = Local<Array>::Cast(value);
			unsigned int num_locations = arrVal->Length();
			if (num_locations > 0) {				
				for (unsigned int i = 0; i < num_locations; i++) {
					Local<Object> obj = Local<Object>::Cast(arrVal->Get(i));
					String::Utf8Value value(obj->ToString());
					std::string s_val = std::string(*value);
					if (i==0) {
						nsfItemCreateTextList(note_handle,itemName.c_str(),s_val.c_str());
					} else {
						nsfItemAppendTextList(note_handle,itemName.c_str(),s_val.c_str());
					}
				}
			}

	} else if (value->IsNumber()) {
		double itemValue  = value->NumberValue();
		nsfItemSetNumber(note_handle,itemName.c_str(), &itemValue);

	} else if (value->IsDate()) {
		double unix_time = v8::Date::Cast(*value)->NumberValue();
		nsfSetItemDate(note_handle,itemName.c_str(),unix_time);
	}

}


void nsfItemSetNumber(unsigned short nHandle, const char * itemName, const double * itemValue) {
	NOTEHANDLE   note_handle;
	NUMBER       number_field;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	note_handle = (NOTEHANDLE)nHandle;  
  
  if (error = NSFItemSetNumber(note_handle, itemName, itemValue)) {
	  DataHelper::GetAPIError(error, error_text);
		printf("Error setItemNumber%s\n",error_text);
	}
}

void nsfItemCreateTextList(unsigned short nHandle, const char * itemName, const char * itemValue) {
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	note_handle = (NOTEHANDLE)nHandle;
	OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, itemValue, MAXWORD, buf, MAXWORD);

	if (error = NSFItemCreateTextList(note_handle, itemName,buf,strlen(itemValue))) {
		DataHelper::GetAPIError(error, error_text);
		printf("Error nsfItemCreateTextList%s\n",error_text);
	}
	
}
void nsfItemAppendTextList(unsigned short nHandle, const char * itemName, const char * itemValue) {
	NOTEHANDLE   note_handle;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];
	note_handle = (NOTEHANDLE)nHandle;
	OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, itemValue, MAXWORD, buf, MAXWORD);
	if (error = NSFItemAppendTextList(note_handle, itemName,buf,strlen(itemValue),TRUE)) {
		DataHelper::GetAPIError(error, error_text);
		printf("Error nsfItemCreateTextList%s\n",error_text);
	}
}

NAN_METHOD(getItemNumber) {
	double note_handle = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);		
	
		double numberValue = nsfItemGetNumber(note_handle, itemName.c_str());
		Local<Number> retval = Nan::New(numberValue);
  	info.GetReturnValue().Set(retval);
}

NAN_METHOD(getItemDate) {
	double value = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemStr (*val);
	NUMBER       number_field;
	double dateTimeValue = 0.0;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
	char buf[MAXWORD];

	unsigned short n_h = (unsigned short) value;
	dateTimeValue = nsfGetItemDate(n_h,itemStr.c_str());
	Local<v8::Date> retval = New<v8::Date>(dateTimeValue).ToLocalChecked();
  info.GetReturnValue().Set(retval); 
}

double nsfGetItemDate(unsigned short n_h, const char * itemName) {
	
	char         field_text[MAXWORD];
	WORD         field_len;
	BOOL         field_found;
	NOTEHANDLE   note_handle;
	double dateTimeValue = 0.0;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);

	note_handle = (NOTEHANDLE)n_h;

	field_found = NSFItemIsPresent ( 
                note_handle,
                itemName,
                (WORD) strlen (itemName));


    if (field_found) {
			TIMEDATE time_date;
			if (NSFItemGetTime(note_handle,
				itemName,
				&time_date)) {
				TIME tid;
				struct tm * timeinfo;

				tid.GM = time_date;
				tid.zone = 0;
				tid.dst = 0;
				TimeGMToLocalZone(&tid);			

				time_t rawtime = time(0);
				timeinfo = localtime(&rawtime);			
				timeinfo->tm_year = tid.year - 1900;
				timeinfo->tm_mon = tid.month-1;
				timeinfo->tm_mday = tid.day;

				timeinfo->tm_hour = tid.hour;
				timeinfo->tm_min = tid.minute-1;
				timeinfo->tm_sec = tid.second;
			
				double dtime = static_cast<double>(mktime(timeinfo))+1;
			
				dateTimeValue = dtime * 1000; // convert to double time
				}
		}	
		return dateTimeValue;
}