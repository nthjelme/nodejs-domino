module.exports = Domino;
var dominoDriver = require('./build/Release/addon');

function Domino() {
	dominoDriver.initSession();
	var dbObj = {};
	var termSession = function() {
		dominoDriver.termSession();
	};
	
	var createDatabase = function(db,callback) {
		dominoDriver.createDatabase(db,function(error,response) {
			callback(error,response);
		});
	};

	var deleteDatabase = function(db,callback) {
		dominoDriver.deleteDatabase(db,function(error,status) {
			callback(error,status);
		});
	};

	var sinitThread = function() {
		dominoDriver.sinitThread();
	}

	var stermThread = function() {
		dominoDriver.stermThread();
	}

	var openDatabase = function(databaseName) {
	
		var db = {handle:0};
		db.handle =  dominoDriver.openDatabase(databaseName);
		if (db.handle==0) {
			throw Error("Error opening database");
		} else {
			db.getDatabaseName = function() {
				return dominoDriver.getDatabaseName(db.handle);
			}

			db.close = function() {
				dominoDriver.closeDatabase(db.handle);
				db.handle=0;
			}

			var baseNote = {};
			baseNote.getItemText = function(itemName) {
				return dominoDriver.getItemText(this.handle,itemName);
			}
			baseNote.setItemText = function(itemName,value) {
				dominoDriver.setItemText(this.handle, itemName, value);
			}
			baseNote.getItemNumber = function(itemName) {
				return dominoDriver.getItemNumber(this.handle,itemName);
			}
			baseNote.setItemNumber = function(itemName,value) {
				return dominoDriver.setItemNumber(this.handle,itemName,value);
			}
			baseNote.getItemDate = function(itemName) {
				return dominoDriver.getItemDate(this.handle,itemName);
			}
			baseNote.setItemDate = function(itemName, date) {
				return dominoDriver.setItemDate(this.handle,itemName,date);
			}

			baseNote.getItemValue = function(itemName) {
				return dominoDriver.getItemValue(this.handle,itemName);
			}
			baseNote.hasItem = function(itemName) {
				return dominoDriver.hasItem(this.handle,itemName);
			}
			baseNote.deleteItem = function(itemName) {
				return dominoDriver.deleteItem(this.handle,itemName);
			}
			baseNote.setAuthor = function(itemName,value) {
				return dominoDriver.setAuthor(this.handle,itemName,value);
			}
			baseNote.setItemValue = function(itemName,value) {
				return dominoDriver.setItemValue(this.handle,itemName,value);
			}

			
			baseNote.setItemMime = function(itemName, header,body) {
				if (!header) {
					header = 'Content-Type: application/json';
				}
				return dominoDriver.setMimeItem(this.handle,itemName,body,header);
			}
		
			baseNote.getItemMime = function(itemName) {
				let mimeItem = {
					value: dominoDriver.getMimeItem(this.handle,itemName)
				}
				return mimeItem;
			}

			baseNote.appendItemValue = function(itemName, itemToAppend) {
				return dominoDriver.appendItemTextList(this.handle,itemName,itemToAppend);
			}

			baseNote.updateNote = function() {
				dominoDriver.updateNote(this.handle);
			}
			baseNote.close = function() {
				dominoDriver.closeNote(this.handle);
			}
			baseNote.getUNID = function() {
				return dominoDriver.getNoteUNID(this.handle);
			}

			db.createNotesNote = function() {
				var note = Object.create(baseNote);
				note.handle = dominoDriver.createNotesNote(db.handle);
				return note;
			}
			
			db.openNotesNote = function(unid) {
				var note = Object.create(baseNote);
				note.handle = dominoDriver.getNotesNote(db.handle,unid);
				
				return note;
			}
		}
		return db;
	}
	
	var use = function(db) {
		var localDb = {database: db};

		var replicate = function(callback) {
			callback({error:"Not implemented"},undefined);
			/*dominoDriver.replicateAsync(db,function(error,result) {
				callback(error,result);
			});*/
		}

		var get = function(unid,callback) {
			dominoDriver.getDocumentAsync(localDb,unid,function(error,document) {
				if (document) {
					document.getResponses = function(responseCallback) {
						dominoDriver.getResponseDocumentsAsync(localDb,this["@unid"], function(err,res) {
							responseCallback(err,res);
						});
					};
					document.makeResponse = function(parent,responseCallback) {
						dominoDriver.makeResponseDocumentAsync(localDb,this["@unid"], parent["@unid"], function(error,result) {
							responseCallback(error,result);
						});
					}
					document.save = function(documentCallback) {
						dominoDriver.saveDocumentAsync(localDb,this,function(error,document) {
							documentCallback(error,document);
						});

					}
				}
				callback(error,document);
			});
		}

		var insert = function(document,callback) {
			dominoDriver.saveDocumentAsync(localDb,document,function(error,document) {
				callback(error,document);
			});
		};

		var deleteFn = function(unid,callback) {
			dominoDriver.deleteDocumentAsync(localDb,unid,function(error,result) {
				callback(error,result);
			});
		};

		var makeResponse = function(doc,parent,callback) {
			dominoDriver.makeResponseDocumentAsync(localDb,doc["@unid"], parent["@unid"], function(error,result) {
					callback(error,result);
			});
		};

		var getResponses = function(doc,callback) {
			callback({error: "Not implemented"},undefined)
			/*
			dominoDriver.getResponseDocumentsAsync(localDb,doc["@unid"], function(error,result) {
					callback(error,result);
			});*/
		};

		var view = function(view,callback) {
			dominoDriver.getViewAsync(localDb,view,function(err,result) {
				callback(err,result);
			});
		};

		var search = function(searchFormula,callback) {
			dominoDriver.searchNsfAsync(localDb,searchFormula,function(err,result) {
				callback(err,result);
			});
		}

		localDb.get = get;
		localDb.insert = insert;
		localDb.del = deleteFn;
		localDb.view = view;
		localDb.replicate = replicate;
		localDb.makeResponse = makeResponse;
		localDb.search = search;

		return localDb;

	};
	process.on('SIGINT', function() {
		dominoDriver.termSession();  
		process.exit();
	});
	
	dbObj.use = use;
	dbObj.termSession = termSession;
	dbObj.createDatabase = createDatabase;
	dbObj.deleteDatabase = deleteDatabase;
	dbObj.openDatabase = openDatabase;
	dbObj.sinitThread = sinitThread;
	dbObj.stermThread = stermThread;
	
	return dbObj;
};
