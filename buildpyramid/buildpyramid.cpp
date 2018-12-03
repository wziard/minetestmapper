#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../include/Image.h"

void outputLeafletCode(std::string const &output, int maxLevel, int tileSize);

class LazyImage
{
	public:
		LazyImage(int w, int h, Color const &bg)
		{
			m_w = w;
			m_h = h;
			m_bg = bg;
			m_image = NULL;
		}

		int GetWidth() { return m_w; }
		int GetHeight() { return m_h; }

		~LazyImage()
		{
			delete m_image;
			m_image = NULL;
		}

		void wipe()
		{
			if (m_image)
			{
				m_image->fill(m_bg);
			}
		}

		void quarterBlit(LazyImage const & from, int x, int y)
		{
			if (!from.m_image)
			{
				return;
			}

			if (!m_image)
			{
				m_image = new Image(m_w, m_h);
				m_image->fill(m_bg);
			}

			from.m_image->scaleBlit(m_image, x * m_w/2, y*m_h/2, m_w/2, m_h/2);
		}

		void quarterBlit(Image const & from, int x, int y)
		{
			if (!m_image)
			{
				m_image = new Image(m_w, m_h);
				m_image->fill(m_bg);
			}

			from.scaleBlit(m_image, x * m_w/2, y*m_h/2, m_w/2, m_h/2);
		}

		void blit(Image const & from, int x, int y)
		{
			if (!m_image)
			{
				m_image = new Image(m_w, m_h);
				m_image->fill(m_bg);
			}

			from.blit(m_image, x, y);
		}

		void save(std::string const &name)
		{
			if (m_image)
			{
				m_image->save(name);
			}
		}
	private:

		int m_w, m_h;
		Color m_bg;
		Image *m_image;

};

int buildPyramid(std::string const &baseName, std::string const &out, LazyImage *im, int minTileX,int minTileY, int maxTileX, int maxTileY, int level, int divisor, bool leafletMode)
{
	int count = 0;

	if ((maxTileX - minTileX) == 1)
	{
		assert((maxTileY - minTileY) == 1);
		std::ostringstream inf;
		inf << "tile_" << minTileX << "_" << minTileY << "_" << baseName;
		try
		{
			Image in(inf.str());

			if (in.GetWidth())
			{
				im->blit(in,0,0);
				count++;
			}
		}
		catch (std::runtime_error const &e)
		{
			// image is allowed not to exist, just return 0;
			return 0;
		}

	}
	else
	{
		LazyImage subTile(im->GetWidth(), im->GetHeight(), Color(255,255,255,0));

		int halfX = minTileX + (maxTileX - minTileX)/2;
		int halfY = minTileY + (maxTileY - minTileY)/2;
		int c = buildPyramid(baseName, out, &subTile, minTileX, minTileY, halfX, halfY, level +1, divisor / 2, leafletMode);
		if (c)
		{
			im->quarterBlit(subTile, 0, 1);
			subTile.wipe();
			count+=c;
		}

		c = buildPyramid(baseName, out, &subTile, halfX,  minTileY, maxTileX, halfY, level +1, divisor / 2, leafletMode);
		if (c)
		{
			im->quarterBlit(subTile, 1, 1);
			subTile.wipe();
			count+=c;
		}

		c = buildPyramid(baseName, out, &subTile, minTileX, halfY, halfX, maxTileY, level +1, divisor / 2, leafletMode);
		if (c)
		{
			im->quarterBlit(subTile, 0, 0);
			subTile.wipe();
			count+=c;
		}


		c= buildPyramid(baseName, out, &subTile, halfX,  halfY, maxTileX, maxTileY, level +1, divisor / 2, leafletMode);
		if (c)
		{
			im->quarterBlit(subTile, 1, 0);
			subTile.wipe();
			count+=c;
		}


	}

	if (count)
	{
		std::ostringstream of;

		of << level << "_" <<  (minTileX / divisor) << "_" << ((leafletMode ? -1 : 1) * minTileY/divisor - (leafletMode ? 1 : 0)) << "_" << out;

		std::cout << "Writing image: " << of.str() << std::endl;
		im->save(of.str());

	}
	return count;
}


