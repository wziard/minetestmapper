Minetest Mapper C++
===================

.. image:: https://travis-ci.org/minetest/minetestmapper.svg?branch=master
    :target: https://travis-ci.org/minetest/minetestmapper

A port of minetestmapper.py to C++ from https://github.com/minetest/minetest/tree/master/util.
This version is both faster and provides more features than the now deprecated Python script.

Requirements
------------

* libgd
* sqlite3
* LevelDB (optional, set ENABLE_LEVELDB=1 in CMake to enable)
* hiredis library (optional, set ENABLE_REDIS=1 in CMake to enable)
* Postgres libraries (optional, set ENABLE_POSTGRES=1 in CMake to enable)

e.g. on Debian:
^^^^^^^^^^^^^^^

	sudo apt-get install libgd-dev libsqlite3-dev libleveldb-dev libhiredis-dev libpq-dev

Windows
^^^^^^^
Minetestmapper for Windows can be downloaded here: https://github.com/minetest/minetestmapper/releases

After extracting the archive, minetestmapper can be invoked from cmd.exe:
::

	cd C:\Users\yourname\Desktop\example\path
	minetestmapper.exe --help

Compilation
-----------

::

    cmake . -DENABLE_LEVELDB=1
    make -j2

Usage
-----

`minetestmapper` has two mandatory paremeters, `-i` (input world path)
and `-o` (output image path).

::

    ./minetestmapper -i ~/.minetest/worlds/my_world/ -o map.png


Parameters
^^^^^^^^^^

bgcolor:
    Background color of image, e.g. ``--bgcolor '#ffffff'``

scalecolor:
    Color of scale marks and text, e.g. ``--scalecolor '#000000'``

playercolor:
    Color of player indicators, e.g. ``--playercolor '#ff0000'``

origincolor:
    Color of origin indicator, e.g. ``--origincolor '#ff0000'``

drawscale:
    Draw scale(s) with tick marks and numbers, ``--drawscale``

drawplayers:
    Draw player indicators with name, ``--drawplayers``

draworigin:
    Draw origin indicator, ``--draworigin``

drawalpha:
    Allow nodes to be drawn with transparency (e.g. water), ``--drawalpha``

extent:
    Don't output any imagery, just print the extent of the full map, ``--extent``

noshading:
    Don't draw shading on nodes, ``--noshading``

noemptyimage:
    Don't output anything when the image would be empty, ``--noemptyimage``

min-y:
    Don't draw nodes below this y value, e.g. ``--min-y -25``

max-y:
    Don't draw nodes above this y value, e.g. ``--max-y 75``

backend:
    Override auto-detected map backend; supported: *sqlite3*, *leveldb*, *redis*, *postgresql*, e.g. ``--backend leveldb``

geometry:
    Limit area to specific geometry (*x:z+w+h* where x and z specify the lower left corner), e.g. ``--geometry -800:-800+1600+1600``

tilesize:
    Don't output one big image, but output tiles of the specified size, e.g. "--tilesize 128x128". The sizes will be rounded to
    a multiple of 16. The filenames will be created in the form <x>_<y>_<filename>, where <x> and <y>
    are the tile numbers and <filename> is the name specified with -o. Skip empty tiles by also specifying --noemptyimage.

zoom:
    Apply zoom to drawn nodes by enlarging them to n*n squares, e.g. ``--zoom 4``

colors:
    Override auto-detected path to colors.txt, e.g. ``--colors ../minetest/mycolors.txt``

scales:
    Draw scales on specified image edges (letters *t b l r* meaning top, bottom, left and right), e.g. ``--scales tbr``

isometric:
    Draws the map in isometric view.

isoshadeheight:
    In isometric mode the color is shaded darker for low terrain, and lighter (more white) for higher terrain.
    This makes shure high mountains stand out against flat plains. Because there's no perspective it is otherwise hard
    to see the height differences. This height gives the height (above --min-y) at which the color will be unchanged.
    The default is 75, which works quite well on magen v7 terrain. If you have very high mountains (or buildings) you \
    probably want to set this higher.

