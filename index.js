module.exports = Domino;
var dominoDriver = require('./build/Release/addon');

function Domino() {
	dominoDriver.initSession();
	var dbObj = {};
	var use = function(db) {
		var localDb = db;
		
		var replicate = function(callback) {
			callback("Not implemented",undefined);
			/*dominoDriver.replicateAsync(db,function(error,result) {
				callback(error,result);
			});*/
		}
		
		var get = function(unid,callback) {			
			dominoDriver.getDocumentAsync(localDb,unid,function(error,document) {
				callback(error,document);
			});
		}
		
		var insert = function(document,callback) {
			dominoDriver.saveDocumentAsync(localDb,document,function(error,document) {
				callback(error,document);
			});
		};
		
		var deleteFn = function(unid,callback) {
			dominoDriver.deleteDocumentAsync(unid,function(error,result) {
				callback(error,result);
			});
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
		return localDb;
			
	};
	dbObj.use = use;
	
	return dbObj;
};
