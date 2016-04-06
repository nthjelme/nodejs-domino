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

#ifndef ITEM_VALUE_H_
#define ITEM_VALUE_H_

#include <nan.h>
#include <lncppapi.h>

class ItemValue {
public:
	ItemValue() {}
	ItemValue(std::string s) {
		type = 1;
		stringValue = s;
	}
	ItemValue(double n) {
		type = 0;
		numberValue = n;
	}
	ItemValue(std::vector<std::string> vecS) {
		type = 2;
		vectorStrValue = vecS;
	}	
	ItemValue(LNItem *item);	
	void TextItem(LNItem *item);
	void DateValue(LNDatetime * date);

	int type;
	double numberValue;
	std::string stringValue;
	std::vector<std::string> vectorStrValue;	
};

#endif  // ITEM_VALUE_H_
