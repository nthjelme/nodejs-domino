#include "DataHelper.h"
#include <osmisc.h>

using std::size_t;

Local<Object> DataHelper::getV8Data(std::map <std::string, ItemValue> *doc) {
	Local<Object> resDoc = Nan::New<Object>();
	std::map<std::string, ItemValue>::iterator it;
	for (it = doc->begin(); it != doc->end(); it++)
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

	
	return resDoc;
}

void DataHelper::GetAPIError(STATUS api_error, char * error_text)
{
	STATUS  string_id = ERR(api_error);
	WORD    text_len;
	
	/* Get the message for this IBM C API for Notes/Domino error code
	from the resource string table. */

	text_len = OSLoadString(NULLHANDLE,
		string_id,
		error_text,
		sizeof(error_text));
	return;

}

Local<Array> DataHelper::getV8Data(std::vector <std::map<std::string, ItemValue>> viewResult) {

	Local<Array> viewRes = New<Array>(viewResult.size());
	
	size_t j;
	for (j = 0; j < viewResult.size(); j++) {
		std::map<std::string, ItemValue> doc = viewResult[j];
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
					size_t ii;
					Local<Array> arr = New<Array>(value.vectorStrValue.size());
					Nan::Set(arr, 0, Nan::New<String>("").ToLocalChecked());
					for (ii = 0; ii < value.vectorStrValue.size(); ii++) {
						Nan::Set(arr, ii, Nan::New<String>(value.vectorStrValue[ii]).ToLocalChecked());
					}
					Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), arr);

				}
				else if (value.type == 3) {
					Nan::Set(resDoc, New<v8::String>(it->first).ToLocalChecked(), New<v8::Date>(value.dateTimeValue).ToLocalChecked());
				}
			}
			Nan::Set(viewRes, j, resDoc);
		}

	return viewRes;

}