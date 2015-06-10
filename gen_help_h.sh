#!/bin/bash

#generate invocation help string in help.h from 
#manpage
set -e

echo "char help_txt[]={\"\\" >./help.h
MANWIDTH=70 man ./vcdmerge.man |sed 's,	,\\t,g;s,\",\\",g;s,\(.*\),\1\\n\\,g' >> ./help.h
echo "\"};" >>./help.h
