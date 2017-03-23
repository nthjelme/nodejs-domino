{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cc",        
		"src/document_async.cc",
		"src/save_document_async.cc",
		"src/makeresponse_document.cc",
		"src/delete_document_async.cc",
		"src/view_async.cc",		
		"src/DocumentItem.cc",
		"src/DataHelper.cc"
      ],
      "conditions": [
	["OS==\"linux\"",{
	  "include_dirs": ["<!(node -e \"require('nan')\")","/opt/ibm/domino/notesapi/include"],	  
	  "libraries": [ "/opt/ibm/domino/notes/latest/linux/libnotes.so"],
	  "defines": [ "GCC3", "GCC4","ND64","NDUNIX64", "UNIX", "LINUX64","LINUX", "LINUX86","W32","LINUX86_64", "W","GCC_LBLB_NOT_SUPPORTED","LONGIS64BIT","DTRACE", "PTHREAD_KERNEL" "_REENTRANT", "USE_THREADSAFE_INTERFACES","_POSIX_THREAD_SAFE_FUNCTIONS","HANDLE_IS_32BITS", "HAS_IOCP", "HAS_BOOL", "HAS_DLOPEN", "USE_PTHREAD_INTERFACES", "LARGE64_FILES", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE","PRODUCTION_VERSION", "OVERRIDEDEBUG"],
	  "cflags_cc!": [
		"-fno-exceptions","-fPIC","-c", "-m64","-fno-strict-aliasing","-std=c++11","-felide-constructors"
	  ],
  	  "cflags_c!": [
		"-fno-exceptions","-fPIC","-c", "-m64","-fPIC","-fno-strict-aliasing","-std=c++11","-fexceptions"
	  ],
	  "ldflags":["-Wformat", "-Wall", "-Wcast-align", "-Wconversion","-shared","-Bsymbolic","-ldl", "-lrt", "-lm", "-lstdc++","-lpthread", "-lc", "-lresolv", "-lc","-rdynamic","-fexceptions" ]
	}],
	["OS==\"win\"", {
	  "include_dirs": ["<!(node -e \"require('nan')\")","<!(echo %NOTES_INCLUDE%)"],
	  "libraries": [ "<!(echo %NOTES_LIB%)/mswin32/notes.lib"],
	  "defines": [ "W32" ],	  	
	}]

      ]
    }
  ]
}
