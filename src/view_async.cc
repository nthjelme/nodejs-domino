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
#include "view_async.h"  
#include "DocumentItem.h"
#include "DataHelper.h"
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <nsfdb.h>
#include <nif.h>
#include <osmem.h>
#include <miscerr.h>
#include <osmisc.h>

#define     CATEGORY_LEVEL_BEING_SEARCHED 0
#define  MAIN_TOPIC_INDENT    0
#define MALLOC_AMOUNT   6048

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

//std::vector <std::map<std::string, ItemValue>> viewResult;

std::vector<std::vector<DocumentItem *>> view;


STATUS PrintSummary(BYTE *pSummary)

/* This function prints the items in the summary for one
entry in a collection.

The information in a view summary is as follows:

header (total length of pSummary, number of items in pSummary)
length of item #1 (including data type)
length of item #2 (including data type)
length of item #3 (including data type)
...
data type of item #1
value of item #1
data type of item #2
value of item #2
data type of item #3
value of item #3
....
*/

{

	/* Local constants */

#define  MAX_ITEMS          20
#define  MAX_ITEM_LEN       100
#define MAX_ITEM_NAME_LEN   100
#define  DATATYPE_SIZE      sizeof(USHORT)
#define  ITEM_LENGTH_SIZE   sizeof(USHORT)
#define  NUMERIC_SIZE       sizeof(NUMBER)
#define  TIME_SIZE          sizeof(TIMEDATE)
	
	/* cleanup flag values*/
#define DO_NOTHING      0x0000
#define FREE_KEY1       0x0004
#define FREE_TRANSLATEDKEY       0x0008
#define FREE_PKEY       0x0011


	/* Local variables */

	BYTE       *pSummaryPos;        /* current position in summary */
	ITEM_TABLE  ItemTable;          /* header at start of summary */
	USHORT      ItemCount;          /* number of items in summary */
	USHORT      NameLength;         /* length of item name w/out terminator*/
	USHORT      ValueLength;        /* length of item value, incl. type */
	WORD        DataType;           /* item data type word */
	
	USHORT      i;                  /* counter for loop over items */
	ITEM    Items[MAX_ITEMS];       /* Stores the array of ITEMs */
	char    ItemText[MAX_ITEM_LEN]; /* Text rendering of item value */
	char    ItemName[MAX_ITEM_NAME_LEN];/* Zero terminated item name */
	NUMBER  NumericItem;            /* a numeric item */
	TIMEDATE   TimeItem;            /* a time/date item */
	
	time_t rawtime;
	double dtime;
	double dateTimeValue;

	std::vector<DocumentItem*> column;

										/* pSummaryPos points to the beginning of the summary buffer. Copy
										the ITEM_TABLE header at the beginning of the summary buffer
										to a local variable. Advance pSummaryPos to point to the next
										byte in the summary buffer after the ITEM_TABLE.
										*/
	pSummaryPos = pSummary;	
	
	memcpy((char*)(&ItemTable), pSummaryPos, sizeof(ITEM_TABLE));
	pSummaryPos += sizeof(ItemTable);

	/* pSummaryPos now points to the first ITEM in an array of ITEM
	structures. Copy this array of ITEM structures into the global
	Items[] array.
	*/
	ItemCount = ItemTable.Items;
	for (i = 0; i < ItemCount; i++)
	{
		
		memcpy((char*)(&Items[i]), pSummaryPos, sizeof(ITEM));
		pSummaryPos += sizeof(ITEM);
	}
	

	/* pSummaryPos now points to the first item name. Loop over each
	item, copying the item name into the ItemName variable and
	converting the item value to printable text in ItemText.
	*/

	for (i = 0; i < ItemCount; i++)
	{		
		NameLength = Items[i].NameLength;
		memcpy(ItemName, pSummaryPos, NameLength);
		ItemName[NameLength] = '\0';
		pSummaryPos += NameLength;
		
		/* pSummaryPos now points to the item value. First get the
		data type. Then step over the data type word to the data
		value and convert the value to printable text. Store the
		text in ItemText.
		*/
		memcpy((char*)(&DataType), pSummaryPos, sizeof(WORD));
		pSummaryPos += sizeof(WORD);

		ValueLength = Items[i].ValueLength - sizeof(WORD);
		/* If the item data type is text, copy into ItemText[]. */
		DocumentItem  *di = new DocumentItem();
		di->name = (char*)malloc(NameLength + 1);
		if (di->name) {
			memcpy(di->name, ItemName, NameLength);
			di->name[NameLength] = '\0';
		}

		switch (DataType)
		{

			/* Extract a text item from the pSummary. */

			case TYPE_TEXT:
				memcpy(ItemText, pSummaryPos, ValueLength);
				ItemText[ValueLength] = '\0';
			
				char buf[MAXWORD];
				OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, ItemText, MAXWORD, buf, MAXWORD);
			
				di->stringValue = buf;
				di->type = 1;
				column.push_back(di);
				break;

			/* Extract a text list item from the pSummary. */

			case TYPE_TEXT_LIST:
				printf("is text list, not (yet) supported");
				/*if (error = ExtractTextList(
					pSummaryPos,
					ItemText))
					return (ERR(error));
				break;
				*/

			/* Extract a number item from the pSummary. */
			case TYPE_NUMBER:
				memcpy(&NumericItem, pSummaryPos, NUMERIC_SIZE);			
				di->numberValue = NumericItem;
				di->type = 2;
				column.push_back(di);
				break;

			/* Extract a time/date item from the pSummary. */

			case TYPE_TIME:
				memcpy(&TimeItem, pSummaryPos, TIME_SIZE);
				TIME tid;
				struct tm * timeinfo;
	
				tid.GM = TimeItem;
				tid.zone = 0;
				tid.dst = 0;
				TimeGMToLocalZone(&tid);
	
				rawtime = time(0);
				timeinfo = localtime(&rawtime);
				timeinfo->tm_year = tid.year - 1900;
				timeinfo->tm_mon = tid.month - 1;
				timeinfo->tm_mday = tid.day;

				timeinfo->tm_hour = tid.hour;
				timeinfo->tm_min = tid.minute - 1;
				timeinfo->tm_sec = tid.second - 1;
				dtime = static_cast<double>(mktime(timeinfo));

				dateTimeValue = dtime * 1000; // convert to double time			
				di->numberValue = dateTimeValue;
				di->type = 3;
				column.push_back(di);
				break;

			/* If the pSummary item is not one of the data types this program
			handles. */

			default:			
				break;
			}
		/* Advance to next item in the pSummary. */
		pSummaryPos += ValueLength;
		//pSummaryPos += (ItemLength[i] - DATATYPE_SIZE);
		/* End of loop that is extracting each item in the pSummary. */
		//viewResult.push_back(doc2);
		

	}
	view.push_back(column);
	/* End of function */

	return (NOERROR);

}



