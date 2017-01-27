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

#ifndef DATA_HELPER_H_
#define DATA_HELPER_H_

#include <nan.h>
#include <ctime>
#include <time.h>
#include <iostream>
#include <map>
#include <iterator>
#include <vector>
#include <global.h>

#include "ItemValue.h"

using v8::Local;
using v8::Array;
using v8::Object;
using v8::Number;
using v8::Value;
using v8::String;
using Nan::New;
using Nan::Null;
using Nan::To;

class DataHelper {
public:
	static Local<Object> getV8Data(std::map <std::string, ItemValue> *doc);
	static Local<Array> getV8Data(std::vector <std::map<std::string, ItemValue>> viewResult);
	static char * DataHelper::GetAPIError(STATUS api_error);
};

#endif  // ITEM_VALUE_H_
