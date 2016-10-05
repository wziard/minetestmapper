#!/bin/bash -e

CXX=g++-6
[ $CC == "clang" ] && CXX=clang-3.8
export CXX


mkdir -p travisbuild
cd travisbuild

cmake \
	-DENABLE_LEVELDB=1 -DUSE_CXX11=$CXX11 \
	-DLEVELDB_LIBRARY=../libleveldb/lib/libleveldb.so \
	-DLEVELDB_INCLUDE_DIR=../libleveldb/include \
	..

make -j2
