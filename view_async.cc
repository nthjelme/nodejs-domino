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
#include "ItemValue.h"
#include <lncppapi.h>
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <osmisc.h>

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




class ViewWorker : public AsyncWorker {
public:
	ViewWorker(Callback *callback, std::string serverName, std::string dbName, std::string viewName,std::string category)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), viewName(viewName),category(category), viewResult() {}
	~ViewWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		LNNotesSession session;
		LNSetThrowAllErrors(TRUE);

		try {			
			session.InitThread();
			LNViewFolder view;
			LNVFEntry mainentry;
			LNDatabase Db;
			
			session.GetDatabase(dbName.c_str(), &Db, serverName.c_str());
			Db.Open();
			
			Db.GetViewFolder(viewName.c_str(), &view);
			view.Open();
			
			LNINT columnCount = view.GetColumnCount();
		
			LNINT i = view.GetEntryCount();;
			

			if (!category.empty()) {				
					LNVFPosition pos;

					LNINT vCount = 0;
					LNVFNavigator nav;
					view.Find(category.c_str(), &mainentry);
					i = mainentry.GetChildCount();

					mainentry.GetPosition(&pos);
					view.SetPosition(pos);

			}
			else {
				view.GotoFirst(&mainentry);
				
			}
			
			int count = 0;
			do {				
				LNDocument      Doc;
				LNINT           IndentLevels = 0;										
				LNINT j;				
				std::map <std::string, ItemValue> doc;


				for (j = 0; j < columnCount; j++) {
					LNItem item = mainentry[j];

					LNVFColumn column;
					view.GetColumn(j, &column);
					LNString internalName = column.GetInternalName();
					std::string itemName = internalName;

					if (item.IsNull()) {
						doc.insert(std::make_pair(itemName, ItemValue("")));
					} else {
						if (mainentry.IsCategory()) {

						}
						else {
							mainentry.GetDocument(&Doc);
							const UNIVERSALNOTEID * ln_unid = Doc.GetUniversalID();
							LNUniversalID lnUnid = LNUniversalID(ln_unid);
							LNString unidStr = lnUnid.GetText();
							std::string unidStdStr = unidStr;
							doc.insert(std::make_pair("@unid", ItemValue(unidStdStr)));
						}
						doc.insert(std::make_pair(itemName, ItemValue(&item)));						
					}
				}
				viewResult.push_back(doc);
				count++;
			} while (view.GotoNext(&mainentry) == LNNOERROR && count < i);

			view.Close();
			Db.Close();

			
			session.TermThread();
		}
		catch (LNSTATUS Lnerror) {
			char ErrorBuf[512];
			LNGetErrorMessage(Lnerror, ErrorBuf, 512);
			if (session.IsInitialized()) {
				session.TermThread();
			}
			SetErrorMessage(ErrorBuf);
		}
	}

	// Executed when the async work is complete
	// this function will be run inside the main event loop
	// so it is safe to use V8 again
	void HandleOKCallback() {
		HandleScope scope;
		Local<Array> viewRes = New<Array>(viewResult.size());
		try {
			int j;
			for (j = 0; j < viewResult.size(); j++) {
				std::map<std::string, ItemValue> doc = viewResult[j];
				Local<Object> resDoc = Nan::New<Object>();
				std::map<std::string, ItemValue>::iterator it;
				for (it = doc.begin(); it != doc.end(); it++)
				{
					ItemValue value = it->second;
					if (value.type == 0) {
						Nan::Set(resDoc, New<String>(it->first).ToLocalChecked(), New<Number>(value.numberValue));
					}
					else if (value.type == 1) {
						Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), New<v8::String>(value.stringValue).ToLocalChecked());

					}
					else if (value.type == 2) {
						int ii;
						Local<Array> arr = New<Array>(value.vectorStrValue.size());
						Nan::Set(arr, 0, Nan::New<String>("").ToLocalChecked());
						for (ii = 0; ii < value.vectorStrValue.size(); ii++) {
							Nan::Set(arr, ii, Nan::New<String>(value.vectorStrValue[ii]).ToLocalChecked());
						}
						Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), arr);

					}
				}
				Nan::Set(viewRes, j, resDoc);
			}
		}
		catch (LNSTATUS Lnerror) {
			char ErrorBuf[512];
			LNGetErrorMessage(Lnerror, ErrorBuf, 512);
			std::cout << "Handle Error:  " << ErrorBuf << std::endl;
		}
		
		Local<Value> argv[] = {
			Null()
			, viewRes
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
	std::string viewName;
	std::string category;
	std::vector <std::map<std::string, ItemValue>> viewResult;

};


NAN_METHOD(GetViewAsync) {	
	v8::Isolate* isolate = info.GetIsolate();	
	Local<Object> param = (info[0]->ToObject());
	Local<Object> viewParam = (info[1]->ToObject());
	Local<Value> viewKey = String::NewFromUtf8(isolate, "view");
	Local<Value> catKey = String::NewFromUtf8(isolate, "category");
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	Local<Value> viewVal = viewParam->Get(viewKey);
	Local<Value> catVal = viewParam->Get(catKey);
	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	String::Utf8Value view(viewVal->ToString());
	String::Utf8Value cat(catVal->ToString());
	std::string serverStr;
	std::string dbStr;
	std::string viewStr;
	std::string catStr;
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	viewStr = std::string(*view);
	catStr = std::string(*cat);
	
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new ViewWorker(callback, serverStr, dbStr, viewStr,catStr));
}
