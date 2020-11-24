#!/usr/bin/env bash
set -ex

if has OSX $NODE_LABELS; then
    FORMULAE=(boost openssl pkg-config qt)
    if has OSX-10.13 $NODE_LABELS || has OSX-10.14 $NODE_LABELS; then
        FORMULAE+=(python)
    fi

    if [[ -n $GITHUB_ACTIONS ]]; then
        # Homebrew doesn't have cryptopp packages, so build from source
        git clone https://github.com/weidai11/cryptopp/
        cd cryptopp
        make -j4
        make install
        cd ..

        # Travis images come with a large number of pre-installed
        # brew packages, don't waste time upgrading all of them
        for FORMULA in "${FORMULAE[@]}"; do
            brew list --versions "$FORMULA" || brew install "$FORMULA"
        done

        brew link qt --force
    else
        brew update
        brew upgrade
        brew install "${FORMULAE[@]}"
        brew cleanup
    fi

elif has Ubuntu $NODE_LABELS; then
    sudo apt-get -qq update
    sudo apt-get -qy install g++ pkg-config python3-minimal \
                             libboost-all-dev libssl-dev libsqlite3-dev \
                             libcrypto++-dev qt5-default
fi