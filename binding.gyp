{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cc",        
		"src/document_async.cc",
		"src/save_document_async.cc",
		"src/view_async.cc",
		"src/ItemValue.cc",
		"src/DataHelper.cc"		
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")","<!(echo %NOTES_INCLUDE%)"],
	  "libraries": [ "<!(echo %NOTES_LIB%)/mswin32/notes.lib"],
	  "defines": [ "W32" ],	  
    }
  ]
}
