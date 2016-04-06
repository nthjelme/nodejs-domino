{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "addon.cc",        
        "async.cc",
		"document_async.cc",
		"save_document_async.cc",
		"view_async.cc",
		"ItemValue.cc"
      ],
      "include_dirs": ["<!(node -e \"require('nan')\")"]	  
    }
  ]
}
