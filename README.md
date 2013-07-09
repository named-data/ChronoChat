ChronoChat
==========

ChronoChat is a multiparty chat application that demostrates our synchronization primitive that we call ChronoSync.

Note that after you click to close ChronoChat, it will keep running on your system tray. To restore it to normal size window, you have to click on the system tray icon (normally on the upper right corner of your screen). Clicking on the dock won't work for now and is still on the to-do list (because I'm using qt for gui, not the native Cocoa framework).

## Known Issues
---------------

1. When you switch to a new room, you'll temporarily see yourself in two nodes for a minute or so. It won't affect others, just yourself. Hopefully it's not so disturbing.
2. Sometimes you may not get the most up-to-date chat history.

## For those who wants (or is forced to) compile from source code
-----------------------------------------------------------------

You need pkg-config, protobuf, boost, and qt to compile this app. All of them can be installed through macports.
The following command should do it:
$ sudo port install pkg-config protobuf-cpp boost qt4-mac 

To compile this app:

1. in the top directory, check out the "sync" submodule

	$ git submodule update --init

2. go to sync directory, do:

	$ ./waf configure
	$ ./waf
	$ sudo ./waf install

3. back to the top directory, set up environment for pkg-config, make sure libsync.pc is searchable, this is mine:

	$ export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig

4. compile Chronos:

	$ ./waf configure
	$ ./waf

Congratulations! build/ChronoChat.app is ready to use (on a Mac).

