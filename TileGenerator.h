/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  23.08.2012 12:35:59
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#ifndef TILEGENERATOR_H_JJNUCARH
#define TILEGENERATOR_H_JJNUCARH

#include <gd.h>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include "PixelAttributes.h"
#include "db.h"

struct Color {
	Color(): r(0xFF), g(0xFF), b(0xFF), a(0) {};
	Color(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b), a(0xFF) {};
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {};
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct ColorEntry {
    ColorEntry(): r(0), g(0), b(0), a(0), t(0) {};
	ColorEntry(uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint8_t t): r(r), g(g), b(b), a(a), t(t) {};
	inline Color to_color() const { return Color(r, g, b, a); }
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	uint8_t t;
};


struct BlockPos {
	int x;
	int y;
	int z;
	bool operator<(const BlockPos& p) const
	{
		if (z > p.z) {
			return true;
		}
		if (z < p.z) {
			return false;
		}
		if (y > p.y) {
			return true;
		}
		if (y < p.y) {
			return false;
		}
		if (x > p.x) {
			return true;
		}
		if (x < p.x) {
			return false;
		}
		return false;
	}
};


class TileGenerator
{
private:
	typedef std::basic_string<unsigned char> unsigned_string;
	typedef std::map<std::string, ColorEntry> ColorMap;
	typedef std::pair<BlockPos, unsigned_string> Block;
	typedef std::list<Block> BlockList;

public:
	TileGenerator();
	~TileGenerator();
	void setBgColor(const std::string &bgColor);
	void setScaleColor(const std::string &scaleColor);
	void setOriginColor(const std::string &originColor);
	void setPlayerColor(const std::string &playerColor); Color parseColor(const std::string &color);
	void setDrawOrigin(bool drawOrigin);
	void setDrawPlayers(bool drawPlayers);
	void setDrawScale(bool drawScale);
	void setDrawAlpha(bool drawAlpha);
	void setShading(bool shading);
	void setGeometry(int x, int y, int w, int h);
	void setMinY(int y);
	void setMaxY(int y);
	void parseColorsFile(const std::string &fileName);
	void setBackend(std::string backend);
	void generate(const std::string &input, const std::string &output);

private:
	void parseColorsStream(std::istream &in);
	void openDb(const std::string &input);
	void loadBlocks();
	BlockPos decodeBlockPos(int64_t blockId) const;
	void createImage();
	void renderMap();
	std::list<int> getZValueList() const;
	std::map<int, BlockList> getBlocksOnZ(int zPos);
	void renderMapBlock(const unsigned_string &mapBlock, const BlockPos &pos, int version);
	void renderShading(int zPos);
	void renderScale();
	void renderOrigin();
	void renderPlayers(const std::string &inputPath);
	void writeImage(const std::string &output);
	void printUnknown();
	int getImageX(int val) const;
	int getImageY(int val) const;

private:
	Color m_bgColor;
	Color m_scaleColor;
	Color m_originColor;
	Color m_playerColor;
	bool m_drawOrigin;
	bool m_drawPlayers;
	bool m_drawScale;
	bool m_drawAlpha;
	bool m_shading;
	int m_border;
	std::string m_backend;

	DB *m_db;
	gdImagePtr m_image;
	PixelAttributes m_blockPixelAttributes;
	int m_xMin;
	int m_xMax;
	int m_zMin;
	int m_zMax;
	int m_yMin;
	int m_yMax;
	int m_geomX;
	int m_geomY;
	int m_geomX2;
	int m_geomY2;
	int m_mapWidth;
	int m_mapHeight;
	std::list<std::pair<int, int> > m_positions;
	std::map<int, std::string> m_nameMap;
	ColorMap m_colors;
	uint16_t m_readedPixels[16];
	std::set<std::string> m_unknownNodes;

	int m_blockAirId;
	int m_blockIgnoreId;
}; /* -----  end of class TileGenerator  ----- */

#endif /* end of include guard: TILEGENERATOR_H_JJNUCARH */

