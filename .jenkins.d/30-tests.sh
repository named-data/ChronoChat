#!/usr/bin/env bash
set -x
set -e

# Prepare environment
sudo rm -Rf ~/.ndn

./build/unit-tests -l test_suite
