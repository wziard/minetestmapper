#ifndef IMAGE_HEADER
#define IMAGE_HEADER

#include "types.h"
#include <string>
#include <gd.h>

typedef gdPoint ImagePoint;

struct Color {
	Color() : r(0), g(0), b(0), a(0) {};
	Color(u8 r, u8 g, u8 b) : r(r), g(g), b(b), a(255) {};
	Color(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) {};
	inline Color noAlpha() const { return Color(r, g, b); }

	Color gamma(double val) const;

	bool operator!=(const Color &other) const { return  r!=other.r || g != other.g || b!=other.b || a!=other.a; }
	bool operator==(const Color &other) const { return  r==other.r && g == other.g && b==other.b && a==other.a; }
	u8 r, g, b, a;
};

class Image {
public:
	Image(const std::string &fileName);
	Image(int width, int height);
	~Image();

	void scaleBlit(Image *to, int x, int y, int w, int h) const;
	void blit(Image *to, int x, int y);
	void blit(Image *to, int xs, int ys, int xd, int yd, int w, int h);
	void setPixel(int x, int y, const Color &c);
	Color getPixel(int x, int y) const;
	void drawLine(int x1, int y1, int x2, int y2, const Color &c);
	void drawText(int x, int y, const std::string &s, const Color &c);
	void drawFilledRect(int x, int y, int w, int h, const Color &c);
	void drawFilledPolygon(int nrPoints, ImagePoint const *p, Color const &c, bool setAlpha);
	void drawCircle(int x, int y, int diameter, const Color &c);
	void save(const std::string &filename) const;
	void fill(const Color &c, bool setAlpha = false);

	inline int GetHeight() { return m_height; }
	inline int GetWidth() { return m_width; }
	void crop(int x1, int y1, int x2, int y2);
private:
	Image(const Image&);

	int m_width, m_height;
	gdImagePtr m_image;
};

#endif // IMAGE_HEADER