int main(int argc, char **argv)
{
	if (argc !=3)
	{
		std::cerr << "Usage: buildpyramid <metadatafile> <outname>\n" << std::endl;
		exit(1);
	}

	std::ifstream mt;
	mt.open(argv[1], std::ios::in);

	if (!mt.is_open())
	{
		std::cerr << "Couldn't open file '\n" << argv[1] << "'" <<  std::endl;
		exit(1);
	}


	std::string baseName;
	int numTilesX, numTilesY, minTileX, minTileY, tileSizeX, tileSizeY;
	int count=0;
	while (true)
	{

		std::string label;

		mt >> label;

		if (mt.eof())
		{
			std::cerr << "Error parsing metadata file\n" << std::endl;
			exit(1);
		}

		if (label == "BaseName:")
		{
			mt >> baseName;
			count++;
		}
		else if (label ==  "NumTiles:")
		{
			mt >> numTilesX >> numTilesY;
			count++;
		}
		else if (label ==  "MinTile:")
		{
			mt >> minTileX >> minTileY;
			count++;
		}
		else if (label ==  "TileSize:")
		{
			mt >> tileSizeX >> tileSizeY;
			count++;
		}

		if (count >3)
		{
			break;
		}
	}

	if (tileSizeX != tileSizeY)
	{
		std::cerr << "Can't handle non-square tiles." << std::endl;
		exit(1);
	}

	int maxDim = numTilesX + minTileX;

	maxDim = -minTileX > maxDim ? -minTileX : maxDim;

	maxDim = numTilesY + minTileY > maxDim ? numTilesY - minTileY : maxDim;

	maxDim = -minTileY > maxDim ? -minTileY : maxDim;

	assert(maxDim >0);

	int maxLevel;
	// round up to power of 2
	for (int i = 0; i < 62 ; i++)
	{
		if ((1 << i) >= maxDim)
		{
			maxDim = 1<<i;
			maxLevel = i;
			break;
		}
	}


	LazyImage im(tileSizeX, tileSizeY, Color(255,255,255,0));
	std::ostringstream of;
	std::string out(argv[2]);

	buildPyramid(baseName, out, &im, 0,0, maxDim, maxDim, 0, maxDim, true);
	buildPyramid(baseName, out, &im, -maxDim,0, 0, maxDim, 0, maxDim, true);
	buildPyramid(baseName, out, &im, -maxDim,-maxDim, 0,0, 0, maxDim, true);
	buildPyramid(baseName, out, &im, 0,-maxDim, maxDim, 0, 0, maxDim, true);



	outputLeafletCode(out, maxLevel, tileSizeX);
	return 0;

}






static char const *leafletMapHtml =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"\t<title>MinetestMapper</title>\n"
"\t<meta charset=\"utf-8\" />\n"
"\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"\t<!-- link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"favicon.ico\" /-->\n"
"\t<link rel=\"stylesheet\" href=\"leaflet.css\" integrity=\"sha512-puBpdR0798OZvTTbP4A8Ix/l+A4dHDD0DGqYW6RQ+9jxkRFclaxxQb/SJAWZfWAkuyeQUytO7+7N4QKrDh+drA==\" crossorigin=\"\"/>\n"
"\t<script src=\"leaflet.js\" integrity=\"sha512-nMMmRyTVoLYqjP9hrbed9S+FzjZHW5gY1TWCHA5ckwXZBadntCNs8kEqAWdrb9O7rxbCaA4lKTIWjDXZxflOcA==\" crossorigin=\"\"></script>\n"
"\t<style> .labelclass{position: absolute; background: rgba(255,0,255,0); font-size:20px;}</style>\n"
"</head>\n"
"<body>\n"
"<div id=\"mapid\" style=\"width: 90vw; height: 90vh;\"></div>\n"
"<script>\n"
"\tvar MineTestMap = L.map('mapid', {\n"
"\tcrs: L.CRS.Simple,\n"
"\t});\n"
"\tMineTestMap.setView([0.0, 0.0], %d);\n"
"\tL.tileLayer('{z}_{x}_{y}_%s', {\n"
"\t\tminNativeZoom: 0,\n"
"\t\tmaxNativeZoom: %d,\n"
"\t\tattribution: 'Minetest World',\n"
"\t\ttileSize: %d,\n"
"\t\terrorTileUrl: \"empty_tile_%s\",\n"
"\t\t}).addTo(MineTestMap);\n"
"\tvar popup = L.popup();\n"
"\tvar mapZoom = %f;\n"
"\tfunction onMapClick(e) {\n"
"\t\tvar scaledPos = L.latLng(e.latlng.lat / mapZoom, e.latlng.lng / mapZoom);\n"
"\t\tpopup\n"
"\t\t\t.setLatLng(e.latlng)\n"
"\t\t\t.setContent(\"You clicked the map at \"+ scaledPos.toString())\n"
"\t\t\t.openOn(MineTestMap);\n"
"\t}\n"
"\tMineTestMap.on('click', onMapClick);\n"
"</script>\n"
"<script src=\"markers.js\" defer></script>"
"</body>\n"
"</html>\n";


void outputLeafletCode(std::string const &output, int maxLevel, int tileSize)
{


	std::ostringstream fn;
	fn << output << ".html";

	// I use fopen instead of ostr because I need fprintf to put the correct values into the
	// html static string above. ostream is nice and C++ and all, but formatted output handling
	// for streams is retarded. Use the best tool for the job.
	FILE *out = fopen(fn.str().c_str(), "w");

	if (!out)
	{
		std::cout << "error opening file:" << fn.str() << std::endl;
		return;
	}

	fprintf(out, leafletMapHtml, maxLevel, output.c_str(), maxLevel, tileSize, output.c_str(), 1.0/pow(2,maxLevel));

	fclose(out);
}


