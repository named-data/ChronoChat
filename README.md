ChronoChat
==========

ChronoChat is a multiparty chat application that demostrates our synchronization primitive that we call ChronoSync.

Note that after you click to close ChronoChat, it will keep running on your system tray. To restore it to normal size window, you have to click on the system tray icon (normally on the upper right corner of your screen). Clicking on the dock won't work for now and is still on the to-do list (because I'm using qt for gui, not the native Cocoa framework).

## Known issues
---------------

1. When you switch to a new room, you'll temporarily see yourself in two nodes for a minute or so. It won't affect others, just yourself. Hopefully it's not so disturbing.
2. Sometimes you may not get the most up-to-date chat history.

## For those who wants (or is forced to) compile from source code
-----------------------------------------------------------------

### Compilation steps for OSX

1. Install MacPorts, if not yet installed (http://www.macports.org/), configure [NDN ports repository](http://named-data.net/doc/NFD/current/FAQ.html#how-to-start-using-ndn-macports-repository-on-osx) and install NFD if you don't have it yet.

        sudo port install nfd
        sudo nfd-start

2. Install ChronoChat dependencies

        sudo port install pkgconfig protobuf-cpp boost qt4-mac

3. Fetch source code with submodules

        git clone --recursive git://github.com/named-data/ChronoChat

If you already cloned repository, you can update submodules this way:

        git submodule update --init

4. Configure and install ChronoSync

        cd ChronoChat/ChronoSync
        ./waf configure
        ./waf
        sudo ./waf install

5. Configure and build ChronoChat

        cd ..
        PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig ./waf configure
        ./waf

Congratulations! build/ChronoChat.app is ready to use (on a Mac).

### Compilation steps for Ubuntu 12.04, 13.10, 14.04

1. Configure [NDN PPA repository](http://named-data.net/doc/NFD/current/FAQ.html#how-to-start-using-ndn-ppa-repository-on-ubuntu-linux) and install NFD if you don't have it yet.

        sudo apt-get install nfd

2. Install ChronoChat dependencies

        sudo apt-get install libprotobuf-dev protobuf-compiler libevent-dev
        sudo apt-get install libboost1.48-all-dev
        sudo apt-get install qt4-dev-tools

3. Fetch source code with submodules

        git clone --recursive git://github.com/named-data/ChronoChat

If you already cloned repository, you can update submodules this way:

	    git submodule update --init

4. Configure and install ChronoSync

        cd ChronoChat/ChronoSync
        ./waf configure
        ./waf
        sudo ./waf install
        sudo ldconfig

5. Configure and build ChronoChat

        cd ..
        ./waf configure
        ./waf

Congratulations! build/ChronoChat is ready to use.  Do not forget to start ccnd and configure FIB before using ChronoChat.
