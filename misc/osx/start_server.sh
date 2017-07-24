#!/bin/sh
echo "Edit this script to change the path to Spearmint's dedicated server executable."
echo "Set the sv_dlURL setting to a url like http://yoursite.com/spearmint_path for clients to download extra data."

# sv_dlURL needs to have quotes escaped like \"http://yoursite.com/spearmint_path\" or it will be set to "http:" in-game.

/Applications/Spearmint/Spearmint.app/Contents/MacOS/spearmint-server +set sv_public 1 +set sv_allowDownload 1 +set sv_dlURL \"\" "$@"
