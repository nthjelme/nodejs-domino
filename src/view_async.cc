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
#include "DataHelper.h"
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
	ViewWorker(Callback *callback, std::string serverName, std::string dbName, std::string viewName,std::string category,std::string find)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), viewName(viewName),category(category),find(find), viewResult() {}
	~ViewWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		LNNotesSession session;
		LNSetThrowAllErrors(TRUE);

		try {	
			std::cout << "category: " << category << " , find: " << find << std::endl;
			session.InitThread();
			LNViewFolder view;
			LNVFEntry mainentry;
			LNDatabase Db;
			LNVFNavigator nav;
			
			session.GetDatabase(dbName.c_str(), &Db, serverName.c_str());
			Db.Open();
			
			Db.GetViewFolder(viewName.c_str(), &view);
			view.Open();
			
			LNINT columnCount = view.GetColumnCount();
		
			LNINT i = view.GetEntryCount();;
			

			if (!category.empty()) {	
				std::cout << "run 0" << std::endl;
					LNVFPosition pos;

					LNINT vCount = 0;
					LNVFNavigator nav;
					view.Find(category.c_str(), &mainentry);
					i = mainentry.GetChildCount();

					mainentry.GetPosition(&pos);
					view.SetPosition(pos);

			}
			else if (!find.empty()) {
				std::cout << "run 0,5" << std::endl;
				view.Find(find.c_str(), &mainentry,0,&nav);
				std::cout << "found, " << nav.GetEntryCount() << std::endl;
			}

			
			else {
				std::cout << "run 0.8" << std::endl;
				view.GotoFirst(&mainentry);
				
			}

			std::cout << "run 1" << std::endl;
			
			int count = 0;
			do {				
				std::cout << "run 2" << std::endl;
				LNDocument      Doc;
				LNINT           IndentLevels = 0;										
				LNINT j;				
				std::map <std::string, ItemValue> doc;

				std::cout << "run 3" << std::endl;
				for (j = 0; j < columnCount; j++) {
					std::cout << "run 4" << std::endl;
					LNItem item = mainentry[j];
					std::cout << "run 4.1" << std::endl;
					LNVFColumn column;
					std::cout << "run 4.2" << std::endl;
					view.GetColumn(j, &column);
					std::cout << "run 5" << std::endl;
					LNString internalName =column.GetInternalName();
					std::string itemName = internalName;					
					if (item.IsNull()) {
						std::cout << "run 6" << std::endl;
						doc.insert(std::make_pair(itemName, ItemValue("")));
					} else {
						if (mainentry.IsCategory()) {							
							std::cout << "run 7" << std::endl;
						}
						else {
							std::cout << "run 8" << std::endl;
							mainentry.GetDocument(&Doc);
							const UNIVERSALNOTEID * ln_unid = Doc.GetUniversalID();
							LNUniversalID lnUnid = LNUniversalID(ln_unid);
							LNString unidStr = lnUnid.GetText();
							std::string unidStdStr = unidStr;
							std::cout << "run 9" << std::endl;
							doc.insert(std::make_pair("@unid", ItemValue(unidStdStr)));
						}
						std::cout << "run 10" << std::endl;
						doc.insert(std::make_pair(itemName, ItemValue(&item)));	
						std::cout << "run 11" << std::endl;
					}
				}
				viewResult.push_back(doc);
				std::cout << "run 12" << std::endl;
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
		Local<Array> viewRes = DataHelper::getV8Data(viewResult);
		
		
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
	std::string find;
	std::vector <std::map<std::string, ItemValue>> viewResult;

};


NAN_METHOD(GetViewAsync) {	
	v8::Isolate* isolate = info.GetIsolate();	
	Local<Object> param = (info[0]->ToObject());
	Local<Object> viewParam = (info[1]->ToObject());
	Local<Value> viewKey = String::NewFromUtf8(isolate, "view");
	Local<Value> catKey = String::NewFromUtf8(isolate, "category");
	Local<Value> findKey = String::NewFromUtf8(isolate, "find");
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	Local<Value> viewVal = viewParam->Get(viewKey);
	
	
	std::string catStr;
	std::string findStr;

	
	if (viewParam->Has(findKey)) {
		Local<Value> findVal = viewParam->Get(findKey);
		String::Utf8Value find(findVal->ToString());
		findStr = std::string(*find);
	}	
	if (viewParam->Has(catKey)) {
		Local<Value> catVal = viewParam->Get(catKey);
		String::Utf8Value cat(catVal->ToString());
		catStr = std::string(*cat);
	}

	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	String::Utf8Value view(viewVal->ToString());
	
	
	std::string serverStr;
	std::string dbStr;
	std::string viewStr;
	
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	viewStr = std::string(*view);
	
	
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new ViewWorker(callback, serverStr, dbStr, viewStr,catStr,findStr));
}
