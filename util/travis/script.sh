#!/bin/bash -e

mkdir -p travisbuild
cd travisbuild
cmake -DENABLE_LEVELDB=1 -DUSE_CXX11=$CXX11 ..
make -j2
