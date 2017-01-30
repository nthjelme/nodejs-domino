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

#define     CATEGORY_LEVEL_BEING_SEARCHED 0
#define  MAIN_TOPIC_INDENT    0

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

std::vector <std::map<std::string, ItemValue>> viewResult;


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
	ItemValue iv;
	time_t rawtime;
	double dtime;
	double dateTimeValue;


	std::map <std::string, ItemValue> doc2;

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

		switch (DataType)
		{

			/* Extract a text item from the pSummary. */

		case TYPE_TEXT:
			memcpy(ItemText, pSummaryPos, ValueLength);
			ItemText[ValueLength] = '\0';
			
			char buf[MAXWORD];
			OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, ItemText, MAXWORD, buf, MAXWORD);
			doc2.insert(std::make_pair(ItemName, ItemValue(buf)));
			break;

			/* Extract a text list item from the pSummary. */

		case TYPE_TEXT_LIST:
			printf("is text list, not supported");
			/*if (error = ExtractTextList(
				pSummaryPos,
				ItemText))
				return (ERR(error));
			break;
			*/
			/* Extract a number item from the pSummary. */

		case TYPE_NUMBER:
			memcpy(&NumericItem, pSummaryPos, NUMERIC_SIZE);
			doc2.insert(std::make_pair(ItemName, ItemValue(NumericItem)));
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
			iv = ItemValue();
			iv.dateTimeValue = dateTimeValue;
			iv.type = 3;
			doc2.insert(std::make_pair(ItemName, iv));
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
		viewResult.push_back(doc2);

	}

	/* End of function */

	return (NOERROR);

}



class ViewWorker : public AsyncWorker {
public:
	ViewWorker(Callback *callback, std::string serverName, std::string dbName, std::string viewName,std::string category,std::string find)
		: AsyncWorker(callback), serverName(serverName), dbName(dbName), viewName(viewName),category(category),find(find) {}
	~ViewWorker() {}

	// Executed inside the worker-thread.
	// It is not safe to access V8, or V8 data structures
	// here, so everything we need for input and output
	// should go on `this`.
	void Execute() {
		//char     *DBFileName;            /* pathname of the database */
		//char     *ViewName;              /* name of the view we'll read */
		DBHANDLE hDB;                    /* handle of the database */
		NOTEID      ViewID;              /* note id of the view */
		HCOLLECTION hCollection;         /* collection handle */
		COLLECTIONPOSITION CollPosition; /* index into collection */
		DHANDLE       hBuffer;             /* handle to buffer of info */
		BYTE        *pBuffer;            /* pointer into info buffer */
		BYTE        *pSummary;           /* pointer into info buffer */
		NOTEID      EntryID;             /* a collection entry id */
		DWORD       EntriesFound;        /* number of entries found */
		ITEM_TABLE  ItemTable;           /* table in pSummary buffer */
		WORD        SignalFlag;          /* signal and share warning flags */
		DWORD       i;                   /* a counter */
		STATUS      error = NOERROR;     /* return status from API calls */
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
			printf("opencollection\n");
			DataHelper::GetAPIError(error,error_text);
			SetErrorMessage(error_text);
			NSFDbClose(hDB);
			NotesTermThread();
			return;
		}

		/*Set a COLLECTIONPOSITION to the beginning of the collection. */

		CollPosition.Level = 0;
		CollPosition.Tumbler[0] = 0;

		/* Get the note ID and summary of every entry in the collection. In the
		returned buffer, first comes all of the info about the first entry, then
		all of the info about the 2nd entry, etc. For each entry, the info is
		arranged in the order of the bits in the READ_MASKs. */

		do
		{
			if (error = NIFReadEntries(
				hCollection,        /* handle to this collection */
				&CollPosition,      /* where to start in collection */
				NAVIGATE_NEXT,      /* order to use when skipping */
				1L,                 /* number to skip */
				NAVIGATE_NEXT,      /* order to use when reading */
				0xFFFFFFFF,         /* max number to read */
				
				READ_MASK_NOTEID +  /* info we want */
				READ_MASK_SUMMARY,
				&hBuffer,           /* handle to info buffer (return)  */
				NULL,               /* length of info buffer (return) */
				NULL,               /* entries skipped (return) */
				&EntriesFound,      /* entries read (return) */
				&SignalFlag))       /* share warning and more signal flag
									(return) */
			{
				printf("nifread\n");
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

			for (i = 1; i <= EntriesFound; i++)
			{

				/* Get the ID of this entry. */

				memcpy(&EntryID, pBuffer, sizeof(EntryID));

				/* Advance the pointer over the note id. */

				pBuffer += sizeof(EntryID);

				/* Get the header for the summary items for this entry. */

				memcpy(&ItemTable, pBuffer, sizeof(ItemTable));

				/* Remember where the start of this entry's summary is. Then advance
				the main pointer over the summary. */

				pSummary = pBuffer;
				pBuffer += ItemTable.Length;

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
		NotesTermThread();
		/*try {	
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
		}*/
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
