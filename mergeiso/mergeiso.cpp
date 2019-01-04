#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "../include/Image.h"


void usage()
{
	printf("usage: mergeiso <metadatafile> <outputname> <tilesize>\n");
	exit(1);

}

char const *file(int x, int y, char const *prefix, char const *basename)
{
	static char ret[1024];

	sprintf(ret, "%s%d_%d_%s", prefix, x,y, basename);

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
	int numTilesX, numTilesZ, minTileX, minTileZ, tileSizeX, tileSizeY, zoom, maxImW, maxImH, originHeight;
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
			mt >> numTilesX >> numTilesZ;
			count++;
		}
		else if (label ==  "MinTile:")
		{
			mt >> minTileX >> minTileZ;
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
		else if (label ==  "OriginHeight:")
		{
			mt >> originHeight;
			count++;
		}

		if (count > 6)
		{
			break;
		}
	}


	int blockW = zoom * 4;
	int outTileSize = strtol(argv[3], NULL, 0);


	Image out(outTileSize, outTileSize);


	int blockH = 2*(int)(zoom * 2 /sqrt(3));

	int inputTileW = tileSizeX * blockW;
	int inputTileH = tileSizeX * blockH;

	int dx = inputTileW/2;
	int dy = inputTileH / 2;


	int totalMapW = inputTileW + (dx * (numTilesX+numTilesZ));
	int totalMapH = maxImH + (dy * (numTilesX+numTilesZ));


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

	mapTilesW++;
	mapTilesH++;

	int minMapTileX = (minTileX + minTileZ) * dx / outTileSize - 1;
	int minMapTileY = (minTileX - minTileZ) * dy / outTileSize - 1;

//	int inTop = in->h - dy;
//	int halfHeight = inTop + (totalMapH - inTop) / 2;


	std::ostringstream mfn;
	mfn << "metadata_" << argv[2] << ".txt";
	std::ofstream mf;

	mf.open(mfn.str().c_str());
	mf << "BaseName: " << argv[2] << std::endl;
	mf << "NumTiles: " << mapTilesW << " " << mapTilesH << std::endl;
	mf << "MinTile: " << minMapTileX << " " << minMapTileY << std::endl;
	mf << "TileSize: " << outTileSize/zoom << " " << outTileSize/zoom << std::endl;
	mf << "Zoom: " << zoom << std::endl;
	mf.close();



	for (int x = minMapTileX; x < minMapTileX + mapTilesW; x++)
	{
		for (int y = minMapTileY; y < minMapTileY + mapTilesH; y++)
		{
			Color empty(0,0,0,0);
			out.fill(empty, true);
			int count = 0;
			printf(" TILE %d %d\n", x, y);

			for (int ix = minTileX; ix < minTileX + numTilesX; ix++)
			{
				for (int iy = minTileZ + numTilesZ -1; iy >= minTileZ; iy--)
				{
					// this is the position of the left corner of the isometric tile on the output map

					int leftCornerX = (ix + iy) * dx;
					int leftCornerY = (iy - ix) * dy;

					// now make the current image line up with that
					// in the image it's at (0, h - originHeight)
					int destX = leftCornerX - x * outTileSize;
					// top of image is imh - originHeight heigher than topcorner
					int destY = outTileSize - (leftCornerY - y * outTileSize + (maxImH - originHeight)) - 1;
//					printf("  %d %d:  (%d, %d) (%d, %d)\n", ix, iy, destX, destY, destX + inputTileW, destY - maxImH);
 					//, inputTileW, inputTileH};
					if (destX >= outTileSize || destY >= outTileSize || destX <= -inputTileW || destY <= -maxImH)
					{
//						printf("skip\n");
						continue;
					}

					try
					{
						Image in(file(ix,iy,"iso_tile_", baseName.c_str()));
						int destY = outTileSize - (leftCornerY - y * outTileSize + (in.GetHeight() - originHeight)) - 1;

						if (!(destY < -in.GetHeight()))
						{
							count++;
							in.blit(&out, destX, destY);
//							printf("JA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
						}
						else
						{
//							printf("skip\n");
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
				out.save(file(x, y, "tile_", argv[2]));
			}
		}
	}
}
