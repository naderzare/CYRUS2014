#!/bin/bash
set -e
cd Lib
make -j8
make install
cd ../Agent
make -j8

