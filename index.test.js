var expect = require('chai').expect;
var dominoNsf = require('./index');
var domino = dominoNsf();

var db;
var savedDocumentUnid = "";
var doc = {
	"Form":"Test",
	"Name": "Test",
	"date": new Date(0),
	"number": 10,
	"array": ["a","b","c"]
};
var test_db = {
	"database":"nodejs_domino9.nsf",
	"title":"Test database",
	"server":""
}


describe('domino-nsf',function() {

	before(function() {
		domino.createDatabase(test_db,function(error, database) {
			if (error) {
				console.error(error);
			}
			db = domino.use(database);
		});
		
	});

	after(function() {
		domino.deleteDatabase(test_db,function(error,status) {
			if (error) {
				console.log("error deleting database,",error);
			}
		});
		domino.termSession();
	});

	describe('save document',function() {
		var savedDocument = {};
		before(function(done) {
			db.insert(doc,function(err,result) {
				if(err) {
					done(err);
				} else {
					savedDocument = result;
					savedDocumentUnid = result["@unid"];
					done();
				}
			});
		});
		it ('should create a document with unid with length of 32', function() {
			expect(savedDocument).to.have.property('@unid').with.length(32);
		});
		it ('should have a number property that equals 10', function() {
			expect(savedDocument).to.have.property('number').to.equal(10);
		});
		it('should have a date property that equals ', function() {
			expect(savedDocument).to.have.property('date').to.eql(new Date(0));
		});
		it('shoud have a Name property that equals "Test"',function() {
			expect(savedDocument).to.have.property('Name').to.eql("Test");
		});
		it('shoud have a array property that equals ["a","b","c"]',function() {
			expect(savedDocument).to.have.property('array').to.eql(["a","b","c"]);
		});
	});

	describe('get document', function() {
		var document = {};
		before(function(done) {
			db.get(savedDocumentUnid,function(err,result) {
				if (err) {
					done(err);
				} else {
					document = result;
					done();
				}
			});
		});

		it ('should create a document with unid with length of 32', function() {
			expect(document).to.have.property('@unid').with.length(32);
		});
		it ('should have a number property that equals 10', function() {
			expect(document).to.have.property('number').to.equal(10);
		});
		it('should have a date property that equals ', function() {
			expect(document).to.have.property('date').to.eql(new Date(0));
		});
		it('shoud have a Name property that equals "Test"',function() {
			expect(document).to.have.property('Name').to.eql("Test");
		});
		it('shoud have a array property that equals ["a","b","c"]',function() {
			expect(document).to.have.property('array').to.eql(["a","b","c"]);
		});

	});

	describe('search database', function() {
		var searchResults = [];
		before(function(done) {
			db.search("SELECT *",function(err,result) {
				if (err) {
					done(err);
				} else {
					searchResults = result;
					done();
				}
			});
		});

		it('expect a list of documents with length of 1',function() {
			expect(searchResults).to.have.lengthOf(1);
		});

	});

	 describe('open database and get name', function() {
		let db = {};
		let note = {};
		let newNote = {};
		before(function(done) {
		domino.sinitThread();
			db = domino.openDatabase(test_db.database);
			note = db.getNotesNote(savedDocumentUnid);
			done();
		});

		it('should have a name equals ' + test_db.title, function() {
			expect(db.getDatabaseName()).to.be.equal(test_db.title);
		});

		it('should have a note.Name equals to ' + doc.Name, function() {
			expect(note.getItemText("Name")).to.be.equal(doc.Name);
		});

		it('should have a note.Number equals to ' + doc.number, function() {
			expect(note.getItemNumber("number")).to.be.equal(doc.number);
		});

		it ('should return a number value', function() {
			expect(note.getItemValue("number")).to.be.equal(doc.number);
		});

		it ('should return a text value', function() {
			expect(note.getItemValue("Name")).to.be.equal(doc.Name);
		})

		it('should have a note.Date equals to ' + doc.date, function() {
			expect(new Date(note.getItemDate("date")).getTime()).to.be.equal(doc.date.getTime());
		});

		it('should return a date equals to ' + doc.date, function() {
			expect(note.getItemValue("date").toString()).to.be.equal(doc.date.toString());
		})

		it('should create a new note and get a handle', function() {
			newNote = db.createNotesNote();
			expect(newNote.handle).to.not.equal(0);
		});

		it('should create a item on newly created note', function() {
			newNote.setItemText("test", "test value");
			expect(newNote.getItemText("test")).to.be.equal("test value");
		});

		it('hasItem should return true', function() {
			expect(newNote.hasItem("test")).to.be.true;
			
		});

		it('deleted item should be empty', function() {
			newNote.deleteItem("test");
			expect(newNote.getItemText("test")).to.be.equal("");		
		});		

		it('should create a number item on newly created note', function() {
			newNote.setItemNumber("tall", 33.3);
			expect(newNote.getItemNumber("tall")).to.be.equal(33.3);
		});

		it('should create an array item on newly created note', function() {
			var text_list = ["Text1","Text2","Text3"];
			newNote.setItemValue("text_list",text_list);
			expect(newNote.getItemValue("text_list")).to.deep.equal(text_list);
		});

		it('should append a value to the array', function() {
			let text_list = ["Text1","Text2","Text3"];
			let textToAppend = "Text4";
			text_list.push(textToAppend);
			newNote.appendItemValue("text_list",textToAppend);
			expect(newNote.getItemValue("text_list")).to.deep.equal(text_list);

		});

		it('should create a date item', function() {
			let date = new Date();
			newNote.setItemDate("date_test",date);
			// notes does not save ms in dates, compare the date strings instead.
			expect(newNote.getItemDate("date_test").toString()).to.be.equal(date.toString());
		})
		
		it('should save the newnote and get the note unid', function() {
			newNote.updateNote();
			expect(newNote.getUNID()).to.have.lengthOf(32);
		});
	
		after(function(done) {
			note.close();
			db.close();
			domino.stermThread();
			done();
		})
	}); 
	
	describe('delete document', function() {
		var deleteResult = {};
		before(function(done) {
						db.del(savedDocumentUnid,function(err,result) {
				if (err) {
					done(err);
				} else {
					deleteResult = result;
					done();
				}
			});
		});
		it('should return status property that equals "deleted"',function() {
			expect(deleteResult).to.have.property('status').to.eql("deleted");
		});
	});
});