class ViewWorker : public AsyncWorker {
public:
	ViewWorker(Callback *callback, std::string serverName, std::string dbName, std::string viewName,std::string category,std::string findByKey)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), viewName(viewName),category(category),findByKey(findByKey) {}
	~ViewWorker() {
		view.clear();

	}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {

		WORD					cleanup = DO_NOTHING;
		DBHANDLE				hDB;                    /* handle of the database */
		NOTEID					ViewID;              /* note id of the view */
		HCOLLECTION				hCollection;         /* collection handle */
		COLLECTIONPOSITION		CollPosition; /* index into collection */
		DHANDLE					hBuffer;             /* handle to buffer of info */
		BYTE					*pBuffer;            /* pointer into info buffer */
		BYTE			        *pSummary;           /* pointer into info buffer */
		NOTEID					EntryID;             /* a collection entry id */		
		DWORD					number_match;
		DWORD					EntriesFound;        /* number of entries found */
		ITEM_TABLE				ItemTable;           /* table in pSummary buffer */
		WORD					SignalFlag;          /* signal and share warning flags */		
		STATUS					error = NOERROR;     /* return status from API calls */
		BOOL					FirstTime = TRUE;
		char					*pTemp, *pKey;
		ITEM_TABLE				Itemtbl;
		ITEM					Item;
		WORD					Word;
		char					*Key1;          /* secondary key                          */
		WORD					Item1ValueLen; /* len of actual primary text to match on */
		WORD					TranslatedKeyLen;
		char					*TranslatedKey;      /* Translated string key */
		char *error_text = (char *) malloc(sizeof(char) * 200);   
		
		if (error = NotesInitThread())
		{			
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			return;
		}

		if (error = NSFDbOpen(dbName.c_str(), &hDB))
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NotesTermThread();
			return;
		}

		if (error = NIFFindView(
			hDB,
			viewName.c_str(),
			&ViewID))
		{		
			printf("niffindview\n");
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NSFDbClose(hDB);
			NotesTermThread();
			return;
		}

		if (error = NIFOpenCollection(
			hDB,            /* handle of db with view */
			hDB,            /* handle of db with data */
			ViewID,         /* note id of the view */
			0,              /* collection open flags */
			NULLHANDLE,     /* handle to unread ID list (input and return) */
			&hCollection,   /* collection handle (return) */
			NULLHANDLE,     /* handle to open view note (return) */
			NULL,           /* universal note id of view (return) */
			NULLHANDLE,     /* handle to collapsed list (return) */
			NULLHANDLE))    /* handle to selected list (return) */
		
		{			
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NSFDbClose(hDB);
			NotesTermThread();
			return;
		}

		/*Set a COLLECTIONPOSITION to the beginning of the collection. */

		CollPosition.Level = 0;
		CollPosition.Tumbler[0] = 0;

		number_match = 0xFFFFFFFF;         /* max number to read */
		
		if (category.length() > 0) {
			error = NIFFindByName(hCollection, category.c_str(), FIND_CASE_INSENSITIVE, &CollPosition, &number_match);			
			if (ERR(error) == ERR_NOT_FOUND) {
				SetErrorMessage("Category not found in the collection");
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				NotesTermThread();
				return;
			}
			if (error) {
				DataHelper::GetAPIError(error, error_text);
				SetErrorMessage(error_text);
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				NotesTermThread();
				return;
			}
		}
		else if (findByKey.length()>0) {
			TranslatedKey = (char *)malloc(256);
			if (TranslatedKey == NULL)
			{
				printf("Error: Out of memory.\n");
			}
			else
				cleanup |= FREE_TRANSLATEDKEY;
			Key1 = (char *)malloc(256);
			if (Key1 == NULL)
			{
				printf("Error: Out of memory.\n");
			}
			else
				cleanup |= FREE_KEY1;
				/* Translate the input key to LMBCS */
				TranslatedKeyLen = OSTranslate (OS_TRANSLATE_UTF8_TO_LMBCS,
                                findByKey.c_str(),
                                (WORD) strlen (findByKey.c_str()),
                                TranslatedKey,
                                256);

			Item1ValueLen = TranslatedKeyLen + sizeof(WORD);
			pKey = (char *)malloc(MALLOC_AMOUNT);

			if (pKey == NULL)
			{
				printf("Error: Out of memory.\n");				
			}
			else
				cleanup |= FREE_PKEY;

			pTemp = pKey;

			Itemtbl.Length = (sizeof(Itemtbl) +
				(1 * (sizeof(Item))) + Item1ValueLen);
			Itemtbl.Items = 1;

			memcpy(pTemp, &Itemtbl, sizeof(Itemtbl));
			pTemp += sizeof(Itemtbl);

			Item.NameLength = 0;
			Item.ValueLength = Item1ValueLen;
			memcpy(pTemp, &Item, sizeof(Item));
			pTemp += sizeof(Item);

			Word = TYPE_TEXT;
			memcpy(pTemp, &Word, sizeof(Word));
			pTemp += sizeof(Word);

			memcpy(pTemp, TranslatedKey, TranslatedKeyLen);
			pTemp += TranslatedKeyLen;
			

			error = NIFFindByKey(hCollection, pKey, FIND_CASE_INSENSITIVE, &CollPosition, NULL);
			if (ERR(error) == ERR_NOT_FOUND) {
				SetErrorMessage("Note not found in the collection");
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				NotesTermThread();
				return;
			}
			if (error) {
				DataHelper::GetAPIError(error, error_text);
				SetErrorMessage(error_text);
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				NotesTermThread();
				return;
			}
			number_match = 1;
		}
		else {
			FirstTime = FALSE;
		}
		

		/* Get the note ID and summary of every entry in the collection. In the
		returned buffer, first comes all of the info about the first entry, then
		all of the info about the 2nd entry, etc. For each entry, the info is
		arranged in the order of the bits in the READ_MASKs. */

		do
		{
			if (error = NIFReadEntries(
				hCollection,        /* handle to this collection */
				&CollPosition,      /* where to start in collection */
				(WORD)(FirstTime ? NAVIGATE_CURRENT : NAVIGATE_NEXT),
				/* order to use when skipping */
				FirstTime ? 0L : 1L,    /* number to skip */
				//NAVIGATE_NEXT,      /* order to use when skipping */
				//1L,                 /* number to skip */
				NAVIGATE_NEXT,      /* order to use when reading */
				number_match,
				READ_MASK_NOTEID +  /* info we want */
				//READ_MASK_INDENTLEVELS +
				//READ_MASK_INDEXPOSITION+
				READ_MASK_SUMMARY,
				&hBuffer,           /* handle to info buffer (return)  */
				NULL,               /* length of info buffer (return) */
				NULL,               /* entries skipped (return) */
				&EntriesFound,      /* entries read (return) */
				&SignalFlag))       /* share warning and more signal flag
									(return) */
			{				
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				DataHelper::GetAPIError(error,error_text);
				SetErrorMessage(error_text);
				NotesTermThread();
				return;
			}

			/* Check to make sure there was a buffer of information returned. */

			if (hBuffer == NULLHANDLE)
			{
				
				NIFCloseCollection(hCollection);
				NSFDbClose(hDB);
				printf("\nEmpty buffer returned by NIFReadEntries.\n");
				NotesTermThread();
				return;
			}

			/* Lock down (freeze the location) of the information buffer. Cast
			the resulting pointer to the type we need. */

			pBuffer = (BYTE *)OSLockObject(hBuffer);

			/* Start a loop that extracts the info about each collection entry from
			the information buffer. */

			for (DWORD i = 1; i <= EntriesFound; i++)
			{

				/* Get the ID of this entry. */

				memcpy(&EntryID, pBuffer, sizeof(EntryID));

				/* Advance the pointer over the note id. */

				pBuffer += sizeof(EntryID);

				/* get ident level of entry
				entry_indent = *(WORD*)pBuffer;
				pBuffer += sizeof(WORD);

				entry_index_size = COLLECTIONPOSITIONSIZE
				((COLLECTIONPOSITION*)pBuffer);
				*/
				/* Get the header for the summary items for this entry. */

				memcpy(&ItemTable, pBuffer, sizeof(ItemTable));

				/* Remember where the start of this entry's summary is. Then advance
				the main pointer over the summary. */

				pSummary = pBuffer;
				pBuffer += ItemTable.Length;

				if (category.size() > 0) {
					if (CollPosition.Level == CATEGORY_LEVEL_BEING_SEARCHED) {
						/* Indicate that there is no more to do. */
						SignalFlag &= ~SIGNAL_MORE_TO_DO;
						break;
					}
				}

				//if (NOTEID_CATEGORY & EntryID) continue;

//				if (entry_indent != MAIN_TOPIC_INDENT) continue;

				/* Call a local function to print the summary buffer. */
				if (error = PrintSummary(pSummary))
				{
					printf("error printsummary\n");
					OSUnlockObject(hBuffer);
					OSMemFree(hBuffer);
					NIFCloseCollection(hCollection);
					NSFDbClose(hDB);
					DataHelper::GetAPIError(error,error_text);
					SetErrorMessage(error_text);
					NotesTermThread();
					return;
				}

				/* End of loop that gets info about each entry. */

			}

			/* Unlock the list of note IDs. */

			OSUnlockObject(hBuffer);

			/* Free the memory allocated by NIFReadEntries. */

			OSMemFree(hBuffer);

			/* Loop if the end of the collection has not been reached because the buffer
			was full.  */
			if (FirstTime)
				FirstTime = FALSE;

		} while (SignalFlag & SIGNAL_MORE_TO_DO);

		
		if (error = NIFCloseCollection(hCollection))
		{
			NSFDbClose(hDB);
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NotesTermThread();
			return;
		}

		if (error = NSFDbClose(hDB))
		{
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NotesTermThread();
			return;
		}

		if (cleanup & FREE_KEY1)
			free(Key1);

		if (cleanup & FREE_TRANSLATEDKEY)
			free(TranslatedKey);
		
		if (cleanup & FREE_PKEY)
			free(pKey);
		NotesTermThread();
		
	}

	// Executed when the async work is complete
	// this function will be run inside the main event loop
	// so it is safe to use V8 again
	void HandleOKCallback() {
		HandleScope scope;
		Local<Array> viewRes = New<Array>();//DataHelper::getV8Data(viewResult);
		
		for (size_t j = 0; j < view.size(); j++) {
			std::vector<DocumentItem*> column = view[j];
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
	std::string viewName;
	std::string category;
	std::string findByKey;
	

};


NAN_METHOD(GetViewAsync) {	
	v8::Isolate* isolate = info.GetIsolate();	
	Local<Object> param = (info[0]->ToObject());
	Local<Object> viewParam = (info[1]->ToObject());
	Local<Value> viewKey = String::NewFromUtf8(isolate, "view");
	Local<Value> catKey = String::NewFromUtf8(isolate, "category");
	Local<Value> findKey = String::NewFromUtf8(isolate, "findByKey");
	Local<Value> serverKey = String::NewFromUtf8(isolate, "server");
	Local<Value> databaseKey = String::NewFromUtf8(isolate, "database");
	Local<Value> serverVal = param->Get(serverKey);
	Local<Value> databaseVal = param->Get(databaseKey);

	Local<Value> viewVal = viewParam->Get(viewKey);
	
	
	std::string catStr;
	std::string findByKeyStr;

	
	if (viewParam->Has(findKey)) {
		Local<Value> findVal = viewParam->Get(findKey);
		String::Utf8Value find(findVal->ToString());
		findByKeyStr = std::string(*find);
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

	AsyncQueueWorker(new ViewWorker(callback, serverStr, dbStr, viewStr,catStr,findByKeyStr));
}
