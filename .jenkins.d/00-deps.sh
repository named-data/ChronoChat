#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

[[ -n $NODE_LABELS ]] || exit 0

if has OSX $NODE_LABELS; then
    brew update
    brew upgrade
    brew install boost pkg-config qt4
    brew cleanup
fi

if has Ubuntu $NODE_LABELS; then
    sudo apt-get update -qq -y
    sudo apt-get -qq -y install build-essential
    sudo apt-get -qq -y install libssl-dev libsqlite3-dev

    sudo apt-get -qq -y install libprotobuf-dev protobuf-compiler libevent-dev libcrypto++-dev
    sudo apt-get -qq -y install libboost-all-dev
    sudo apt-get -qq -y install qt5-default
fi
