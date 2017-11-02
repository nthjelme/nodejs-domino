#include "delete_database.h"
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


NAN_METHOD(deleteDatabase) {
    STATUS      error = NOERROR;    /* return status from API calls */
    char *error_text = (char *)malloc(sizeof(char) * 200);
    
    v8::Isolate* isolate = info.GetIsolate();
    Local<Object> param = (info[0]->ToObject());  
    Local<Value> dbKey = String::NewFromUtf8(isolate, "database");
    Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
    Local<Value> dbVal = param->Get(dbKey);
    Local<Value> serverVal = param->Get(serverKey);

    String::Utf8Value db_name(dbVal->ToString());
    Local<Object> errorObj = Nan::New<Object>();
    Local<Object> resDoc = Nan::New<Object>();
    
  
    if (error = NotesInitThread()) {
        DataHelper::GetAPIError(error, error_text);
        goto hasError;  

    }

    if (error = NSFDbDelete(*db_name)) {
        DataHelper::GetAPIError(error, error_text);
        NotesTermThread();
        goto hasError;
    }

    

    hasError: if (error) {
      Nan::Set(errorObj, New<v8::String>("error").ToLocalChecked(), New<v8::String>(error_text).ToLocalChecked());
    }
    
    if (error == NOERROR) {
      Nan::Set(resDoc, New<v8::String>("status").ToLocalChecked(), New<v8::String>("deleted").ToLocalChecked());
    }
  
    Callback *callback = new Callback(info[1].As<Function>());	

    free(error_text);
    Local<Value> argv[] = { errorObj,resDoc };
    callback->Call(2, argv);
}