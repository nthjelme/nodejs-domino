var expect = require('chai').expect;
var dominoNsf = require('./index');
var domino = dominoNsf();

var db = domino.use({server:'',database:'test.nsf'});

var doc = {
	"Form":"Test",
	"Name": "Test",
	"date": new Date(0),
	"number": 10,
	"array": ["a","b","c"]
};		


describe('domino-nsf',function() {
	describe('save document',function() {
		var savedDocument = {};
		before(function(done) {		
			db.insert(doc,function(err,result) {
				if(err) done(err);
				savedDocument = result;
				done();
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
		it('shoud have a Name property that equals Test',function() {
			expect(savedDocument).to.have.property('Name').to.eql("Test");
		});
		it('shoud have a array property that equals ["a","b","c"]',function() {
			expect(savedDocument).to.have.property('array').to.eql(["a","b","c"]);
		});
		
	});
});