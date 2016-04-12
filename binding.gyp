{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "addon.cc",        
		"document_async.cc",
		"save_document_async.cc",
		"view_async.cc",
		"ItemValue.cc"
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")","c:/notesapi/include"],
	  "libraries": [ "C:/notesapi/lib/mswin32/notes.lib","C:/notesapi/lib/mswin32/notescpp.lib" ],
	  "defines": [ "W32" ],
	  
    }
  ]
}
