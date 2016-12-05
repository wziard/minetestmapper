#include <cstdio>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Image.h"

#ifndef NDEBUG
#define SIZECHECK(x, y) do { \
		if((x) < 0 || (x) >= m_width) \
			throw std::out_of_range("sizecheck x"); \
		if((y) < 0 || (y) >= m_height) \
			throw std::out_of_range("sizecheck y"); \
	} while(0)
#else
#define SIZECHECK(x, y) do {} while(0)
#endif

// ARGB but with inverted alpha

static inline RGBQUAD color2rgbquad(Color c)
{
	RGBQUAD c2;
	c2.rgbRed = c.r;
	c2.rgbGreen = c.g;
	c2.rgbBlue = c.b;
	return c2;
}

static inline Color rgbquad2color(RGBQUAD c)
{
	return Color(c.rgbRed, c.rgbGreen, c.rgbBlue);
}


Image::Image(int width, int height) :
	m_width(width), m_height(height), m_image(NULL)
{
	// FIXME: This works because minetestmapper only creates one image
	FreeImage_Initialise();
	printf("Using FreeImage v%s\n", FreeImage_GetVersion());

	m_image = FreeImage_Allocate(width, height, 24);
}

Image::~Image()
{
	FreeImage_Unload(m_image);
}

void Image::setPixel(int x, int y, const Color &c)
{
	SIZECHECK(x, y);
	RGBQUAD col = color2rgbquad(c);
	FreeImage_SetPixelColor(m_image, x, y, &col);
}

Color Image::getPixel(int x, int y)
{
#ifndef NDEBUG
	if(x < 0 || x > m_width || y < 0 || y > m_height)
		throw std::out_of_range("sizecheck");
#endif
	RGBQUAD c;
	FreeImage_GetPixelColor(m_image, x, y, &c);
	return rgbquad2color(c);
}

void Image::drawLine(int x1, int y1, int x2, int y2, const Color &c)
{
	SIZECHECK(x1, y1);
	SIZECHECK(x2, y2);
	// TODO
}

void Image::drawText(int x, int y, const std::string &s, const Color &c)
{
	SIZECHECK(x, y);
	// TODO
}

void Image::drawFilledRect(int x, int y, int w, int h, const Color &c)
{
	SIZECHECK(x, y);
	SIZECHECK(x + w - 1, y + h - 1);
	RGBQUAD col = color2rgbquad(c);
	for(int xx = 0; xx < w; xx++)
		for(int yy = 0; yy < h; yy++)
			FreeImage_SetPixelColor(m_image, x + xx, y + yy, &col);
}

void Image::drawCircle(int x, int y, int diameter, const Color &c)
{
	SIZECHECK(x, y);
	// TODO
}

void Image::save(const std::string &filename)
{
	FreeImage_Save(FIF_PNG, m_image, filename.c_str()); // other formats?
}
