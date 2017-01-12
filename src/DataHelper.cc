#include "DataHelper.h"


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
			int ii;
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

char * DataHelper::GetAPIError(STATUS api_error)
{
	STATUS  string_id = ERR(api_error);
	char    error_text[200];
	WORD    text_len;

	/* Get the message for this IBM C API for Notes/Domino error code
	from the resource string table. */

	text_len = OSLoadString(NULLHANDLE,
		string_id,
		error_text,
		sizeof(error_text));
	return error_text;


}

Local<Array> DataHelper::getV8Data(std::vector <std::map<std::string, ItemValue>> viewResult) {

	Local<Array> viewRes = New<Array>(viewResult.size());
	try {
		int j;
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
					int ii;
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
	}
	catch (LNSTATUS Lnerror) {
		char ErrorBuf[512];
		LNGetErrorMessage(Lnerror, ErrorBuf, 512);
		std::cout << "Handle Error:  " << ErrorBuf << std::endl;
	}
	return viewRes;

}