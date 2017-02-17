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
#include "DocumentItem.h"
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
	SaveDocumentWorker(Callback *callback, std::string serverName, std::string dbName, std::string docUnid, std::vector<DocumentItem *> items)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), unid(docUnid), items(items) {}
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
		bool isEdit = false;
		NOTEHANDLE   note_handle2;
		char *error_text = (char *)malloc(sizeof(char) * 200);

		if (error = NotesInitThread())
		{
			//DataHelper::GetAPIError(error,error_text);
			SetErrorMessage("error init notes thread");
		}

		if (error = NSFDbOpen(dbName.c_str(), &db_handle))
		{
			printf("error opening database\n");
			//DataHelper::GetAPIError(error,error_text);
			SetErrorMessage("error opening database");
		}
		if (unid.length() == 32) {
			// edit document
			isEdit = true;
			const char * chrUNID = unid.c_str();
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
				DataHelper::GetAPIError(error, error_text);
				SetErrorMessage(error_text);
			}
		}
		else {
			//create new document		
			if (error = NSFNoteCreate(db_handle, &note_handle2)) {
				printf("error creating document\n");
				NSFDbClose(db_handle);
				NotesTermThread();
				//DataHelper::GetAPIError(error,error_text);
				//SetErrorMessage(error_text);
			}


		}


		for (std::size_t ii = 0; ii < items.size(); ii++) {

			if (items[ii]->type == 2) {
				if (error = NSFItemSetNumber(note_handle2, items[ii]->name, &items[ii]->numberValue))
				{
					DataHelper::GetAPIError(error, error_text);
					SetErrorMessage(error_text);
				}
			}
			else if (items[ii]->type == 1) {
				// text			
				char buf[MAXWORD];
				OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, items[ii]->stringValue.c_str(), MAXWORD, buf, MAXWORD);

				if (error = NSFItemSetText(note_handle2,
					items[ii]->name,
					buf,
					MAXWORD))
				{
					DataHelper::GetAPIError(error, error_text);
					SetErrorMessage(error_text);
				}

			}
			else if (items[ii]->type == 4) {
				// text list



				for (size_t j = 0; j < items[ii]->vectorStrValue.size(); j++) {
					size_t val_len = strlen(items[ii]->vectorStrValue[j]);
					char *buf = (char*)malloc(val_len + 1);
					OSTranslate(OS_TRANSLATE_UTF8_TO_LMBCS, items[ii]->vectorStrValue[j], val_len, buf, val_len);
					if (j == 0) {

						if (error = NSFItemCreateTextList(note_handle2,
							items[ii]->name,
							buf,
							val_len))
						{
							DataHelper::GetAPIError(error, error_text);
							SetErrorMessage(error_text);
						}
					}
					else {
						if (error = NSFItemAppendTextList(note_handle2,
							items[ii]->name,
							buf,
							val_len,
							TRUE))
						{
							DataHelper::GetAPIError(error, error_text);
							SetErrorMessage(error_text);
						}
					}
				}


			}
			else if (items[ii]->type == 3) {
				// datetime
				TIME tid;

				std::time_t t = static_cast<time_t>(items[ii]->numberValue / 1000);
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
					DataHelper::GetAPIError(error, error_text);
					SetErrorMessage(error_text);
				}

				if (error = NSFItemSetTime(note_handle2, items[ii]->name, &tid.GM))
				{
					DataHelper::GetAPIError(error, error_text);
					SetErrorMessage(error_text);
				}
			}

		}

		if (error = NSFNoteUpdate(note_handle2, 0)) {
			//DataHelper::GetAPIError(error,error_text);
			SetErrorMessage("error updating note");
			NSFDbClose(db_handle);
			NotesTermThread();
			return;
		}

		NSFNoteGetInfo(note_handle2, _NOTE_OID, &tempOID);
		char unid_buffer[33];
		snprintf(unid_buffer, 33, "%08X%08X%08X%08X", tempOID.File.Innards[1], tempOID.File.Innards[0], tempOID.Note.Innards[1], tempOID.Note.Innards[0]);
		DocumentItem *di = new DocumentItem();
		di->name = (char*)malloc(6);
		if (di->name) {
			strcpy(di->name, "@unid");
		}
		di->type = 1;
		di->stringValue = std::string(unid_buffer);
		items.push_back(di);

		if (error = NSFNoteClose(note_handle2))
		{
			//DataHelper::GetAPIError(error,error_text);
			SetErrorMessage("error closing notes");
			NSFDbClose(db_handle);
			NotesTermThread();
			return;
		}
		if (error = NSFDbClose(db_handle))
		{
			//DataHelper::GetAPIError(error,error_text);
			SetErrorMessage("error closing db");
			NotesTermThread();
			return;
		}
		NotesTermThread();


	}

	void HandleOKCallback() {
		HandleScope scope;
		Local<Object> resDoc = Nan::New<Object>();
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
	std::vector<DocumentItem *> items;
};

NAN_METHOD(SaveDocumentAsync) {
	v8::Isolate* isolate = info.GetIsolate();
	Local<Object> param = (info[0]->ToObject());
	Local<Object> document = (info[1]->ToObject());
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);
	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	std::string serverStr;
	std::string dbStr;
	std::string docUnid;
	if (document->Has(String::NewFromUtf8(isolate, "@unid"))) {
		Local<Value> unidVal = document->Get(String::NewFromUtf8(isolate, "@unid"));
		String::Utf8Value unid(unidVal->ToString());
		docUnid = std::string(*unid);
		document->Delete(String::NewFromUtf8(isolate, "@unid"));
	}
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);


	std::vector<DocumentItem*> items = unpack_document(document);

	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new SaveDocumentWorker(callback, serverStr, dbStr, docUnid, items));
}


