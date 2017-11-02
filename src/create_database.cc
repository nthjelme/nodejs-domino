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
    char *error_text = (char *)malloc(sizeof(char) * 200);
    DBHANDLE    db_handle;      /* handle of source database */
    NOTEHANDLE  hIconNote;                         /* handle to the icon note */  
    char        db_info[NSF_INFO_SIZE];            /* database info buffer */
    char        set_db_flags[100] = "";            /* modified icon note flags */
    
    v8::Isolate* isolate = info.GetIsolate();
    Local<Object> param = (info[0]->ToObject());  
    Local<Value> dbKey = String::NewFromUtf8(isolate, "database");
    Local<Value> titleKey = String::NewFromUtf8(isolate, "title");
    Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
    Local<Value> dbVal = param->Get(dbKey);
    Local<Value> titleVal = param->Get(titleKey);
    Local<Value> serverVal = param->Get(serverKey);

    String::Utf8Value db_name(dbVal->ToString());
    String::Utf8Value db_title(titleVal->ToString());
    Local<Object> errorObj = Nan::New<Object>();
    
  
    if (error = NotesInitThread()) {
        DataHelper::GetAPIError(error, error_text);
        goto hasError;      
    }


    if (error = NSFDbCreate (*db_name, DBCLASS_NOTEFILE, FALSE)) {     
        DataHelper::GetAPIError(error, error_text);
        NotesTermThread();
        goto hasError;
    } 
  
    if (error = NSFDbOpen(*db_name, &db_handle)) {
        DataHelper::GetAPIError(error, error_text);
        NotesTermThread();
        goto hasError;;
    }

    /* Get the output database information buffer */
    if (error = NSFDbInfoGet (db_handle, db_info)) {
       NSFDbClose (db_handle);
       DataHelper::GetAPIError(error, error_text);		
       NotesTermThread();
       goto hasError;;
   }

    NSFDbInfoModify (db_info, INFOPARSE_TITLE, *db_title); 
    
    if (error = NSFDbInfoSet (db_handle, db_info)) {
        NSFDbClose (db_handle);
        DataHelper::GetAPIError(error, error_text);
        NotesTermThread();         
        goto hasError;;
    }

    if (!NSFNoteOpen(db_handle, NOTE_ID_SPECIAL+NOTE_CLASS_ICON, 0, &hIconNote)) {

        /* Update the FIELD_TITLE ("$TITLE") field if present */
      
        if (NSFItemIsPresent (hIconNote, FIELD_TITLE, (WORD) strlen (FIELD_TITLE)) ) {
            NSFItemSetText(hIconNote, FIELD_TITLE, db_info, MAXWORD);
            NSFNoteUpdate(hIconNote, 0);
        }

        /* Set the DESIGN_FLAGS ($Flags) field  */
        if (error = NSFItemSetText ( hIconNote, DESIGN_FLAGS, set_db_flags, MAXWORD)) {			
            DataHelper::GetAPIError(error, error_text);
            NSFNoteClose (hIconNote);
            NSFDbClose (db_handle);
            NotesTermThread();
            goto hasError;;
        }

        /* Update the note in the database */
        if (error = NSFNoteUpdate (hIconNote, 0)) {        
            DataHelper::GetAPIError(error, error_text);
            NSFNoteClose (hIconNote);
            NSFDbClose (db_handle);
            NotesTermThread();
            goto hasError;;
        }
    }
  
    NSFNoteClose(hIconNote);
  
    if (error = NSFDbClose (db_handle)) {
        DataHelper::GetAPIError(error, error_text);
        NotesTermThread();
        goto hasError;
    }
  
    
    hasError: if (error) {
        Nan::Set(errorObj, New<v8::String>("error").ToLocalChecked(), New<v8::String>(error_text).ToLocalChecked());		
    }

    Callback *callback = new Callback(info[1].As<Function>());	
    Local<Object> resDoc = Nan::New<Object>();
    
    if (error == NOERROR) {
        Nan::Set(resDoc, New<v8::String>("database").ToLocalChecked(), dbVal);	
        Nan::Set(resDoc, New<v8::String>("title").ToLocalChecked(), titleVal);
        Nan::Set(resDoc, New<v8::String>("server").ToLocalChecked(), serverVal);		
    }
    
    
    free(error_text);
    Local<Value> argv[] = { errorObj,resDoc };
    callback->Call(2, argv);
}