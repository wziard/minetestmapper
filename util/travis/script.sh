#!/bin/bash -e

CXX=g++-7
[ $CC == "clang" ] && CXX=clang++-5.0
export CXX


mkdir -p travisbuild
cd travisbuild

cmake \
	-DENABLE_LEVELDB=1 \
	..

make -j2
