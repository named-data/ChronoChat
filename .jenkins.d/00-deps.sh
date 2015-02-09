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

    if has Ubuntu-12.04 $NODE_LABELS; then
        sudo apt-get install -qq -y libboost1.48-all-dev
    else
        sudo apt-get install -qq -y libboost-all-dev
    fi

    sudo apt-get install -qq -y qt4-dev-tools protobuf-compiler libprotobuf-dev libqt4-sql-sqlite
fi
