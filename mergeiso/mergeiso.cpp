#include <iostream>
#include <fstream>
#include <math.h>
#include "../include/Image.h"


void usage()
{
	printf("usage: mergeiso <metadatafile> <outputname> <tilesize>\n");
	exit(1);

}

char const *file(int x, int y, char const *basename)
{
	static char ret[1024];

	sprintf(ret, "iso_%d_%d_%s", x,y, basename);

	return ret;
}


int main(int argc, char **argv)
{
	if (argc != 4)
	{
		usage();
	}

	std::ifstream mt;
	mt.open(argv[1], std::ios::in);

	if (!mt.is_open())
	{
		std::cerr << "Couldn't open file '\n" << argv[1] << "'" <<  std::endl;
		exit(1);
	}


	std::string baseName;
	int numTilesX, numTilesY, minTileX, minTileY, tileSizeX, tileSizeY, zoom, maxImW, maxImH;
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
		else if (label ==  "Zoom:")
		{
			mt >> zoom;
			count++;
		}
		else if (label ==  "MaxImageSize:")
		{
			mt >> maxImW >> maxImH;
			count++;
		}

		if (count > 4)
		{
			break;
		}
	}


	int blockW = zoom * 4;
	int outTileSize = strtol(argv[3], NULL, 0);


	Image out(outTileSize, outTileSize);


	int blockH = (int)((blockW)/sqrt(3));

	int inputTileW = tileSizeX * blockW;

	int dx = inputTileW/2;
	int dy = (inputTileW/blockW) * blockH / 2;


	int totalMapW = inputTileW + (dx * (numTilesX+numTilesY));
	int totalMapH = maxImH + (dy * (numTilesX+numTilesY));


	int mapTilesW = totalMapW / outTileSize;
	int mapTilesH = totalMapH / outTileSize;


	if (totalMapW % outTileSize)
	{
		mapTilesW++;
	}

	if (totalMapH % outTileSize)
	{
		mapTilesH++;
	}


//	int inTop = in->h - dy;
//	int halfHeight = inTop + (totalMapH - inTop) / 2;





	for (int x = 0; x < mapTilesW; x++)
	{
		for (int y = 0; y < mapTilesH; y++)
		{
			Color empty(0,0,0,0);
			out.fill(empty, true);
			int count = 0;
			printf(" TILE %d %d\n", x, y);

			for (int ix = 0; ix < numTilesX; ix++)
			{
				for (int iy = numTilesY -1; iy >= 0; iy--)
				{

					int destX = (ix+iy) * dx - x * outTileSize;
					int destY = outTileSize - ((iy-ix)*dy - y * outTileSize)  - maxImH - dy* numTilesX;
 					//, inputTileW, inputTileH};
					if (destX >= outTileSize || destY >= outTileSize || destX <= -inputTileW || destY <= -maxImH)
					{
						continue;
					}

					try
					{
						Image in(file(ix,iy,baseName.c_str()));

						destX = (ix+iy) * dx - x * outTileSize;
						destY = outTileSize - ((iy-ix)*dy - y * outTileSize)  - in.GetHeight() - dy* numTilesX;
						if (!(destX >= outTileSize || destY >= outTileSize || destX <= inputTileW || destY <= in.GetHeight()))
						{
							count++;
							in.blit(&out, destX, destY);
						}

					}
					catch(std::runtime_error const &e)
					{
						// do nothing, loading is allowed to fail
					}
				}

			}

			if (count)
			{
				out.save(file(x, y, "retiled.png"));
			}
		}
	}
}
