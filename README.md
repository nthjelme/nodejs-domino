# Node.js driver for NSF



## Install
    npm install domino-nsf

## Requirements
The domino-nsf package is currently windows only. The binaries has been build for Win32 and tested with Node 8.9.4, 32bit. 

**The Notes program folder needs to be added to the system PATH.**

## Linux

If you want to run on Linux, you'll need to build it from source. Check out development section for more..






## Usage

### Async API

```js
const domino = require('domino-nsf')();

let doc = {
  "FullName":"John Smith",
  "tags":["test","test2"],
  "age":33,
  "Form":"Person"
};

const db = domino.use('database.nsf');

db.get("documentUNID",function(error,document) {
	console.log("document",document);
});

db.insert(doc,function(error,document) {
	// returns the saved document
	console.log("document",document);
});

db.makeResponse(doc,parentDoc,function(err,res) {
	
});

db.view({view:"People",category:""},function(err,view) {
	  console.log("view result",view);
});

db.search("SELECT *", functoin(err,results) {
	//returns the search results
});

db.del("documentUNID",function(error,result) {
	console.log("result",result);
});


// to end session call
domino.termSession(); 
```
### View parameters
```js
{
  view: "the view name",
  max: "number, max entries to get"
  category: "the category to get"
  findByKey: "the key to search by"
  exact: true/false, exact or partial match when using findByKey
}
```

### Synchronous API
```js
const domino = require('domino-nsf')();

// you must run sinitThread before calling any notes api.
domino.sinitThread();
let db = domino.openDatabase('test.nsf');
let note = db.openNotesNote();
note.setItemText('Form','Test');
note.setItemText('Subject','Hello World!');

// save the note
note.updateNote();

// close the note and db
note.close();
db.close();


// terminate thread before exiting
domino.stermThread();

```

### Avaliable methods
#### Domino object

    sinitThread()
init the notes session/thread

    stermThread()
terminate the notes thread

    createDatabase('server!!path/databasename.nsf')
create a new **database**, on given serve and path. If path is omitted, localhost is used.   
Returns a **database** object.

    openDatabase('server!!path/databasename.nsf')
Opens a database.  
Returns a **database** object.
 
    deleteDatabase('server!!path/databasename.nsf');

#### Database object
    openNotesNote('unid')
Opens a Notes note by *UNID*.  
Returns a **note** object. 

    createNotesNote()
Creates a new note in the database.  
Returns a Notes object.

    getDatabaseName()
Return the database name / title.

    close()
Closes the database handle.

#### Notes object
    getItemText('itemName')
returns the items value as a string, returns empty string if item does not exists.

    getItemNumber('itemName')
returns the items value as a number

    getItemDate('itemName')

return item date value as js date.

    getItemValue('itemName')
returns the item value as a text,number,text array or date depending on type.

    hasItem('itemName')
returns true/false if note has item.

    getUNID()
returns the note UNID

    updateNote()
saves the note to database.

    setItemText('itemName','string')
set a string value to an item. If the item exists, it will replace the item value.

    setItemDate('itemName', date)
set a Date object value to an item

    setItemNumber('itemName,number)
set a number value to an item. If the item exists, it will replace the item value.

    setItemValue('itemName', value);
set an value to an item, value can be text,number,text array or js Date object

    appendItemValue('itemName','string')
append a string value to an existing text array.

    deleteItem('itemName')
delete an item from a note.  
Returns true if item was deleted.

    close()
Close the note handle. After calling close, you cannot call any methods on the current **Note** object.

## Development and Contribution

### Local Development Windows
To build the addon, you need the 
* Domino C API
* Nan for Node.js.
* Microsoft VisualStudio 2015/2017 or by using using Microsoft's windows-build-tools
* [node-gyp](https://github.com/nodejs/node-gyp)


#### Configuring enviroment for node-gyp build
You must set these environment variables before you build the addon

NOTES_INCLUDE must contain: 
* the C API header files

NOTES_LIB must contain:
* the path to the Notes C library folder

### Linux/Docker
To set up a development enviroment using Docker, you can use this 
[Dockerfile]("https://github.com/nthjelme/nodejs-domino/blob/master/docker/Dockerfile")


#### Configuring and building
    node-gyp configure
..and build..  

    node-gyp build


