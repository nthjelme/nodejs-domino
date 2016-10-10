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
#include <lncppapi.h>
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <osmisc.h>
#include <ctime>

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
		LNSetThrowAllErrors(TRUE);

		try {
			LNNotesSession session;
			session.InitThread();

			LNDatabase Db;
			LNDocument			  NewDoc;
			BOOLEAN isEdit = false;
			
			session.GetDatabase(dbName.c_str(), &Db, serverName.c_str());
			Db.Open();

			std::map<std::string, ItemValue>::iterator iter;
			iter = doc.find("@unid");
			if (iter!=doc.end()) {
				isEdit = true;
				ItemValue unidItem = iter->second;				
				const char * chrUNID = unidItem.stringValue.c_str();
				const LNString * lnstrUNID = new LNString(chrUNID);
				LNUniversalID * lnUNID = new LNUniversalID(*lnstrUNID);
				const UNIVERSALNOTEID * unidUNID = lnUNID->GetUniversalID();
				Db.GetDocument(unidUNID, &NewDoc);				
				isEdit = true;
			} else {
				Db.CreateDocument(&NewDoc);				
			}
			NewDoc.Open(LNNOTEOPENFLAGS_DEFAULT);
			std::map<std::string, ItemValue>::iterator it;
			for (it = doc.begin(); it != doc.end(); it++)
			{
				ItemValue value = it->second;				
				if (value.type == 0) {
					LNNumbers item = LNNumbers();
					if (isEdit) {
						if (NewDoc.HasItem(it->first.c_str())) {
							NewDoc.GetItem(it->first.c_str(), &item);
						}
						else {
							NewDoc.CreateItem(it->first.c_str(), &item);
						}
					}
					else {
						NewDoc.CreateItem(it->first.c_str(), &item);
					}

					LNNumber lnvalue = value.numberValue;
					item.SetValue(lnvalue);
				}
				else if(value.type == 1) {
					LNText item = LNText();
					if (isEdit) {
						if (NewDoc.HasItem(it->first.c_str())) {
							NewDoc.GetItem(it->first.c_str(), &item);
						}
						else {
							NewDoc.CreateItem(it->first.c_str(), &item);
						}
					}
					else {
						NewDoc.CreateItem(it->first.c_str(), &item);
					}
					LNString valStr;
					LNStringTranslate(value.stringValue.c_str(), LNCHARSET_UTF8,&valStr);
					
					item.SetValue(valStr);
				}
				else if (value.type == 2) {
					LNText item = LNText();
					if (isEdit) {
						if (NewDoc.HasItem(it->first.c_str())) {
							NewDoc.GetItem(it->first.c_str(), &item);
							item.DeleteAll();
						}
						else {
							NewDoc.CreateItem(it->first.c_str(), &item);
						}
					}
					else {
						NewDoc.CreateItem(it->first.c_str(), &item);
					}
					size_t ii;
					for (ii = 0; ii < value.vectorStrValue.size(); ii++) {						
						LNString val;
						LNStringTranslate(value.vectorStrValue[ii].c_str(), LNCHARSET_UTF8, &val);
						item.Append(val);
					}

				}
				else if (value.type == 3) {					
					LNDatetimes item = LNDatetimes();					
					LNDatetime dt = LNDatetime();				

					try {
						std::time_t t = static_cast<time_t>(value.dateTimeValue / 1000);
						struct tm* ltime = localtime(&t);
						int year = ltime->tm_year + 1900;
						int month = ltime->tm_mon + 1;
						int day = ltime->tm_mday;
						int hour = ltime->tm_hour+1;
						int minute = ltime->tm_min+1;
						int second = ltime->tm_sec+1;
						dt.SetDate(month, day, year);
						dt.SetTime(hour, minute, second);
						NewDoc.CreateItem(it->first.c_str(), &item);
						item.SetValue(dt);					
					}
					catch (...) {
						std::cout << "error creating tm struct" << std::endl;
					}					
				}
			}

			NewDoc.Save();
			UNIVERSALNOTEID *noteid = NewDoc.GetUniversalID();
			LNUniversalID lnUnid = LNUniversalID(noteid);			
			LNString unidStr = lnUnid.GetText();			
			doc.insert(std::make_pair("@unid", ItemValue(unidStr.GetTextPtr())));
			session.TermThread();
		} catch (LNSTATUS Lnerror) {
			char ErrorBuf[512];
			LNGetErrorMessage(Lnerror, ErrorBuf, 512);
			std::cout << "SaveDocError:  " << ErrorBuf << std::endl;
		}
	}

	void HandleOKCallback() {
		HandleScope scope;
		Local<Object> resDoc = Nan::New<Object>();		
		

		try {
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
					size_t ii;
					Local<Array> arr = New<Array>(value.vectorStrValue.size());					
					for (ii = 0; ii < value.vectorStrValue.size(); ii++) {
						Nan::Set(arr, ii, Nan::New<String>(value.vectorStrValue[ii]).ToLocalChecked());
					}
					Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), arr);

				}
				else if (value.type == 3) {					
					Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), New<v8::Date>(value.dateTimeValue).ToLocalChecked());
				}
			}

		}
		catch (LNSTATUS Lnerror) {
			char ErrorBuf[512];
			LNGetErrorMessage(Lnerror, ErrorBuf, 512);
			std::cout << "Handle Error:  " << ErrorBuf << std::endl;
		}
		std::cout << "return" << std::endl;
		Local<Value> argv[] = {
			Null()
			, resDoc
		};
		std::cout << "set callback,call" << std::endl;
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
			std::vector<std::string> docVec;
			for (unsigned int i = 0; i < num_locations; i++) {
				Local<Object> obj = Local<Object>::Cast(arrVal->Get(i));
				String::Utf8Value value(obj->ToString());
				docVec.push_back(*value);
			}
			doc.insert(std::make_pair(key, ItemValue(docVec)));			

		} else if (val->IsNumber()) {
			Local<Number> numVal = val->ToNumber();			
			doc.insert(std::make_pair(key, ItemValue(numVal->NumberValue())));			
		} else if (val->IsDate()) {
			double ms = v8::Date::Cast(*val)->NumberValue();
			//Local<Number> numVal = val->ToNumber();
			//double millisSinceEpoch = numVal->NumberValue();
			//std::time_t t = static_cast<time_t>(ms);
			std::cout << "start time in:" << ms << std::endl;
			doc.insert(std::make_pair(key, ItemValue(ms,3)));
		}
		
	}	
	
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new SaveDocumentWorker(callback, serverStr, dbStr, doc));
}