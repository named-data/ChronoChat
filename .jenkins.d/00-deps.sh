#!/usr/bin/env bash
set -ex

if has OSX $NODE_LABELS; then
    FORMULAE=(boost openssl pkg-config qt)
    if has OSX-10.13 $NODE_LABELS || has OSX-10.14 $NODE_LABELS; then
        FORMULAE+=(python)
    fi

    if [[ -n $GITHUB_ACTIONS ]]; then
        # Don't waste time upgrading all pre-installed packages
        for FORMULA in "${FORMULAE[@]}"; do
            brew list --versions "$FORMULA" || brew install "$FORMULA"
        done

        # Ensure /usr/local/opt/openssl exists
        brew reinstall openssl

        # Homebrew qt is keg-only, force symlinking it into /usr/local
        brew link --force qt
    else
        brew update
        brew upgrade
        brew install "${FORMULAE[@]}"
        brew cleanup
    fi

elif has Ubuntu $NODE_LABELS; then
    sudo apt-get -qq update
    sudo apt-get -qy install build-essential pkg-config python3-minimal \
                             libboost-all-dev libssl-dev libsqlite3-dev \
                             qt5-default
fi
