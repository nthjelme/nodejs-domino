#include "DocumentItem.h"
#include "nan.h"
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

void pack_document(v8::Local<v8::Object> & target, std::vector<DocumentItem *> items) {	
	for (std::size_t i = 0; i < items.size(); i++) {
		DocumentItem *di = items[i];
		
		if (items[i]->type == 1) {
			
			Nan::Set(target, New<v8::String>(items[i]->name).ToLocalChecked(), New<v8::String>(items[i]->stringValue).ToLocalChecked());
		}
		else if (items[i]->type == 2) {
			Nan::Set(target, New<String>(items[i]->name).ToLocalChecked(), New<Number>(items[i]->numberValue));
		}
		else if (items[i]->type == 3) {
			Nan::Set(target, New<v8::String>(items[i]->name).ToLocalChecked(), New<v8::Date>(items[i]->numberValue).ToLocalChecked());
		}
		else if (items[i]->type == 4) {
			size_t j;
			Local<Array> arr = New<Array>();
			for (size_t j = 0; j < items[i]->vectorStrValue.size(); j++) {
				Nan::Set(arr, j, Nan::New<String>(items[i]->vectorStrValue[j]).ToLocalChecked());
			}
			Nan::Set(target, New<v8::String>(items[i]->name).ToLocalChecked(), arr);
		}
	}
}

std::vector<DocumentItem *> unpack_document(v8::Local<v8::Object> & document) {
	std::vector<DocumentItem*> items;
	
	Local<Array> propNames = document->GetOwnPropertyNames();
	unsigned int num_prop = propNames->Length();
	for (unsigned int i = 0; i < num_prop; i++) {
		DocumentItem *di = new DocumentItem();
		std::vector<char*> docVec;
		Local<Value> name = propNames->Get(i);
		std::string key;

		Local<String> keyStr = Local<String>::Cast(name);
		v8::String::Utf8Value param1(keyStr->ToString());
		key = std::string(*param1);
		Local<Value> val = document->Get(name);

		if (val->IsString()) {
			String::Utf8Value value(val->ToString());
			std::string valueStr = std::string(*value);
			di->stringValue = valueStr;
			di->name = (char*)malloc(key.size() + 1);
			if (di->name) {
				memcpy(di->name, key.c_str(), key.size());
				di->name[key.size()] = '\0';
			}
			di->type = 1;
			items.push_back(di);
		}
		else if (val->IsArray()) {
			Local<Array> arrVal = Local<Array>::Cast(val);
			unsigned int num_locations = arrVal->Length();
			if (num_locations > 0) {
				
				for (unsigned int i = 0; i < num_locations; i++) {
					Local<Object> obj = Local<Object>::Cast(arrVal->Get(i));
					String::Utf8Value value(obj->ToString());
					std::string s_val = std::string(*value);
					size_t val_len = s_val.size();
					char *vecValue =  (char*)malloc(val_len + 1);
					if (vecValue) {
						strcpy(vecValue, s_val.c_str());
						//memcpy(vecValue, s_val.c_str(), val_len);
						//vecValue[val_len] = '\0';
						docVec.push_back(vecValue);
					}
					
				}
				di->vectorStrValue = docVec;
				di->name = (char*)malloc(key.size() + 1);
				if (di->name) {
					memcpy(di->name, key.c_str(), key.size());
					di->name[key.size()] = '\0';
				}
				di->type = 4;
				items.push_back(di);
				
			}
		}
		else if (val->IsNumber()) {
			Local<Number> numVal = val->ToNumber();
			di->numberValue = numVal->NumberValue();
			di->name = (char*)malloc(key.size() + 1);
			if (di->name) {
				memcpy(di->name, key.c_str(), key.size());
				di->name[key.size()] = '\0';
			}
			di->type = 2;
			items.push_back(di);
		}
		else if (val->IsDate()) {			
			di->numberValue = v8::Date::Cast(*val)->NumberValue();
			di->name = (char*)malloc(key.size() + 1);
			if (di->name) {
				memcpy(di->name, key.c_str(), key.size());
				di->name[key.size()] = '\0';
			}
			di->type = 3;
			items.push_back(di);

		}
	}
	return items;

}