#!/bin/bash -e

#######
# this expects an env similar to what minetest's buildbot uses
# extradll_path will typically contain libgcc, libstdc++ and libpng
toolchain_file=
toolchain_file64=
libgd_dir=
libgd_dir64=
zlib_dir=
zlib_dir64=
sqlite_dir=
sqlite_dir64=
leveldb_dir=
leveldb_dir64=
extradll_path=
extradll_path64=
#######

[ -f ./CMakeLists.txt ] || exit 1

if [ "$1" == "32" ]; then
	:
elif [ "$1" == "64" ]; then
	toolchain_file=$toolchain_file64
	libgd_dir=$libgd_dir64
	zlib_dir=$zlib_dir64
	sqlite_dir=$sqlite_dir64
	leveldb_dir=$leveldb_dir64
	extradll_path=$extradll_path64
else
	echo "Usage: $0 <32 / 64>"
	exit 1
fi

cmake . \
	-DCMAKE_INSTALL_PREFIX=/tmp \
	-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
	-DCMAKE_EXE_LINKER_FLAGS="-s" \
	\
	-DENABLE_LEVELDB=1 \
	\
	-DLIBGD_INCLUDE_DIR=$libgd_dir/include \
	-DLIBGD_LIBRARY=$libgd_dir/lib/libgd.dll.a \
	\
	-DZLIB_INCLUDE_DIR=$zlib_dir/include \
	-DZLIB_LIBRARY=$zlib_dir/lib/libz.dll.a \
	\
	-DSQLITE3_INCLUDE_DIR=$sqlite_dir/include \
	-DSQLITE3_LIBRARY=$sqlite_dir/lib/libsqlite3.dll.a \
	\
	-DLEVELDB_INCLUDE_DIR=$leveldb_dir/include \
	-DLEVELDB_LIBRARY=$leveldb_dir/lib/libleveldb.dll.a

make -j4

mkdir pack
cp -p \
	AUTHORS colors.txt COPYING README.rst \
	minetestmapper.exe \
	$libgd_dir/bin/libgd-3.dll \
	$zlib_dir/bin/zlib1.dll \
	$sqlite_dir/bin/libsqlite3-0.dll \
	$leveldb_dir/bin/libleveldb.dll \
	$extradll_path/*.dll \
	pack/
zipfile=minetestmapper-win$1.zip
(cd pack; zip -9r ../$zipfile *)

make clean
rm -r pack CMakeCache.txt

echo "Done."
