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
	"database":"nodejs_domino.nsf",
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
