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
#include "makeresponse_document.h"
#include "DocumentItem.h"
#include "DataHelper.h"
#include <nsfdb.h>
#include <nif.h>
#include <nsfnote.h>
#include <osmisc.h>
#include <iostream>

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


class MakeResponseDocumentWorker : public AsyncWorker {
public:
	MakeResponseDocumentWorker(Callback *callback, std::string serverName, std::string dbName, std::string unid, std::string responseUnid)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), unid(unid), responseUnid(responseUnid) {}
	~MakeResponseDocumentWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		DBHANDLE    db_handle;      /* handle of source database */
		STATUS   error = NOERROR;           /* return status from API calls */
		char *error_text = (char *)malloc(sizeof(char) * 200);
		LIST       ListHdr;
		UNID       NoteUNID;
		UNID	   ResponseUNID;		
		NOTEHANDLE note_handle;
		
		/*  block of memory to hold the response reference list */
		char       *buf;

		buf = (char *)malloc(sizeof(LIST) + sizeof(UNID));

		if (buf == NULL)
		{
			SetErrorMessage("malloc failed");
			return;
		}

		if (error = NotesInitThread())
		{			
			SetErrorMessage("error init notes thread");
		}

		// create response unid
		if (unid.length() != 32) {
			SetErrorMessage("Not valid parent document unid");
			return;
		}
		// create response unid
		if (responseUnid.length() != 32) {
			SetErrorMessage("Not valid response unid");
			return;
		}

	
		DataHelper::ToUNID(unid.c_str(), &NoteUNID);
		DataHelper::ToUNID(responseUnid.c_str(), &ResponseUNID);
	
		/* Initialize the LIST header part of the response reference list */
		ListHdr.ListEntries = (USHORT)1;

		/* Pack the LIST and the UNID members of the Noteref list into
			a memory block.
		*/
		memcpy(buf, (char*)&ListHdr, sizeof(LIST));
		memcpy((buf + sizeof(LIST)), (char*)&NoteUNID, sizeof(UNID));

		if (error = NSFDbOpen(dbName.c_str(), &db_handle))
		{				
			SetErrorMessage("error opening database");
		}

		if (error = NSFNoteOpenByUNID(
			db_handle,  /* database handle */
			&ResponseUNID, /* note ID */
			(WORD)0,                      /* open flags */
			&note_handle))          /* note handle (return) */
		{
			DataHelper::GetAPIError(error, error_text);
			SetErrorMessage(error_text);
			NSFDbClose(db_handle);
		}
	

		if (error = NSFItemAppend(note_handle,
			ITEM_SUMMARY,
			FIELD_LINK,         /* $REF */
			(WORD)strlen(FIELD_LINK),
			TYPE_NOTEREF_LIST,  /* data type */
			buf,                /* populated RESPONSE */
			(DWORD)(sizeof(LIST) + sizeof(UNID))))
		{
			NSFNoteClose(note_handle);
			SetErrorMessage("Failed to create response document");
			return;
		}

		free(buf);

		/* Update the note  */
		if (error = NSFNoteUpdate(note_handle, 0))
		{
			NSFNoteClose(note_handle);
			DataHelper::GetAPIError(error, error_text);
			SetErrorMessage(error_text);
			return;
		}

		error = NSFNoteClose(note_handle);

		NSFDbClose(db_handle);
		
		NotesTermThread();	
	}

	void HandleOKCallback() {
		HandleScope scope;
		Local<Object> resDoc = Nan::New<Object>();
		Nan::Set(resDoc, New<v8::String>("status").ToLocalChecked(), New<v8::String>("made response").ToLocalChecked());
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
	std::string responseUnid;
};

NAN_METHOD(MakeResponseDocumentAsync) {
	v8::Isolate* isolate = info.GetIsolate();

	Local<Object> param = (info[0]->ToObject());
	Local<String> unidParam = (info[1]->ToString());
	Local<String> parentUnidParam = (info[2]->ToString());
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	String::Utf8Value unid(unidParam->ToString());
	String::Utf8Value parentUnid(parentUnidParam->ToString());

	std::string serverStr;
	std::string dbStr;
	std::string unidStr;
	std::string parentUnidStr;
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	unidStr = std::string(*unid);
	parentUnidStr = std::string(*parentUnid);
	Callback *callback = new Callback(info[3].As<Function>());

	AsyncQueueWorker(new MakeResponseDocumentWorker(callback, serverStr, dbStr, unidStr,parentUnidStr));
}