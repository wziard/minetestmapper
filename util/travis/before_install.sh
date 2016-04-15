#!/bin/bash -e


if [ $CC = "clang" ]; then
	export PATH="/usr/bin/:$PATH"
	sudo sh -c 'echo "deb http://ppa.launchpad.net/eudoxos/llvm-3.1/ubuntu precise main" >> /etc/apt/sources.list'
	sudo apt-key adv --keyserver pool.sks-keyservers.net --recv-keys 92DE8183
	sudo apt-get update
	sudo apt-get install llvm-3.1
	sudo apt-get install clang
fi

sudo apt-get update
sudo apt-get install cmake libgd2-noxpm-dev libsqlite3-dev \
	p7zip-full

# I love how nothing ever works correctly with precise
wget http://minetest.kitsunemimi.pw/libleveldb-1.18-ubuntu12.04.7z
sudo 7z x -o/usr libleveldb-1.18-ubuntu12.04.7z

