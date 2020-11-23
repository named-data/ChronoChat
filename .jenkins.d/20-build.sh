#!/usr/bin/env bash
set -x
set -e

# Cleanup
sudo ./waf --color=yes distclean

# Configure/build in optimized mode with tests
./waf --color=yes configure --debug --with-tests
./waf --color=yes build
