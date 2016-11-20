module.exports = Domino;
var dominoDriver = require('./build/Release/addon');

function Domino() {
	dominoDriver.initSession();
	var dbObj = {};
	var termSession = function() {
		dominoDriver.termSession();
	};

	var use = function(db) {
		var localDb = db;

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

		localDb.get = get;
		localDb.insert = insert;
		localDb.del = deleteFn;
		localDb.view = view;
		localDb.replicate = replicate;
		localDb.makeResponse = makeResponse;

		return localDb;

	};
	
	process.on('SIGINT', function() {
		dominoDriver.termSession();  
		process.exit();
	});
	
	dbObj.use = use;
	dbObj.termSession = termSession;

	return dbObj;
};
