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
#include "DocumentItem.h"
#include "DataHelper.h"
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <nsfdb.h>
#include <nsfsearc.h>
#include "nsf_search.h"
#include <nif.h>
#include <textlist.h>
#include <osmem.h>
#include <miscerr.h>
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

NOTEHANDLE   note_handle_srch;
std::vector<DocumentItem *> items2;
std::vector<std::vector<DocumentItem *>> view2;

STATUS LNPUBLIC print_fields
            (void far *db_handle,
            SEARCH_MATCH far *pSearchMatch,
            ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
 
		OID tempOID;
		STATUS       error;
		std::vector<DocumentItem *> curr_match;
		items2 = curr_match;

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

/* Skip this note if it does not really match the search criteria (it is
now deleted or modified).  This is not necessary for full searches,
but is shown here in case a starting date was used in the search. */

    if (!(SearchMatch.SERetFlags & SE_FMATCH))
        return (NOERROR);
		
/* Open the note. */

  if (error = NSFNoteOpen (*(DBHANDLE far *)db_handle,SearchMatch.ID.NoteID,0, &note_handle_srch)) {
		return (ERR(error));
	}
	
	if (error = NSFItemScan(
		note_handle_srch,	/* note handle */
		field_actions2,	/* action routine for fields */
		&note_handle_srch))	/* argument to action routine */

	{
		return(ERR(error));
	}

	NSFNoteGetInfo(note_handle_srch, _NOTE_OID, &tempOID);
		char unid_buffer[33];
		snprintf(unid_buffer, 33, "%08X%08X%08X%08X", tempOID.File.Innards[1], tempOID.File.Innards[0], tempOID.Note.Innards[1], tempOID.Note.Innards[0]);
		DocumentItem *di = new DocumentItem();
		di->name = (char*)malloc(6);
		if (di->name) {
			strcpy(di->name, "@unid");
		}
		di->type = 1;
		di->stringValue = std::string(unid_buffer);
		items2.push_back(di);
	view2.push_back(items2);

	if (error = NSFNoteClose (note_handle_srch))
		return (ERR(error));

	/* End of subroutine. */
	return (NOERROR);
}


class NSFSearchWorker : public AsyncWorker {
public:
	NSFSearchWorker(Callback *callback, std::string serverName, std::string dbName, std::string search_formula)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), search_formula(search_formula) {}
	~NSFSearchWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		DBHANDLE    db_handle;      /* handle of source database */
		char        formula[] = "@All"; /* an ASCII selection formula. */
		FORMULAHANDLE    formula_handle;    /* a compiled selection formula */
    WORD     wdc;                       /* a word we don't care about */
    STATUS   error = NOERROR;           /* return status from API calls */
		char *error_text = (char *)malloc(sizeof(char) * 200);
    if (error = NotesInitThread()) {			
			SetErrorMessage("error init notes thread");
		}
        
    if (error = NSFDbOpen (dbName.c_str(), &db_handle)) {  
    	NotesTermThread();
      SetErrorMessage("error opening database\n");
      return;
		} 

		if (error = NSFFormulaCompile (
                NULL,               /* name of formula (none) */
                (WORD) 0,           /* length of name */
                formula,            /* the ASCII formula */
                (WORD) strlen(formula),    /* length of ASCII formula */
                &formula_handle,    /* handle to compiled formula */
                &wdc,               /* compiled formula length (don't care) */
                &wdc,               /* return code from compile (don't care) */
                &wdc, &wdc, &wdc, &wdc)) /* compile error info (don't care) */
        
    {
    	NSFDbClose (db_handle);
      NotesTermThread();
      DataHelper::GetAPIError(error, error_text);
			SetErrorMessage(error_text);
      return;
        
		}

        if (error = NSFSearch (
                db_handle,      /* database handle */
                formula_handle, /* selection formula */
                NULL,           /* title of view in selection formula */
                0,              /* search flags */
                NOTE_CLASS_DOCUMENT,/* note class to find */
                NULL,           /* starting date (unused) */
                print_fields,   /* call for each note found */
                &db_handle,     /* argument to print_fields */
                NULL))          /* returned ending date (unused) */
        {
            NSFDbClose (db_handle);
            NotesTermThread();
            DataHelper::GetAPIError(error, error_text);
						SetErrorMessage(error_text);
            return;
        
        }

        /* Free the memory allocated to the compiled formula. */

        OSMemFree (formula_handle);
				NSFDbClose(db_handle);

		NotesTermThread();
	}

	void HandleOKCallback() {
		HandleScope scope;
		Local<Array> viewRes = New<Array>();//DataHelper::getV8Data(viewResult);

		for (size_t j = 0; j < view2.size(); j++) {
			std::vector<DocumentItem*> column = view2[j];
			Local<Object> resDoc = Nan::New<Object>();
			for (size_t k = 0; k < column.size(); k++) {
				DocumentItem *di = column[k];
				if (di->type == 1) {
					Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), New<v8::String>(di->stringValue).ToLocalChecked());
				}
				else if (di->type == 2) {
					Nan::Set(resDoc, New<String>(di->name).ToLocalChecked(), New<Number>(di->numberValue));
				}
				else if (di->type == 3) {
					Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), New<v8::Date>(di->numberValue).ToLocalChecked());
				}
				else if (di->type == 4) {
					Local<Array> arr = New<Array>();
					for (size_t j = 0; j < di->vectorStrValue.size(); j++) {
						if (di->vectorStrValue[j]) {
							Nan::Set(arr, j, Nan::New<String>(di->vectorStrValue[j]).ToLocalChecked());
						}
					}
					Nan::Set(resDoc, New<v8::String>(di->name).ToLocalChecked(), arr);
				}
			}
			Nan::Set(viewRes, j, resDoc);
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
	std::string search_formula;	
};

