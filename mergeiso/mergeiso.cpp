#include <iostream>
#include <fstream>
#include <math.h>
#include "../include/Image.h"


void usage()
{
	printf("usage: mergeiso <basename> <w> <h> <inputBlockWidth> <outputTileSize>\n");
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
	if (argc !=6)
	{
		usage();
	}

	std::ifstream meta;

	meta.open(std::string("iso_metadata_") + argv[1] + std::string(".txt"), std::ios::in);

	std::string maxImSize;
	int inputTileW, inputTileH;
	meta >> maxImSize >> inputTileW >> inputTileH;
	meta.close();

	char const *baseName = argv[1];
	int tilesW = strtol(argv[2], NULL, 0);
	int tilesH = strtol(argv[3], NULL, 0);
	int blockW = strtol(argv[4], NULL, 0);
	int outTileSize = strtol(argv[5], NULL, 0);


	Image out(outTileSize, outTileSize);


	int blockH = (int)((blockW)/sqrt(3));


	int dx = inputTileW/2;
	int dy = (inputTileW/blockW) * blockH / 2;


	int totalMapW = inputTileW + (dx * (tilesW+tilesH));
	int totalMapH = inputTileH + (dy * (tilesW+tilesH));


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

			for (int ix = 0; ix < tilesW; ix++)
			{
				for (int iy = tilesH -1; iy >= 0; iy--)
				{

					int destX = (ix+iy) * dx - x * outTileSize;
					int destY = outTileSize - ((iy-ix)*dy - y * outTileSize)  - inputTileH - dy* tilesW;
 					//, inputTileW, inputTileH};
					if (destX >= outTileSize || destY >= outTileSize || destX <= -inputTileW || destY <= -inputTileH)
					{
						continue;
					}

					try
					{
						Image in(file(ix,iy,baseName));

						destX = (ix+iy) * dx - x * outTileSize;
						destY = outTileSize - ((iy-ix)*dy - y * outTileSize)  - in.GetHeight() - dy* tilesW;
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
