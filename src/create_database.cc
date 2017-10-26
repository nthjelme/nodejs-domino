#include "create_database.h"
#include <nsfdb.h>

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

NAN_METHOD(createDatabase) {
  STATUS      error = NOERROR;    /* return status from API calls */
  v8::Isolate* isolate = info.GetIsolate();
	Local<String> databaseName = (info[0]->ToString());
	String::Utf8Value db_name(databaseName->ToString());
  printf("database: %s\n",*db_name);

  if (error = NSFDbCreate (*db_name, DBCLASS_NOTEFILE, FALSE))
    {     
	    printf("error creating db\n");
    }
  
  Callback *callback = new Callback(info[1].As<Function>());	
  Local<Object> resDoc = Nan::New<Object>();
	Nan::Set(resDoc, New<v8::String>("status").ToLocalChecked(), New<v8::String>("Database successfully created").ToLocalChecked());		
  Local<Value> argv[] = { resDoc };
  callback->Call(1, argv);
}