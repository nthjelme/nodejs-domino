#include "DataHelper.h"
#include <osmisc.h>

void DataHelper::GetAPIError(STATUS api_error, char * error_text)
{
	STATUS  string_id = ERR(api_error);
	WORD    text_len;
	
	/* Get the message for this IBM C API for Notes/Domino error code
	from the resource string table. */

	text_len = OSLoadString(NULLHANDLE,
		string_id,
		error_text,
		200);
	return;
}

void DataHelper::ToUNID(const char *unidStr, UNID * Unid) {
	if (strlen(unidStr) == 32) {		
		char unid_buffer[33];
		strncpy(unid_buffer, unidStr, 32);
		unid_buffer[32] = '\0';
		if (strlen(unid_buffer) == 32)
		{
			/* Note part second, reading backwards in buffer */
			Unid->Note.Innards[0] = (DWORD)strtoul(unid_buffer + 24, NULL, 16);
			unid_buffer[24] = '\0';
			Unid->Note.Innards[1] = (DWORD)strtoul(unid_buffer + 16, NULL, 16);
			unid_buffer[16] = '\0';

			/* DB part first */
			Unid->File.Innards[0] = (DWORD)strtoul(unid_buffer + 8, NULL, 16);
			unid_buffer[8] = '\0';
			Unid->File.Innards[1] = (DWORD)strtoul(unid_buffer, NULL, 16);
		}
	}
}