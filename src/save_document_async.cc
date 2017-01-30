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
#include "save_document_async.h"
#include "ItemValue.h"
#include "DataHelper.h"
#include <lncppapi.h>
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <osmisc.h>
#include <ctime>
#include <stdio.h>

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


class SaveDocumentWorker : public AsyncWorker {
public:
	SaveDocumentWorker(Callback *callback, std::string serverName, std::string dbName, std::map <std::string, ItemValue> doc)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), doc(doc) {}
	~SaveDocumentWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {

		DBHANDLE    db_handle;      /* handle of source database */
		UNID temp_unid;
		OID tempOID;
		STATUS   error = NOERROR;           /* return status from API calls */
		BOOLEAN isEdit = false;
		NOTEHANDLE   note_handle2;
		char *error_text =  (char *)malloc(sizeof(char) * 200);   

		if (error = NotesInitThread())
		{			
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}

		if (error = NSFDbOpen(dbName.c_str(), &db_handle))
		{		
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}

		std::map<std::string, ItemValue>::iterator iter;
		iter = doc.find("@unid");
		if (iter != doc.end()) {
			// edit document
			
			isEdit = true;
			ItemValue unidItem = iter->second;
			const char * chrUNID = unidItem.stringValue.c_str();
			char unid_buffer[33];
			strncpy(unid_buffer, chrUNID, 32);
			unid_buffer[32] = '\0';			
			if (strlen(unid_buffer) == 32)
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
				&note_handle2))          /* note handle (return) */
			{			
				printf("error opening document\n");
				DataHelper::GetAPIError(error,error_text);
				SetErrorMessage(error_text);
			}
		}
		else {
			//create new document			
			if (error = NSFNoteCreate(db_handle, &note_handle2)) {				
				DataHelper::GetAPIError(error,error_text);
				SetErrorMessage(error_text);
			}

		}

		std::map<std::string, ItemValue>::iterator it;
		for (it = doc.begin(); it != doc.end(); it++)
		{
			ItemValue value = it->second;
			if (value.type == 0) {
				if (error = NSFItemSetNumber(note_handle2, it->first.c_str(), &value.numberValue))
				{
					DataHelper::GetAPIError(error,error_text);
					SetErrorMessage(error_text);
				}
			}
			else if (value.type == 1) {
				// text			
				
				char buf[MAXWORD];
				OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, value.stringValue.c_str(), MAXWORD, buf, MAXWORD);
				if (error = NSFItemSetText(note_handle2,
					it->first.c_str(),
					buf,
					MAXWORD))
				{					
					DataHelper::GetAPIError(error,error_text);
					SetErrorMessage(error_text);
				}
				
			}
			else if (value.type == 2) {
				// text list
				char buf[MAXWORD];
				
				size_t ii;
				for (ii = 0; ii < value.vectorStrValue.size(); ii++) {
					OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, value.vectorStrValue[ii].c_str(), MAXWORD, buf, MAXWORD);
					if (ii == 0) {						
						if (error = NSFItemCreateTextList(note_handle2,
							it->first.c_str(),
							buf,
							MAXWORD))
						{
							DataHelper::GetAPIError(error,error_text);
							SetErrorMessage(error_text);
						}
					}
					else {
						if (error = NSFItemAppendTextList(note_handle2,
							it->first.c_str(),
							buf,
							MAXWORD,
							TRUE))
						{
							DataHelper::GetAPIError(error,error_text);
							SetErrorMessage(error_text);
						}
					}
				}

				
			}
			else if (value.type == 3) {
				// datetime
				TIME tid;
				
				std::time_t t = static_cast<time_t>(value.dateTimeValue / 1000);
				struct tm* ltime = localtime(&t);				 
				tid.year = ltime->tm_year + 1900;
				tid.month = ltime->tm_mon + 1;
				tid.day = ltime->tm_mday;
				tid.hour = ltime->tm_hour;
				tid.minute = ltime->tm_min + 1;
				tid.second = ltime->tm_sec + 1;
				tid.zone = 0;
				tid.dst = 0;
				tid.hundredth = 0;
				
				if (error = TimeLocalToGM(&tid)) {
					DataHelper::GetAPIError(error,error_text);
					SetErrorMessage(error_text);
				}				

				if (error = NSFItemSetTime(note_handle2, it->first.c_str(), &tid.GM))
				{
					DataHelper::GetAPIError(error,error_text);
					SetErrorMessage(error_text);
				}
			}

		}
		
		if (error = NSFNoteUpdate(note_handle2, 0)) {
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}

		NSFNoteGetInfo(note_handle2, _NOTE_OID, &tempOID);
		char unid_buffer[33];
		snprintf(unid_buffer, 33, "%08X%08X%08X%08X", tempOID.File.Innards[1], tempOID.File.Innards[0], tempOID.Note.Innards[1], tempOID.Note.Innards[0]);
		
		doc.insert(std::make_pair("@unid", ItemValue(unid_buffer)));
		if (error = NSFNoteClose(note_handle2))
		{			
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}
		if (error = NSFDbClose(db_handle))
		{			
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
		}		
		NotesTermThread();

	}

	void HandleOKCallback() {		
		HandleScope scope;
		Local<Object> resDoc = DataHelper::getV8Data(&doc);			
		Local<Value> argv[] = {
			Null()
			, resDoc
		};		
		callback->Call(2, argv);
	}

	void HandleErrorCallback() {
		HandleScope scope;
		Local<Object> errorObj = Nan::New<Object>();
		Nan::Set(errorObj, New<v8::String>("errorMessage").ToLocalChecked(), New<v8::String>(ErrorMessage()).ToLocalChecked());
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
	std::map <std::string, ItemValue> doc;
};

