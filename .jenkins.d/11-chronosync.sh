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
sudo ./waf -j1 --color=yes distclean
./waf -j1 --color=yes configure
./waf -j1 --color=yes build
sudo ./waf install -j1 --color=yes
sudo ldconfig || true
popd >/dev/null
