/*******************************************************************************
* Domino addon for Node.js
*
* Copyright (c) 2016 Nils T. Hjelme
*
* The MIT License (MIT)
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************/

#include <nan.h>
#include "document_async.h"
#include "DocumentItem.h"
#include "DataHelper.h"
#include <nsfdb.h>
#include <nif.h>
#include <nsfnote.h>
#include <osmisc.h>
#include <textlist.h>
#include <osmem.h>
#include <time.h>
#include <iostream>
#include <map>
#include <iterator>
#include <vector>

using v8::Function;
using v8::Local;
using v8::Number;
using v8::Value;
using v8::String;
using v8::Object;
using v8::Array;
using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::Callback;
using Nan::HandleScope;
using Nan::New;
using Nan::Null;
using Nan::To;

NOTEHANDLE   note_handle;
std::vector<DocumentItem *> items;

template< typename T >
struct delete_pointer_element
{
	void operator()(T element) const
	{
		delete element;
	}
};

class DocumentWorker : public AsyncWorker {
public:
	DocumentWorker(Callback *callback, std::string serverName, std::string dbName, std::string unid)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), unid(unid){
	}
	~DocumentWorker() {
		// delete name pointer
		for (size_t i = 0; i < items.size(); i++) {
			if (items[i]->name) {
				delete items[i]->name;
			}			
		}
	}
	
	
	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {			
		DBHANDLE    db_handle;      /* handle of source database */
		UNID temp_unid;
		STATUS   error = NOERROR;           /* return status from API calls */
		char *error_text =  (char *) malloc(sizeof(char) * 200);   

		if (error = NotesInitThread())
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}
		
		if (error = NSFDbOpen(dbName.c_str(), &db_handle))
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NotesTermThread();
		}
		
		char unid_buffer[33];
		strncpy(unid_buffer, unid.c_str(), 32);
		
		unid_buffer[32] = '\0';
	
		if (unid.length() == 32)
		{	
			/* Note part second, reading backwards in buffer */
			temp_unid.Note.Innards[0] = (DWORD)strtoul(unid_buffer + 24, NULL, 16);
			unid_buffer[24] = '\0';
			temp_unid.Note.Innards[1] = (DWORD)strtoul(unid_buffer + 16, NULL, 16);
			unid_buffer[16] = '\0';

			/* DB part first */
			temp_unid.File.Innards[0] = (DWORD)strtoul(unid_buffer + 8, NULL, 16);
			unid_buffer[8] = '\0';
			temp_unid.File.Innards[1] = (DWORD)strtoul(unid_buffer, NULL, 16);
		}

		if (error = NSFNoteOpenByUNID(
			db_handle,  /* database handle */
			&temp_unid, /* note ID */
			(WORD)0,                      /* open flags */
			&note_handle))          /* note handle (return) */
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NSFDbClose(db_handle);
		}


		if (error = NSFItemScan(
			note_handle,	/* note handle */
			field_actions,	/* action routine for fields */
			&note_handle))	/* argument to action routine */

		{					
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NSFNoteClose(note_handle);
			NSFDbClose(db_handle);
		}

		if (error = NSFDbClose(db_handle))
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}
		
		NotesTermThread();
		
	}


	
	// Executed when the async work is complete
	// this function will be run inside the main event loop
	// so it is safe to use V8 again
	void HandleOKCallback() {
		HandleScope scope;		
		Local<Object> resDoc = Nan::New<Object>();
		Nan::Set(resDoc, New<v8::String>("@unid").ToLocalChecked(), New<v8::String>(unid).ToLocalChecked());
		for (std::size_t i = 0; i < items.size(); i++) {
			DocumentItem *di = items[i];
			if (di->type == 1) {				
				Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), New<v8::String>(di->stringValue).ToLocalChecked());
			}
			else if (di->type == 2) {
				Nan::Set(resDoc, New<String>(di->name).ToLocalChecked(), New<Number>(di->numberValue));
			}
			else if (di->type == 3) {
				Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), New<v8::Date>(di->numberValue).ToLocalChecked());
			}
			else if (di->type == 4) {				
				Local<Array> arr = New<Array>();
				for (size_t j = 0; j < di->vectorStrValue.size(); j++) {
					if (di->vectorStrValue[j]) {						
						Nan::Set(arr, j, Nan::New<String>(di->vectorStrValue[j]).ToLocalChecked());
					}					
				}
				Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), arr);
			}			
		}		
		Local<Value> argv[] = {
			Null()
			, resDoc
		};
		
		callback->Call(2, argv);
		
	}

	void HandleErrorCallback() {
		HandleScope scope;
		Local<Object> errorObj = Nan::New<Object>();
		Nan::Set(errorObj, New<v8::String>("errorMessage").ToLocalChecked(), New<v8::String>(this->ErrorMessage()).ToLocalChecked());
		Local<Value> argv[] = {
			errorObj,
			Null()
		};
		callback->Call(2, argv);
	}

