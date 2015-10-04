#!/bin/sh
echo "Edit this script to change the path to Spearmint's dedicated server executable.\n Set the sv_dlURL setting to a url like http://yoursite.com/spearmint_path for clients to download extra data"
/Applications/Spearmint/Spearmint.app/Contents/MacOS/spearmint-server +set dedicated 2 +set sv_allowDownload 1 +set sv_dlURL "" +set com_hunkmegs 64 "$@"
