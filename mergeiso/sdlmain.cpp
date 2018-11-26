#include <iostream>
#include <fstream>
#include <SDL.h>
#include "SDL_image.h"


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


	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	SDL_Window *window = SDL_CreateWindow("SDL2 prog", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1900,1024, SDL_WINDOW_OPENGL);

	if (!window)
	{
		printf("can't open window!");
		exit(1);
	}

	IMG_Init(IMG_INIT_PNG);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer)
	{
		SDL_DestroyWindow(window);
		printf("can't create renderer!");
		exit(1);
	}


	SDL_Surface *out = SDL_CreateRGBSurfaceWithFormat(0, outTileSize, outTileSize, 32, SDL_PIXELFORMAT_RGBA32);

	SDL_Rect tileRect = {0,0, 1024, 1024};

//	SDL_RenderSetScale(renderer, .25,.25);

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



//	mapTilesW = 1;

	SDL_Surface *in = NULL;
	for (int x = 0; x < mapTilesW; x++)
	{
		for (int y = 0; y < mapTilesH; y++)
		{
			SDL_FillRect(out, NULL,  SDL_MapRGBA(out->format, 0, 0, 0, 0));
			int count = 0;
			printf(" TILE %d %d\n", x, y);

			for (int ix = 0; ix < tilesW; ix++)
			{
				for (int iy = tilesH -1; iy >= 0; iy--)
				{

					SDL_Rect dest = { (ix+iy) * dx - x * outTileSize, outTileSize - ((iy-ix)*dy - y * outTileSize)  - inputTileH - dy* tilesW , inputTileW, inputTileH};
					if (dest.x >= outTileSize || dest.y >= outTileSize || dest.x <= -dest.w || dest.y <= -dest.h)
					{
						continue;
					}

					if (in)
					{
						SDL_FreeSurface(in);
					}
					in = IMG_Load(file(ix,iy,baseName));

					if (in && in->h)
					{
						SDL_Rect dest = { (ix+iy) * dx - x * outTileSize, outTileSize - ((iy-ix)*dy - y * outTileSize)  - in->h - dy* tilesW , in->h, inputTileH};
						if (!(dest.x >= outTileSize || dest.y >= outTileSize || dest.x <= -dest.w || dest.y <= -dest.h))
						{
							count++;
							printf("added %s :  %d %d  %d %d\n", file(ix,iy,baseName), dest.x, dest.y, dest.w, dest.h);
							SDL_BlitSurface(in, NULL, out, &dest);
						}

					}
				}
			}

			if (count)
			{
						SDL_RenderClear(renderer);
						SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, out);
						SDL_Rect srcRect = { 0,0, outTileSize, outTileSize};
						SDL_RenderCopy(renderer, t, &srcRect, &tileRect);

						SDL_DestroyTexture(t);
						SDL_RenderPresent(renderer);
				SDL_SaveBMP(out, file(x, y, "out.bmp"));
			}
		}
	}


	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();


}