private:	
	std::string serverName;
	std::string dbName;
	std::string unid;	
};



STATUS LNCALLBACK field_actions(WORD unused, WORD item_flags, char far *name_ptr, WORD name_len, void far *item_valu, DWORD item_value_len, void far *note_handle2) {
	WORD         field_len;
	char         field_text[132000];
	STATUS		error = NOERROR;
	NUMBER       number_field;

	BLOCKID                bidLinksItem;
	DWORD                  dwLinksValueLen;
	BLOCKID                bidLinksValue;
	WORD                   item_type;
	DocumentItem * di = new DocumentItem();


	char *field_name = (char*)malloc(name_len + 1);
	if (field_name) {		
		memcpy(field_name, name_ptr, name_len);
		field_name[name_len] = '\0';
	}
	
	if (error = NSFItemInfo(note_handle, field_name,
		strlen(field_name), &bidLinksItem,
		&item_type, &bidLinksValue,
		&dwLinksValueLen)) {
		
		return (error);
	}
	if (item_type == TYPE_TEXT) {

		field_len = NSFItemGetText(
			note_handle,
			field_name,
			field_text,
			(WORD) sizeof(field_text));

		char buf[MAXWORD];
		OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, buf, MAXWORD);
		di->stringValue = std::string(buf);
		di->name = (char*)malloc(name_len + 1);
		if (di->name) {
			memcpy(di->name, name_ptr, name_len);
			di->name[name_len] = '\0';
		}		
		di->type = 1;
		items.push_back(di);
		
	}
	else if (item_type == TYPE_TEXT_LIST) {
		
		WORD num_entries = NSFItemGetTextListEntries(note_handle,
			field_name);
		std::vector<char*> vectorStrValue = std::vector<char*>(0);
		for (int counter = 0; counter < num_entries; counter++)
		{
			field_len = NSFItemGetTextListEntry(note_handle,
				field_name, counter, field_text, (WORD)(sizeof(field_text) - 1));
			//text_buf2[text_len] = '\0';
			// buf[MAXWORD];
			char *buf = (char*)malloc(field_len + 1);
			OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, buf, MAXWORD);
			vectorStrValue.push_back(buf);
			
		}
		di->vectorStrValue = vectorStrValue;
		di->name = field_name;
		di->type = 4;
		items.push_back(di);
	}
	else if (item_type == TYPE_NUMBER) {
		NSFItemGetNumber(
			note_handle,
			field_name,
			&number_field);
		di->numberValue = (double)number_field;
		di->name = field_name;
		di->type = 2;
		items.push_back(di);
	}
	
	else if (item_type == TYPE_TIME) {
		TIMEDATE time_date;
		if (NSFItemGetTime(note_handle,
			field_name,
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
			timeinfo->tm_sec = tid.second-1;
			
			double dtime = static_cast<double>(mktime(timeinfo));
			
			double dateTimeValue = dtime * 1000; // convert to double time
			di->numberValue = dateTimeValue;
			di->name = field_name;
			di->type = 3;
			items.push_back(di);
		}


	}

	//delete field_name;

	return(NOERROR);

}

NAN_METHOD(GetDocumentAsync) {
	v8::Isolate* isolate = info.GetIsolate();

	Local<Object> param = (info[0]->ToObject());
	Local<String> unidParam = (info[1]->ToString());
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	String::Utf8Value unid(unidParam->ToString());

	std::string serverStr;
	std::string dbStr;
	std::string unidStr;
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	unidStr = std::string(*unid);		
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new DocumentWorker(callback, serverStr,dbStr,unidStr));
}
