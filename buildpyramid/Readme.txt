usage: buildpyramid <metadatafile> <outputname>

Reads the metadata file (output by minetestmapper) describing how many tiles are available and writes a full zoom pyramid and some HTML code for use with leaflet.js to display a 'slippy' map.
just copy leaflet.js and leaflet.css form leaflet into the same folder and you whould have a working map.

Outputname should end in .jpg (recommended) or .png.



buildpyramid can't handle subdirectories. run it from the directory containing your minetestmapper output. it will write your map to the same folder.

