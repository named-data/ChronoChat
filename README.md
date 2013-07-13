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

### Compilation steps for OSX

1. Install MacPorts, if not yet installed (http://www.macports.org/) and configure NDN ports repository (https://github.com/named-data/ccnx/wiki/Using-ccnx-with-macports).  If your Macports are installed in `/opt/local`, add the following line at the end of `/opt/local/etc/macports/sources.conf` before the default port repository:

        rsync://macports.named-data.net/macports/

2. Update port definitions and install (+load) required packages

        $ sudo port selfupdate

        # Install and CCNx
        sudo port install ccnx
        sudo port load ccnx

        # Install ChronoChat dependencies
        sudo port install pkgconfig protobuf-cpp boost qt4-mac

3. Fetch source code with submodules

         git clone --recursive git://github.com/named-data/ChronoChat

If you already cloned repository, you can update submodules this way:

	git submodule update --init

4. Configure and install libsync

        cd ChronoChat/sync
	./waf configure
	./waf
	sudo ./waf install

5. Configure and build ChronoChat

        cd ..
        PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig ./waf configure
        ./waf

Congratulations! build/ChronoChat.app is ready to use (on a Mac).

### Compilation steps for Ubuntu 12.04

1. Install dependencies 

        # General dependencies (build tools and dependencies for CCNx)
        sudo apt-get install git libpcap-dev libxml2-dev make libssl-dev libexpat-dev g++ pkg-config

        # ChronoSync/ChronoChat dependencies
        sudo apt-get install libprotobuf-dev protobuf-compiler libevent-dev
        sudo apt-get install libboost1.48-all-dev
        sudo apt-get install qt4-dev-tools

**NOTE** Only 1.48 version of boost libraries should be installed from packages.  Since Ubuntu 12.04 ships with two versions, please make sure that 1.46 is not present, otherwise result is not guaranteed.

2. Download and install NDN fork of CCNx software

        git clone git://github.com/named-data/ccnx
        cd ccnx
        ./configure
        make
        sudo make install

3. Fetch source code with submodules

         git clone --recursive git://github.com/named-data/ChronoChat

If you already cloned repository, you can update submodules this way:

	git submodule update --init

4. Configure and install libsync

        cd ChronoChat/sync
	./waf configure
	./waf
	sudo ./waf install
        sudo ldconfig

5. Configure and build ChronoChat

        cd ..
        ./waf configure
        ./waf

Congratulations! build/ChronoChat is ready to use.  Do not forget to start ccnd and configure FIB before using ChronoChat.


