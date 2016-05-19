{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "addon.cc",        
		"document_async.cc",
		"save_document_async.cc",
		"delete_document_async.cc",
		"view_async.cc",
		"ItemValue.cc",
		"replicate_database_async.cc"
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")","<!(echo %NOTES_INCLUDE%)"],
	  "libraries": [ "<!(echo %NOTES_LIB%)/mswin32/notes.lib","<!(echo %NOTES_LIB%)/mswin32/notescpp.lib"],
	  "defines": [ "W32" ],	  
    }
  ]
}