STATUS LNCALLBACK field_actions2(WORD unused, WORD item_flags, char far *name_ptr, WORD name_len, void far *item_valu, DWORD item_value_len, void far *note_handle_srch2) {
	WORD         field_len;
	char         field_text[132000];
	STATUS		error = NOERROR;
	NUMBER       number_field;

	BLOCKID                bidLinksItem;
	DWORD                  dwLinksValueLen;
	BLOCKID                bidLinksValue;
	WORD                   item_type;
	DocumentItem * di = new DocumentItem();
	char error_text[200];

	
	char *field_name = (char*)malloc(name_len + 1);
	if (field_name) {
		memcpy(field_name, name_ptr, name_len);
		field_name[name_len] = '\0';
	}

	if (error = NSFItemInfo(note_handle_srch, field_name,
		strlen(field_name), &bidLinksItem,
		&item_type, &bidLinksValue,
		&dwLinksValueLen)) {
		DataHelper::GetAPIError(error, error_text);

		return (error);
	}
	if (item_type == TYPE_TEXT) {
		field_len = NSFItemGetText(
			note_handle_srch,
			field_name,
			field_text,
			(WORD) sizeof(field_text));

		char buf[MAXWORD];
		OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, buf, MAXWORD);
		di->stringValue = std::string(buf);
		di->name = (char*)malloc(name_len + 1);
		if (di->name) {
			memcpy(di->name, name_ptr, name_len);
			di->name[name_len] = '\0';
		}
		di->type = 1;
		items2.push_back(di);

	}
	else if (item_type == TYPE_TEXT_LIST) {

		WORD num_entries = NSFItemGetTextListEntries(note_handle_srch,
			field_name);
		std::vector<char*> vectorStrValue = std::vector<char*>(0);
		for (int counter = 0; counter < num_entries; counter++)
		{
			field_len = NSFItemGetTextListEntry(note_handle_srch,
				field_name, counter, field_text, (WORD)(sizeof(field_text) - 1));
			char *buf = (char*)malloc(field_len + 1);
			OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, field_text, MAXWORD, buf, MAXWORD);
			vectorStrValue.push_back(buf);

		}
		di->vectorStrValue = vectorStrValue;
		di->name = field_name;
		di->type = 4;
		items2.push_back(di);
	}
	else if (item_type == TYPE_NUMBER) {
		NSFItemGetNumber(
			note_handle_srch,
			field_name,
			&number_field);
		di->numberValue = (double)number_field;
		di->name = field_name;
		di->type = 2;
		items2.push_back(di);
	}

	else if (item_type == TYPE_TIME) {
		TIMEDATE time_date;
		if (NSFItemGetTime(note_handle_srch,
			field_name,
			&time_date)) {
			TIME tid;
			struct tm * timeinfo;

			tid.GM = time_date;
			tid.zone = 0;
			tid.dst = 0;
			TimeGMToLocalZone(&tid);

			time_t rawtime = time(0);
			timeinfo = localtime(&rawtime);
			timeinfo->tm_year = tid.year - 1900;
			timeinfo->tm_mon = tid.month - 1;
			timeinfo->tm_mday = tid.day;

			timeinfo->tm_hour = tid.hour;
			timeinfo->tm_min = tid.minute - 1;
			timeinfo->tm_sec = tid.second - 1;

			double dtime = static_cast<double>(mktime(timeinfo));

			double dateTimeValue = dtime * 1000; // convert to double time
			di->numberValue = dateTimeValue;
			di->name = field_name;
			di->type = 3;
			items2.push_back(di);
		}


	}

	//delete field_name;

	return(NOERROR);

}


NAN_METHOD(SearchNsfAsync) {
	v8::Isolate* isolate = info.GetIsolate();

	Local<Object> param = (info[0]->ToObject());
	Local<String> searchParam = (info[1]->ToString());
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	String::Utf8Value serverName(serverVal->ToString());
	String::Utf8Value dbName(databaseVal->ToString());
	String::Utf8Value search_formula(searchParam->ToString());

	std::string serverStr;
	std::string dbStr;
	std::string searchFormulaStr;
	serverStr = std::string(*serverName);
	dbStr = std::string(*dbName);
	searchFormulaStr = std::string(*search_formula);
	Callback *callback = new Callback(info[2].As<Function>());

	AsyncQueueWorker(new NSFSearchWorker(callback, serverStr, dbStr, searchFormulaStr));
}