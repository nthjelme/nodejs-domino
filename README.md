# Node.js driver for NSF



## Install
    npm install domino-nsf

## Requirements
The domino-nsf package is currently windows onl. The binaries has been build for Win32 and tested with Node 6.1.0, 32bit. 

The Notes program folder needs to be added to the system PATH.

## Linux

If you want to run on Linux, you'll need to build it from source. Check out development section for more..






## Usage

```js
var Domino = require('domino-nsf');
var domino = Domino();

var doc = {
  "FullName":"John Smith",
  "tags":["test","test2"],
  "age":33,
  "Form":"Person"
};

var db = domino.use({server:'',database:'database.nsf'});

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


