#include <string>
#include <vector>
#include "nan.h"

using namespace std;
using v8::Local;
using v8::Object;

class DocumentItem {
public:
	DocumentItem() {
		stringValue = ""; type = 0; name = ""; numberValue = 0.0; vectorStrValue= std::vector<char *>();
	}
	~DocumentItem() {
		printf("destructor..\n");
		if (name) {
			printf("delete name");
			delete name;
		}
		vectorStrValue.clear();
	}
	DocumentItem(char * n, string d) {
		stringValue = d;
		type = 1;
		name = n;
	}
	DocumentItem(char* n, double d) {
		numberValue = d;
		type = 2;
		name = n;
	}
	DocumentItem(char * n, double d, double r) {
		numberValue = d;
		type = r;
		name = n;
	}
	DocumentItem(char* n, std::vector<char *> d) {
		vectorStrValue = d;
		type = 4;
		name = n;
	}

	string stringValue;
	string headerValue;
	double numberValue;
	std::vector<char*> vectorStrValue;
	char * name;
	double type;
};

void pack_document(v8::Local<v8::Object> & target, std::vector<DocumentItem *>);
std::vector<DocumentItem*> unpack_document(v8::Local<v8::Object> & document);
