{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cc",        
		"src/document_async.cc",
		"src/save_document_async.cc",
		"src/delete_document_async.cc",
		"src/view_async.cc",
		"src/ItemValue.cc",
		"src/DataHelper.cc",
		"src/getresponse_documents.cc",
		"src/makeresponse_document.cc",
		"src/replicate_database_async.cc"
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")","<!(echo %NOTES_INCLUDE%)"],
	  "libraries": [ "<!(echo %NOTES_LIB%)/mswin32/notes.lib","<!(echo %NOTES_LIB%)/mswin32/notescpp.lib"],
	  "defines": [ "W32" ],	  
    }
  ]
}
