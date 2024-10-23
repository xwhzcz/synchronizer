#! /bin/bash

[ -e build ] & rm -r build
mkdir build
cd build && cmake .. -DDEBUG_INFO=ON -DCMAKE_BUILD_TYPE=Release && make