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

#ifndef NOTES_DOCUMENT_H_
#define NOTES_DOCUMENT_H_

#include <nan.h>
#include "global.h"

NAN_METHOD(closeNote);
NAN_METHOD(getNotesNote);
NAN_METHOD(createNotesNote);
NAN_METHOD(updateNote);
NAN_METHOD(getNoteUNID);
NAN_METHOD(setItemText);
NAN_METHOD(getItemText);
NAN_METHOD(setItemNumber);
NAN_METHOD(getItemNumber);
NAN_METHOD(getItemDate);
NAN_METHOD(setItemValue);
NAN_METHOD(getItemValue);
NAN_METHOD(hasItem);
NAN_METHOD(setAuthor);
NAN_METHOD(appendItemTextList);
NAN_METHOD(getMimeItem);
NAN_METHOD(deleteItem);

void nsfItemSetText(unsigned short nHandle, const char * itemName, const char * itemValue);
void nsfGetItemText(unsigned short nHandle, const char * itemName,char *value);
void nsfItemSetNumber(unsigned short nHandle, const char * itemName, const double * itemValue);
double nsfItemGetNumber(unsigned short nHandle, const char * itemName);
void nsfItemCreateTextList(unsigned short nHandle, const char * itemName, const char * itemValue);
void nsfItemAppendTextList(unsigned short nHandle, const char * itemName, const char * itemValue);
BOOL hasItem(unsigned short nHandle,const char * itemName);
STATUS nsfDeleteItem(unsigned short nHandle,const char * itemName);



#endif  // NOTES_DOCUMENT_H_