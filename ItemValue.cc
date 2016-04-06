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

#include "ItemValue.h"
#include <iostream>

void ItemValue::TextItem(LNItem *item) {
	LNText tekst = (LNText)*item;
	LNINT j;
	std::vector<std::string> result_list;

	for (j = 0; j < tekst.GetCount(); j++) {
		LNString value = tekst[j];
		char buf[132000];
		LNStringTranslate(value, LNCHARSET_UTF8, 132000, buf);
		//OSTranslate(OS_TRANSLATE_LMBCS_TO_UTF8, value.GetTextPtr(), 132000, buf, 132000);
		result_list.push_back(buf);
	}
	vectorStrValue = result_list;
	type = 2;
}

ItemValue::ItemValue(LNItem *item) {
	LNITEMTYPE type = item->GetType();
	if (type == LNITEMTYPE_TEXT) {
		TextItem(item);		
	}
	else if (type == LNITEMTYPE_NUMBERS) {
		LNNumbers tall = (LNNumbers)*item;
		LNNumber value = tall[0];		
		type = 0;
		numberValue = value;		
	}
	else if (type == LNITEMTYPE_DATETIMES) {
		LNDatetimes dates = (LNDatetimes)*item;
		if (!dates.IsNull()) {
			LNDatetime date = dates[0];
			DateValue(&date);			
		}

	}
}

void ItemValue::DateValue(LNDatetime * date) {	
	LNINT day,year,month,hour,minute,second,hundredth;
	std::string isodate;
	char * em = "";
	char * ed = "";
	char * eM = "";
	char * es = "";
	char * eH = "";

	date->AdjustTimeZone(LNTIMEZONE_GMT);
	date->GetDate(&month, &day, &year);

	if (date->IsTimeDefined()) {
		date->GetTime(&hour, &minute, &second, &hundredth);
	}
	else {
		hour = 0;
		minute = 0;
		second = 0;
		hundredth = 0;
	}
	if (month < 10) {
		em = "0";
	}
	if (day < 10) {
		ed = "0";
	}
	if (hour < 10) {
		eH = "0";
	}
	if (minute < 10) {
		eM = "0";
	}
	if (second < 10) {
		es = "0";
	}

	isodate.append(std::to_string(year));
	isodate.append("-");
	isodate.append(em);
	isodate.append(std::to_string(month));
	isodate.append("-");
	isodate.append(ed);
	isodate.append(std::to_string(day));
	isodate.append("T");
	isodate.append(eH);
	isodate.append(std::to_string(hour));
	isodate.append(":");
	isodate.append(eM);
	isodate.append(std::to_string(minute));
	isodate.append(":");
	isodate.append(es);
	isodate.append(std::to_string(second));
	isodate.append(".");
	isodate.append(std::to_string(hundredth));
	isodate.append("Z");
	
	type = 1;
	stringValue = isodate;	
}