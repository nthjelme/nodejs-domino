#!/bin/bash

serverID=/local/notesdata/server.id

if [ ! -f "$serverID" ]; then
    /opt/ibm/domino/bin/server -listen 1352
else
    /opt/ibm/domino/rc_domino_script start
    /bin/bash
fi
