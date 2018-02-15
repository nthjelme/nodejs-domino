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
#include "save_document_async.h"  
#include "view_async.h"
#include "replicate_database_async.h"
#include "makeresponse_document.h"
#include "delete_document_async.h"
#include "create_database.h"
#include "delete_database.h"
#include "nsf_search.h"
#include <iostream>
#include "global.h"
#include "nsfdb.h"
#include "nsfdata.h"
#include "osmisc.h"
#include "osfile.h"
#include "htmlapi.h"
#include "notes_document.h"
#include "notes_database.h"

using v8::FunctionTemplate;
using v8::Handle;
using v8::Object;
using v8::String;
using node::AtExit;
using Nan::GetFunction;
using Nan::New;
using Nan::Set;

//LNNotesSession session; // session as a global, should maby be in a Persist object?

void InitDominoSession(const Nan::FunctionCallbackInfo<v8::Value>& info) { 
	char *argv[] = { "./addon" };
	STATUS error;
	if (error = NotesInitExtended(1, argv))	{
		printf("\n Unable to initialize Notes.\n");

	}	
}

void TermDominoSession(const Nan::FunctionCallbackInfo<v8::Value>& info) {	
	NotesTerm();
}

void sinitThread(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	STATUS					error = NOERROR;     /* return status from API calls */
	char *error_text = (char *) malloc(sizeof(char) * 200);   
		if (error = NotesInitThread())
		{
			DataHelper::GetAPIError(error,error_text);
			printf("error: %s", error_text);
		}
}

void stermThread(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	NotesTermThread();
}


static void termAtExitDominoSession(void*) {	
	

}
NAN_MODULE_INIT(InitAll) {	
	Set(target, New<String>("getDocumentAsync").ToLocalChecked(),
	  GetFunction(New<FunctionTemplate>(GetDocumentAsync)).ToLocalChecked());
	
	Set(target, New<String>("saveDocumentAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(SaveDocumentAsync)).ToLocalChecked());
	
	
	Set(target, New<String>("deleteDocumentAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(DeleteDocumentAsync)).ToLocalChecked());
	
	Set(target, New<String>("makeResponseDocumentAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(MakeResponseDocumentAsync)).ToLocalChecked());

	Set(target, New<String>("searchNsfAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(SearchNsfAsync)).ToLocalChecked());

	Set(target, New<String>("sinitThread").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(sinitThread)).ToLocalChecked());

	Set(target, New<String>("stermThread").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(stermThread)).ToLocalChecked());
		
	/**
	 * Database methods
	 * 
	 * */
	Set(target, New<String>("openDatabase").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(openDatabase)).ToLocalChecked());
		
	Set(target, New<String>("closeDatabase").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(closeDatabase)).ToLocalChecked());

	Set(target, New<String>("getDatabaseName").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getDatabaseName)).ToLocalChecked());
		
	Set(target, New<String>("replicationSummary").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(replicationSummary)).ToLocalChecked());
				
	Set(target, New<String>("getNotesNote").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getNotesNote)).ToLocalChecked());

	Set(target, New<String>("createNotesNote").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(createNotesNote)).ToLocalChecked());

	
	/**
	 * Document methods
	 * */
	Set(target, New<String>("closeNote").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(closeNote)).ToLocalChecked());

	Set(target, New<String>("updateNote").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(updateNote)).ToLocalChecked());

	Set(target, New<String>("getNoteUNID").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getNoteUNID)).ToLocalChecked());

	Set(target, New<String>("getItemText").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getItemText)).ToLocalChecked());

	Set(target, New<String>("hasItem").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(hasItem)).ToLocalChecked());

	Set(target, New<String>("setItemText").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(setItemText)).ToLocalChecked());
	
	Set(target, New<String>("setAuthor").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(setAuthor)).ToLocalChecked());
		
	Set(target, New<String>("setItemValue").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(setItemValue)).ToLocalChecked());
		
	Set(target, New<String>("getItemNumber").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getItemNumber)).ToLocalChecked());

	Set(target, New<String>("setItemNumber").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(setItemNumber)).ToLocalChecked());
						
	Set(target, New<String>("getItemDate").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getItemDate)).ToLocalChecked());

	Set(target, New<String>("setItemDate").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(setItemDate)).ToLocalChecked());	

	Set(target, New<String>("getItemValue").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getItemValue)).ToLocalChecked());

	Set(target, New<String>("appendItemTextList").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(appendItemTextList)).ToLocalChecked());

	Set(target, New<String>("getMimeItem").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(getMimeItem)).ToLocalChecked());
	
	Set(target, New<String>("deleteItem").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(deleteItem)).ToLocalChecked());
		
	/*
	Set(target, New<String>("getResponseDocumentsAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(GetResponseDocumentsAsync)).ToLocalChecked());
		
	Set(target, New<String>("replicateDatabaseAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(ReplicateDatabaseAsync)).ToLocalChecked());
	*/	
	Set(target, New<String>("createDatabase").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(createDatabase)).ToLocalChecked());

		Set(target, New<String>("deleteDatabase").ToLocalChecked(),
			GetFunction(New<FunctionTemplate>(deleteDatabase)).ToLocalChecked());

	Set(target, New<String>("getViewAsync").ToLocalChecked(),
	  GetFunction(New<FunctionTemplate>(GetViewAsync)).ToLocalChecked());
	
	Set(target, Nan::New("initSession").ToLocalChecked(),
	  Nan::New<v8::FunctionTemplate>(InitDominoSession)->GetFunction());
	
	Set(target, Nan::New("termSession").ToLocalChecked(),
		Nan::New<v8::FunctionTemplate>(TermDominoSession)->GetFunction());
	
	AtExit(termAtExitDominoSession);
	
}

NODE_MODULE(addon, InitAll)
