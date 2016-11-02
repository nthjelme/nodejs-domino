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
#include "getresponse_documents.h"
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



class ResponseDocumentsWorker : public AsyncWorker {
public:
	ResponseDocumentsWorker(Callback *callback, std::string serverName, std::string dbName, std::string unid)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), unid(unid), responses() {}
	~ResponseDocumentsWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		LNSetThrowAllErrors(TRUE);
		LNNotesSession session;
		try {
			session.InitThread();
			LNString                DbTitle = "";
			
			LNDatabase Db;
			LNDocumentArray   documentResponses;
			LNINT responseCount;
			session.GetDatabase(dbName.c_str(), &Db, serverName.c_str());
			Db.Open();

			const LNString * lnstrUNID = new LNString(unid.c_str());

			//Get UNID *
			LNUniversalID * lnUNID = new LNUniversalID(*lnstrUNID);
			const UNIVERSALNOTEID * unidUNID = lnUNID->GetUniversalID();
			LNDocument			  Doc;
			Db.GetDocument(unidUNID, &Doc);
			Doc.Open();
			Doc.GetResponses(&documentResponses);

			responseCount = documentResponses.GetCount();
			for (int j = 0; j < responseCount; j++) {
				LNItemArray items;
				LNDocument resDoc;
				std::map <std::string, ItemValue> doc;

				resDoc = documentResponses[j];
				resDoc.Open();
				resDoc.GetItems(&items);

				const UNIVERSALNOTEID * ln_unid = resDoc.GetUniversalID();
				LNUniversalID lnUnid = LNUniversalID(ln_unid);
				LNString unidStr = lnUnid.GetText();
				std::string unidStdStr = unidStr;
				doc.insert(std::make_pair("@unid", ItemValue(unidStdStr)));

				
				for (LNINT i = 0; i < Doc.GetItemCount(); i++) {
					LNItem item = items[i];

					LNITEMTYPE type = item.GetType();
					LNString itemName = item.GetName();
					std::string iName = itemName;

					if (type == LNITEMTYPE_RICHTEXT) {

						LNRichText rt = (LNRichText)item;
						LNString rtText;
						LNRTCursor beginCursor;
						LNRTCursor endCursor;
						rt.GetCursor(&beginCursor);
						rt.GetEndCursor(&endCursor);
						std::string rtString;
						LNSTATUS status = LNWARN_TOO_MUCH_TEXT;
						while (status == LNWARN_TOO_MUCH_TEXT) {
							//TODO: LNStringtranlsate seems removes text from end, find another solution.
							status = rt.GetText(beginCursor, endCursor, &rtText, &beginCursor);
							LNINT bufLength = rtText.GetLength();
							rtString = rtText;
							char *buf = (char *)malloc(sizeof(char) *bufLength + 1);
							LNStringTranslate(rtText, LNCHARSET_UTF8, bufLength, buf);
							//OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, rtText.GetTextPtr(), bufLength, buf, bufLength);
							rtString.append(buf);
							free(buf);
						}
						doc.insert(std::make_pair(iName, ItemValue(rtString)));

					}
					else {
						doc.insert(std::make_pair(iName, ItemValue(&item)));
					}
					
				}
				responses.push_back(doc);

			}
			
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
		Local<Array> viewRes = New<Array>(responses.size());
		try {
			int j;
			for (j = 0; j < responses.size(); j++) {
				std::map<std::string, ItemValue> doc = responses[j];
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
							Nan::Set(arr, 0, Nan::New<String>(value.vectorStrValue[ii]).ToLocalChecked());
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
	std::string unid;	
	std::vector <std::map<std::string, ItemValue>> responses;
};


NAN_METHOD(GetResponseDocumentsAsync) {
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

	AsyncQueueWorker(new ResponseDocumentsWorker(callback, serverStr, dbStr, unidStr));
}