NAN_METHOD(SaveDocumentAsync) {	
	v8::Isolate* isolate = info.GetIsolate();	
	Local<Object> param = (info[0]->ToObject());
	Local<Object> docParam = (info[1]->ToObject());	
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);
	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	std::string serverStr;
	std::string dbStr;
	
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	
	std::map <std::string, ItemValue> doc;
	
	// get document
	Local<Array> propNames = docParam->GetOwnPropertyNames();
	unsigned int num_prop = propNames->Length();
	for (unsigned int i = 0; i < num_prop; i++) {
		Local<Value> name = propNames->Get(i);
		std::string key;
		
		Local<String> keyStr = Local<String>::Cast(name);
		v8::String::Utf8Value param1(keyStr->ToString());
		key = std::string(*param1);
		Local<Value> val = docParam->Get(name);		
		
		if (val->IsString()) {
			String::Utf8Value value(val->ToString());
			std::string valueStr = std::string(*value);			
			doc.insert(std::make_pair(key, ItemValue(std::string(*value))));			
		} else if (val->IsArray()) {			
			Local<Array> arrVal = Local<Array>::Cast(val);
			unsigned int num_locations = arrVal->Length();
			if (num_locations>0) {
				std::vector<std::string> docVec;
				for (unsigned int i = 0; i < num_locations; i++) {
					Local<Object> obj = Local<Object>::Cast(arrVal->Get(i));
					String::Utf8Value value(obj->ToString());
					docVec.push_back(*value);
				}
				doc.insert(std::make_pair(key, ItemValue(docVec)));			
			}
		} else if (val->IsNumber()) {			
			Local<Number> numVal = val->ToNumber();			
			doc.insert(std::make_pair(key, ItemValue(numVal->NumberValue())));			
		} else if (val->IsDate()) {
			double ms = v8::Date::Cast(*val)->NumberValue();
			doc.insert(std::make_pair(key, ItemValue(ms,3)));
		}
		
	}	
	
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new SaveDocumentWorker(callback, serverStr, dbStr, doc));
}