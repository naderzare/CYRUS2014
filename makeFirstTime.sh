#!/bin/bash
set -e
cd Lib
autoreconf -i
automake --add-missing
./configure --prefix=`pwd|sed 's/...$//'`/Agent/Lib
make -j8
make install 
cd ../Agent
autoreconf -i
automake --add-missing
./configure --with-librcsc=`pwd`/Lib/ CXXFLAGS='-std=c++03'
make -j8

