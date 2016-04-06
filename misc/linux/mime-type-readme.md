How to install Spearmint demo file mime type and open the demos from file browsers.

## File type

Identify Spearmint demos as "Spearmint demo" or "Video" (new [nautilus basic type display](https://mail.gnome.org/archives/commits-list/2012-September/msg07135.html) behavior) in file browsers by doing the following;

    cd misc/linux

For current user only;

    xdg-mime install x-spearmint-demo.xml
    update-mime-database ~/.local/share/mime

or system wide using;

    sudo xdg-mime install --mode system x-spearmint-demo.xml
    sudo update-mime-database /usr/share/mime

I'm not whether running `update-mime-database` is required.


## Open demos
The desktop file for being able to open the demos must be installed system-wide?
Requires spearmint executable such as /usr/local/bin/spearmint for the option to appear in nautilus.

    sudo desktop-file-install spearmint-demo.desktop
    sudo xdg-mime default spearmint-demo.desktop application/x-spearmint-demo


## Icon

Hmm... was it something like this?

    xdg-icon-resource install --size 128 --context mimetypes application-x-spearmint-demo.png

I think I had a icon working at some point...


## Developer notes
(Originally written for Debian 7, GNOME 3 fallback)

If update x-spearmint-demo.xml using "xdg-mime install x-spearmint-demo.xml", must `killall -9 nautilus` to display new comment.

Validate desktop file using

    desktop-file-validate spearmint-demo.desktop

