#include "DataHelper.h"
#include <osmisc.h>

void DataHelper::GetAPIError(STATUS api_error, char * error_text)
{
	printf("get api error\n");
	STATUS  string_id = ERR(api_error);
	WORD    text_len;
	
	/* Get the message for this IBM C API for Notes/Domino error code
	from the resource string table. */

	text_len = OSLoadString(NULLHANDLE,
		string_id,
		error_text,
		sizeof(error_text));
	return;

}