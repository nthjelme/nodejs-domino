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
#include <osmisc.h>
#include <osmem.h>
#include <ctime>
#include <stdio.h>

#define BUFSIZE 185536

NAN_METHOD(closeNote) {
  //v8::Isolate* isolate = info.GetIsolate();
  NOTEHANDLE   note_handle;
	double value = info[0]->NumberValue();
	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;
	NSFNoteClose(note_handle);	
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
	}
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

	unsigned short n_h = (unsigned short) value;
	note_handle = (NOTEHANDLE)n_h;

	if (error = NSFItemInfo(note_handle, itemName.c_str(),
		strlen(itemName.c_str()), &bidLinksItem,
		&item_type, &bidLinksValue,
		&dwLinksValueLen)) {
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
	}
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
	tid.second = ltime->tm_sec-1;// + 1;
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

NAN_METHOD(getMimeItem) {
	NOTEHANDLE   note_handle;	
	BLOCKID fieldBlock;
	DWORD fieldLen;
	WORD fieldType;	
	MIME_PART *pMime; 
	DHANDLE        text_buffer;
  char         *text_ptr;
	DWORD        text_length;
	char         field_text[BUFSIZE+1];
	char achBuf[256];
	DWORD dwBufLen;
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);
	double n_h = info[0]->NumberValue();
	v8::String::Utf8Value val(info[1]->ToString());
	std::string itemName (*val);
	unsigned short nHandle = (unsigned short) n_h;
	note_handle = (NOTEHANDLE)nHandle;
	printf("get item: %s\n",itemName.c_str());

	if (error = MIMEConvertMIMEPartsCC(note_handle, FALSE, NULL)) {
			DataHelper::GetAPIError(error, error_text);
			printf("Error set convertmime : %s\n",error_text);
		}

	if (error = NSFItemInfo(note_handle, itemName.c_str(), (WORD)strlen(itemName.c_str()), NULL, &fieldType, &fieldBlock, &fieldLen)) {
		printf("error getting item info..");
		//return ERR(error); 
	}

	
	if (fieldType == TYPE_MIME_PART) {
	/*	pMime = (MIME_PART *)OSLockBlock(MIME_PART, fieldBlock); 
		char *pText; 
		pText = (char *)pMime + sizeof(MIME_PART); 
		DWORD textLen; 
		char fieldText[9999]; 
		textLen = pMime->wByteCount; 
		memcpy(fieldText, pText, textLen); 
		fieldText[textLen] = '\0'; 
		OSUnlockBlock(fieldBlock);*/
		info.GetReturnValue().Set(Nan::New<String>("mime part").ToLocalChecked()); 
	} else if (fieldType== TYPE_COMPOSITE) {
		info.GetReturnValue().Set(
  	Nan::New<String>("TYPE_COMPOSITE").ToLocalChecked()); 
	} else {
		info.GetReturnValue().Set(
  	Nan::New<String>("not a mime or composite field").ToLocalChecked()); 
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