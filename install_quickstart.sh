#!/bin/bash
echo If you are lucky, this is it.
read 
mkdir deploy
cmake -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/deploy .
make
make install
