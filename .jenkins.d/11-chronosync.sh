#!/usr/bin/env bash
set -x
set -e

sudo rm -Rf /usr/local/include/ChronoSync
sudo rm -f /usr/local/lib/libChronoSync*
sudo rm -f /usr/local/lib/pkgconfig/ChronoSync*


# Update ChronoSync
git submodule init
git submodule update
pushd ChronoSync >/dev/null
sudo ./waf --color=yes distclean
./waf --color=yes configure
./waf --color=yes build
sudo ./waf install --color=yes
sudo ldconfig || true
popd >/dev/null
