#include <cstdio>
#include <cstdlib>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <math.h>
#include <set>

#include "TileGenerator.h"
#include "config.h"
#include "PlayerAttributes.h"
#include "BlockDecoder.h"
#include "util.h"
#include "db-sqlite3.h"
#if USE_POSTGRESQL
#include "db-postgresql.h"
#endif
#if USE_LEVELDB
#include "db-leveldb.h"
#endif
#if USE_REDIS
#include "db-redis.h"
#endif

using namespace std;

template<typename T>
static inline T mymax(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
static inline T mymin(T a, T b)
{
	return (a > b) ? b : a;
}

// rounds n (away from 0) to a multiple of f while preserving the sign of n
static int round_multiple_nosign(int n, int f)
{
	int abs_n, sign;
	abs_n = (n >= 0) ? n : -n;
	sign = (n >= 0) ? 1 : -1;
	if (abs_n % f == 0)
		return n; // n == abs_n * sign
	else
		return sign * (abs_n + f - (abs_n % f));
}

static inline unsigned int colorSafeBounds (int channel)
{
	return mymin(mymax(channel, 0), 255);
}

static Color mixColors(Color a, Color b)
{
	Color result;
	double a1 = a.a / 255.0;
	double a2 = b.a / 255.0;

	result.r = (int) (a1 * a.r + a2 * (1 - a1) * b.r);
	result.g = (int) (a1 * a.g + a2 * (1 - a1) * b.g);
	result.b = (int) (a1 * a.b + a2 * (1 - a1) * b.b);
	result.a = (int) (255 * (a1 + a2 * (1 - a1)));

	return result;
}

TileGenerator::TileGenerator():
	m_bgColor(255, 255, 255),
	m_scaleColor(0, 0, 0),
	m_originColor(255, 0, 0),
	m_playerColor(255, 0, 0),
	m_drawOrigin(false),
	m_drawPlayers(false),
	m_drawScale(false),
	m_drawAlpha(false),
	m_shading(true),
	m_leaflet(false),
	m_dontWriteEmpty(false),
	m_buildPyramid(false),
	m_backend(""),
	m_xBorder(0),
	m_yBorder(0),
	m_db(NULL),
	m_image(NULL),
	m_xMin(INT_MAX),
	m_xMax(INT_MIN),
	m_zMin(INT_MAX),
	m_zMax(INT_MIN),
	m_yMin(-30000),
	m_yMax(30000),
	m_geomX(-2048),
	m_geomY(-2048),
	m_geomX2(2048),
	m_geomY2(2048),
	m_tileW(INT_MAX),
	m_tileH(INT_MAX),
	m_zoom(1),
	m_scales(SCALE_LEFT | SCALE_TOP)
{
}

TileGenerator::~TileGenerator()
{
	closeDatabase();
}

void TileGenerator::setBgColor(const std::string &bgColor)
{
	m_bgColor = parseColor(bgColor);
}

void TileGenerator::setScaleColor(const std::string &scaleColor)
{
	m_scaleColor = parseColor(scaleColor);
}

void TileGenerator::setOriginColor(const std::string &originColor)
{
	m_originColor = parseColor(originColor);
}

void TileGenerator::setPlayerColor(const std::string &playerColor)
{
	m_playerColor = parseColor(playerColor);
}

void TileGenerator::setZoom(int zoom)
{
	if (zoom < 1)
		throw std::runtime_error("Zoom level needs to be a number: 1 or higher");
	m_zoom = zoom;
}

void TileGenerator::setScales(uint flags)
{
	m_scales = flags;
}

Color TileGenerator::parseColor(const std::string &color)
{
	Color parsed;
	if (color.length() != 7)
		throw std::runtime_error("Color needs to be 7 characters long");
	if (color[0] != '#')
		throw std::runtime_error("Color needs to begin with #");
	unsigned long col = strtoul(color.c_str() + 1, NULL, 16);
	parsed.b = col & 0xff;
	parsed.g = (col >> 8) & 0xff;
	parsed.r = (col >> 16) & 0xff;
	parsed.a = 255;
	return parsed;
}

void TileGenerator::setDrawOrigin(bool drawOrigin)
{
	m_drawOrigin = drawOrigin;
}

void TileGenerator::setDrawPlayers(bool drawPlayers)
{
	m_drawPlayers = drawPlayers;
}

void TileGenerator::setDrawScale(bool drawScale)
{
	m_drawScale = drawScale;
}

void TileGenerator::setDrawAlpha(bool drawAlpha)
{
	m_drawAlpha = drawAlpha;
}

void TileGenerator::setShading(bool shading)
{
	m_shading = shading;
}

void TileGenerator::setLeaflet(bool leaflet)
{
	m_leaflet = leaflet;
}

void TileGenerator::setBuildPyramid(bool pyramid)
{
	m_buildPyramid = pyramid;
}

void TileGenerator::setBackend(std::string backend)
{
	m_backend = backend;
}

void TileGenerator::setGeometry(int x, int y, int w, int h)
{
	m_geomX  = round_multiple_nosign(x, 16) / 16;
	m_geomY  = round_multiple_nosign(y, 16) / 16;
	m_geomX2 = round_multiple_nosign(x + w, 16) / 16;
	m_geomY2 = round_multiple_nosign(y + h, 16) / 16;
}

void TileGenerator::setTileSize(int w, int h)
{
	m_tileW = round_multiple_nosign(w, 16) / 16;
	m_tileH = round_multiple_nosign(h, 16) / 16;
}


void TileGenerator::setMinY(int y)
{
	m_yMin = y;
}

void TileGenerator::setMaxY(int y)
{
	m_yMax = y;
}

void TileGenerator::parseColorsFile(const std::string &fileName)
{
	ifstream in;
	in.open(fileName.c_str(), ifstream::in);
	if (!in.is_open())
		throw std::runtime_error("Specified colors file could not be found");
	parseColorsStream(in);
}

void TileGenerator::printGeometry(const std::string &input)
{
	string input_path = input;
	if (input_path[input.length() - 1] != PATH_SEPARATOR) {
		input_path += PATH_SEPARATOR;
	}

	openDb(input_path);
	loadBlocks();

	std::cout << "Map extent: "
		<< m_xMin*16 << ":" << m_zMin*16
		<< "+" << (m_xMax - m_xMin+1)*16
		<< "+" << (m_zMax - m_zMin+1)*16
		<< std::endl;

	closeDatabase();

}

void TileGenerator::setDontWriteEmpty(bool f)
{
	m_dontWriteEmpty = f;
}

void TileGenerator::addMarker(std::string marker)
{
	m_markers.insert(marker);
}


void TileGenerator::generate(const std::string &input, const std::string &output)
{

	string input_path = input;
	if (input_path[input.length() - 1] != PATH_SEPARATOR) {
		input_path += PATH_SEPARATOR;
	}

	openDb(input_path);
	loadBlocks();

	if (m_dontWriteEmpty  && ! m_positions.size())
	{
		closeDatabase();
		return;
	}

	createImage();


	if (m_tileW < INT_MAX || m_tileH < INT_MAX)
	{
		int minTileX = 0;
		int minTileZ = 0;

		int flipY = 1;
		if (m_leaflet)
		{
			// for leaflet tiles, it's nice if the origin is on the corner between tile (-1,-1) and tile (0,0)
			m_xMin = round_multiple_nosign(m_xMin, m_tileW);
			m_zMin = round_multiple_nosign(m_zMin, m_tileH);
			// for some reason the tile indices in leaflet are calculated with -(y/tilesize) while x is calculated with (x/tilesize)
			// so in tile indices +y is pointing down, while in map coordinates it is pointing up (go figure, but whatever)
			// this means tile 0_0_ is the tile with the origin in the top-left corner.
			minTileX = m_xMin / m_tileW;
			minTileZ = m_zMin / m_tileH + 1;
			flipY = -1;
		}

		sortPositionsIntoTiles();

		// round xMax/zMax tot integer number of tiles
		m_xMax = m_xMin + m_numTilesX * m_tileW;
		m_zMax = m_zMin + m_numTilesY * m_tileH;

		int maxZoomLevel = 0;
		if (m_buildPyramid)
		{
			int maxDimX = m_xMax > -m_xMin ? m_xMax : -m_xMin;
			int maxDimY = m_zMax > -m_zMin ? m_zMax : -m_zMin;
			int maxDim = maxDimX > maxDimY ? maxDimX : maxDimY;

			maxZoomLevel =  static_cast<int>(log2(maxDim / m_tileW)) + 1;
		}



		int trueXMin = m_xMin;
		int trueZMin = m_zMin;

		for (int x = 0; x < m_numTilesX; x++)
		{
			for (int y = 0; y < m_numTilesY; y++)
			{
				TileMap::iterator t = m_tiles.find(x + (y << 16));
				m_xMin = trueXMin + x * m_tileW;
				m_zMin = trueZMin + y * m_tileH;
				m_xMax = m_xMin + m_tileW - 1;
				m_zMax = m_zMin + m_tileH -1;

				if (t != m_tiles.end() || !m_dontWriteEmpty)
				{
					m_image->fill(m_bgColor);
					if (t != m_tiles.end())
						renderMap(t->second);
					if (m_drawScale) {
						renderScale();
					}
					if (m_drawOrigin) {
						renderOrigin();
					}
					if (m_drawPlayers) {
						renderPlayers(input_path);
					}
					ostringstream fn;
					if (m_leaflet)
					{
						fn << maxZoomLevel << "_";
					}
					fn << x + minTileX << '_' << (flipY * (y + minTileZ)) << '_' << output;
					writeImage(fn.str());
					if (m_buildPyramid)
					{
						m_availableTiles.insert(Coords(x + minTileX,(flipY * (y + minTileZ))));
					}
				}
			}
		}
		if (m_leaflet)
		{
			ostringstream fn;
			fn << "empty_tile_" << output;
			m_image->fill(m_bgColor);
			writeImage(fn.str());
		}
		if (m_buildPyramid)
		{
			buildPyramid(output, maxZoomLevel);
		}
		if (m_leaflet)
		{
			outputLeafletCode(output, maxZoomLevel);
		}

	}
	else
	{

		m_image->fill(m_bgColor);
		renderMap(m_positions);
		if (m_drawScale) {
			renderScale();
		}
		if (m_drawOrigin) {
			renderOrigin();
		}
		if (m_drawPlayers) {
			renderPlayers(input_path);
		}
		writeImage(output);
	}
	closeDatabase();
	printUnknown();

	delete m_image;
	m_image = NULL;
}

void TileGenerator::parseColorsStream(std::istream &in)
{
	char line[128];
	while (in.good()) {
		in.getline(line, 128);

		for(char *p = line; *p; p++) {
			if(*p != '#')
				continue;
			*p = '\0'; // Cut off at the first #
			break;
		}
		if(strlen(line) == 0)
			continue;

		char name[64 + 1];
		unsigned int r, g, b, a, t;
		a = 255;
		t = 0;
		int items = sscanf(line, "%64s %u %u %u %u %u", name, &r, &g, &b, &a, &t);
		if(items < 4) {
			std::cerr << "Failed to parse color entry '" << line << "'" << std::endl;
			continue;
		}
	
		ColorEntry color = ColorEntry(r, g, b, a, t);
		m_colorMap[name] = color;
	}
}

void TileGenerator::openDb(const std::string &input)
{
	std::string backend = m_backend;
	if(backend == "") {
		std::ifstream ifs((input + "/world.mt").c_str());
		if(!ifs.good())
			throw std::runtime_error("Failed to read world.mt");
		backend = read_setting("backend", ifs);
		ifs.close();
	}

	if(backend == "sqlite3")
		m_db = new DBSQLite3(input);
#if USE_POSTGRESQL
	else if(backend == "postgresql")
		m_db = new DBPostgreSQL(input);
#endif
#if USE_LEVELDB
	else if(backend == "leveldb")
		m_db = new DBLevelDB(input);
#endif
#if USE_REDIS
	else if(backend == "redis")
		m_db = new DBRedis(input);
#endif
	else
		throw std::runtime_error(((std::string) "Unknown map backend: ") + backend);
}

void TileGenerator::closeDatabase()
{
	delete m_db;
	m_db = NULL;
}

void TileGenerator::loadBlocks()
{
	std::vector<BlockPos> vec = m_db->getBlockPos();
	for (std::vector<BlockPos>::iterator it = vec.begin(); it != vec.end(); ++it) {
		BlockPos pos = *it;
		// Check that it's in geometry (from --geometry option)
		if (pos.x < m_geomX || pos.x >= m_geomX2 || pos.z < m_geomY || pos.z >= m_geomY2)
			continue;
		// Check that it's between --min-y and --max-y
		if (pos.y * 16 < m_yMin || pos.y * 16 > m_yMax)
			continue;
		// Adjust minimum and maximum positions to the nearest block
		if (pos.x < m_xMin)
			m_xMin = pos.x;
		if (pos.x > m_xMax)
			m_xMax = pos.x;

		if (pos.z < m_zMin)
			m_zMin = pos.z;
		if (pos.z > m_zMax)
			m_zMax = pos.z;
		m_positions.push_back(std::pair<int, int>(pos.x, pos.z));
	}
	m_positions.sort();
	m_positions.unique();
}

void TileGenerator::createImage()
{
	const int scale_d = 40; // pixels reserved for a scale
	if(!m_drawScale)
		m_scales = 0;


	// If a geometry is explicitly set, set the bounding box to the requested geometry
	// instead of cropping to the content. This way we will always output a full tile
	// of the correct size.
	if (m_geomX > -2048 && m_geomX2 < 2048)
	{
		m_xMin = m_geomX;
		m_xMax = m_geomX2-1;
	}

	if (m_geomY > -2048 && m_geomY2 < 2048)
	{
		m_zMin = m_geomY;
		m_zMax = m_geomY2-1;
	}

	m_mapWidth = (m_xMax - m_xMin + 1);
	m_mapHeight = (m_zMax - m_zMin + 1);

	if (m_tileW < INT_MAX)
		m_mapWidth = m_tileW;

	if (m_tileH < INT_MAX)
		m_mapHeight = m_tileH;

	m_mapWidth *= 16;
	m_mapHeight *= 16;

	m_xBorder = (m_scales & SCALE_LEFT) ? scale_d : 0;
	m_yBorder = (m_scales & SCALE_TOP) ? scale_d : 0;
	m_blockPixelAttributes.setWidth(m_mapWidth);

	int image_width, image_height;
	image_width = (m_mapWidth * m_zoom) + m_xBorder;
	image_width += (m_scales & SCALE_RIGHT) ? scale_d : 0;
	image_height = (m_mapHeight * m_zoom) + m_yBorder;
	image_height += (m_scales & SCALE_BOTTOM) ? scale_d : 0;

	if(image_width > 4096 || image_height > 4096)
		std::cerr << "Warning: The width or height of the image to be created exceeds 4096 pixels!"
			<< " (Dimensions: " << image_width << "x" << image_height << ")"
			<< std::endl;
	m_image = new Image(image_width, image_height);
	m_image->drawFilledRect(0, 0, image_width, image_height, m_bgColor); // Background
}

void TileGenerator::renderMap(PositionsList &positions)
{
	BlockDecoder blk(m_markers.size() > 0);
	std::list<int> zlist = getZValueList(positions);
	for (std::list<int>::iterator zPosition = zlist.begin(); zPosition != zlist.end(); ++zPosition) {
		int zPos = *zPosition;
		std::map<int16_t, BlockList> blocks;
		m_db->getBlocksOnZ(blocks, zPos);
		for (PositionsList::const_iterator position = positions.begin(); position != positions.end(); ++position) {
			if (position->second != zPos)
				continue;

			m_readPixels.reset();
			m_readInfo.reset();
			for (int i = 0; i < 16; i++) {
				for (int j = 0; j < 16; j++) {
					m_color[i][j] = m_bgColor; // This will be drawn by renderMapBlockBottom() for y-rows with only 'air', 'ignore' or unknown nodes if --drawalpha is used
					m_color[i][j].a = 0; // ..but set alpha to 0 to tell renderMapBlock() not to use this color to mix a shade
					m_thickness[i][j] = 0;
				}
			}

			int xPos = position->first;
			blocks[xPos].sort();
			const BlockList &blockStack = blocks[xPos];
			for (BlockList::const_iterator it = blockStack.begin(); it != blockStack.end(); ++it) {
				const BlockPos &pos = it->first;

				blk.reset();
				blk.decode(it->second);
				if (blk.isEmpty())
					continue;
				renderMapBlock(blk, pos);

				// Exit out if all pixels for this MapBlock are covered
				if (m_readPixels.full())
					break;
			}
			if (!m_readPixels.full())
				renderMapBlockBottom(blockStack.begin()->first);
		}
		if (m_shading)
			renderShading(zPos);
	}
}

void TileGenerator::renderMapBlock(const BlockDecoder &blk, const BlockPos &pos)
{
	int xBegin = (pos.x - m_xMin) * 16;
	int zBegin = (m_zMax - pos.z) * 16;
	int minY = (pos.y * 16 > m_yMin) ? 0 : m_yMin - pos.y * 16;
	int maxY = (pos.y * 16 < m_yMax) ? 15 : m_yMax - pos.y * 16;
	for (int z = 0; z < 16; ++z) {
		int imageY = zBegin + 15 - z;
		for (int x = 0; x < 16; ++x) {
			if (m_readPixels.get(x, z))
				continue;
			int imageX = xBegin + x;

			for (int y = maxY; y >= minY; --y) {
				string name = blk.getNode(x, y, z);
				if (name == "")
					continue;

				if (m_markers.count(name))
				{
					cout << "Marker: " << name << " " << (pos.x*16 + x) << " " << (pos.y * 16 + y) << " " << (pos.z * 16 + z) << endl;
					BlockDecoder::NodeMetaData const & nm = blk.getNodeMetaData(x,y,z);
					for (BlockDecoder::NodeMetaData::const_iterator i = nm.begin(); i != nm.end(); i++)
					{
						cout << "Marker: \"" << i->first << '"' << ":" <<   '"' << i->second << '"' << endl;
					}
				}

				ColorMap::const_iterator it = m_colorMap.find(name);
				if (it == m_colorMap.end()) {
					m_unknownNodes.insert(name);
					continue;
				}
				const Color c = it->second.to_color();
				if (m_drawAlpha) {
					if (m_color[z][x].a == 0)
						m_color[z][x] = c; // first visible time, no color mixing
					else
						m_color[z][x] = mixColors(m_color[z][x], c);
					if(m_color[z][x].a < 0xff) {
						// near thickness value to thickness of current node
						m_thickness[z][x] = (m_thickness[z][x] + it->second.t) / 2.0;
						continue;
					}
					// color became opaque, draw it
					setZoomed(imageX, imageY, m_color[z][x]);
					m_blockPixelAttributes.attribute(15 - z, xBegin + x).thickness = m_thickness[z][x];
				} else {
					setZoomed(imageX, imageY, c.noAlpha());
				}
				m_readPixels.set(x, z);

				// do this afterwards so we can record height values
				// inside transparent nodes (water) too
				if (!m_readInfo.get(x, z)) {
					m_blockPixelAttributes.attribute(15 - z, xBegin + x).height = pos.y * 16 + y;
					m_readInfo.set(x, z);
				}
				break;
			}
		}
	}
}

void TileGenerator::renderMapBlockBottom(const BlockPos &pos)
{
	if (!m_drawAlpha)
		return; // "missing" pixels can only happen with --drawalpha

	int xBegin = (pos.x - m_xMin) * 16;
	int zBegin = (m_zMax - pos.z) * 16;
	for (int z = 0; z < 16; ++z) {
		int imageY = zBegin + 15 - z;
		for (int x = 0; x < 16; ++x) {
			if (m_readPixels.get(x, z))
				continue;
			int imageX = xBegin + x;

			// set color since it wasn't done in renderMapBlock()
			setZoomed(imageX, imageY, m_color[z][x]);
			m_readPixels.set(x, z);
			m_blockPixelAttributes.attribute(15 - z, xBegin + x).thickness = m_thickness[z][x];
		}
	}
}

void TileGenerator::renderShading(int zPos)
{
	int zBegin = (m_zMax - zPos) * 16;
	for (int z = 0; z < 16; ++z) {
		int imageY = zBegin + z;
		if (imageY >= m_mapHeight)
			continue;
		for (int x = 0; x < m_mapWidth; ++x) {
			if(
				!m_blockPixelAttributes.attribute(z, x).valid_height() ||
				!m_blockPixelAttributes.attribute(z, x - 1).valid_height() ||
				!m_blockPixelAttributes.attribute(z - 1, x).valid_height()
			)
				continue;

			// calculate shadow to apply
			int y = m_blockPixelAttributes.attribute(z, x).height;
			int y1 = m_blockPixelAttributes.attribute(z, x - 1).height;
			int y2 = m_blockPixelAttributes.attribute(z - 1, x).height;
			int d = ((y - y1) + (y - y2)) * 12;
			if (m_drawAlpha) { // less visible shadow with increasing "thickness"
				double t = m_blockPixelAttributes.attribute(z, x).thickness * 1.2;
				d *= 1.0 - mymin(t, 255.0) / 255.0;
			}
			d = mymin(d, 36);

			Color c = m_image->getPixel(getImageX(x), getImageY(imageY));
			c.r = colorSafeBounds(c.r + d);
			c.g = colorSafeBounds(c.g + d);
			c.b = colorSafeBounds(c.b + d);
			setZoomed(x, imageY, c);
		}
	}
	m_blockPixelAttributes.scroll();
}

void TileGenerator::renderScale()
{
	const int scale_d = 40; // see createImage()

	if (m_scales & SCALE_TOP) {
		m_image->drawText(24, 0, "X", m_scaleColor);
		for (int i = (m_xMin / 4) * 4; i <= m_xMax; i += 4) {
			std::ostringstream buf;
			buf << i * 16;

			int xPos = getImageX(i * 16, true);
			if (xPos >= 0) {
				m_image->drawText(xPos + 2, 0, buf.str(), m_scaleColor);
				m_image->drawLine(xPos, 0, xPos, m_yBorder - 1, m_scaleColor);
			}
		}
	}

	if (m_scales & SCALE_LEFT) {
		m_image->drawText(2, 24, "Z", m_scaleColor);
		for (int i = (m_zMax / 4) * 4; i >= m_zMin; i -= 4) {
			std::ostringstream buf;
			buf << i * 16;

			int yPos = getImageY(i * 16 + 1, true);
			if (yPos >= 0) {
				m_image->drawText(2, yPos, buf.str(), m_scaleColor);
				m_image->drawLine(0, yPos, m_xBorder - 1, yPos, m_scaleColor);
			}
		}
	}

	if (m_scales & SCALE_BOTTOM) {
		int xPos = m_xBorder + m_mapWidth*m_zoom - 24 - 8,
			yPos = m_yBorder + m_mapHeight*m_zoom + scale_d - 12;
		m_image->drawText(xPos, yPos, "X", m_scaleColor);
		for (int i = (m_xMin / 4) * 4; i <= m_xMax; i += 4) {
			std::ostringstream buf;
			buf << i * 16;

			xPos = getImageX(i * 16, true);
			yPos = m_yBorder + m_mapHeight*m_zoom;
			if (xPos >= 0) {
				m_image->drawText(xPos + 2, yPos, buf.str(), m_scaleColor);
				m_image->drawLine(xPos, yPos, xPos, yPos + 39, m_scaleColor);
			}
		}
	}

	if (m_scales & SCALE_RIGHT) {
		int xPos = m_xBorder + m_mapWidth*m_zoom + scale_d - 2 - 8,
			yPos = m_yBorder + m_mapHeight*m_zoom - 24 - 12;
		m_image->drawText(xPos, yPos, "Z", m_scaleColor);
		for (int i = (m_zMax / 4) * 4; i >= m_zMin; i -= 4) {
			std::ostringstream buf;
			buf << i * 16;

			xPos = m_xBorder + m_mapWidth*m_zoom;
			yPos = getImageY(i * 16 + 1, true);
			if (yPos >= 0) {
				m_image->drawText(xPos + 2, yPos, buf.str(), m_scaleColor);
				m_image->drawLine(xPos, yPos, xPos + 39, yPos, m_scaleColor);
			}
		}
	}
}

void TileGenerator::renderOrigin()
{
	if (m_xMin > 0 || m_xMax < 0 ||
		m_zMin > 0 || m_zMax < 0)
		return;
	m_image->drawCircle(getImageX(0, true), getImageY(0, true), 12, m_originColor);
}

void TileGenerator::renderPlayers(const std::string &inputPath)
{
	PlayerAttributes players(inputPath);
	for (PlayerAttributes::Players::iterator player = players.begin(); player != players.end(); ++player) {
		if (player->x < m_xMin*16 || player->x > m_xMax * 16 ||
			player->z < m_zMin*16 || player->z > m_zMax * 16 )
		{
			continue;

		}
		if (player->y < m_yMin || player->y > m_yMax)
			continue;
		int imageX = getImageX(player->x, true),
			imageY = getImageY(player->z, true);

		m_image->drawFilledRect(imageX - 1, imageY, 3, 1, m_playerColor);
		m_image->drawFilledRect(imageX, imageY - 1, 1, 3, m_playerColor);
		m_image->drawText(imageX + 2, imageY, player->name, m_playerColor);
	}
}

inline std::list<int> TileGenerator::getZValueList(PositionsList &positions) const
{
	std::list<int> zlist;
	for (PositionsList::const_iterator position = positions.begin(); position != positions.end(); ++position)
		zlist.push_back(position->second);
	zlist.sort();
	zlist.unique();
	zlist.reverse();
	return zlist;
}

void TileGenerator::writeImage(const std::string &output)
{
	m_image->save(output);
	cout << "wrote image:" << output << endl;
}

void TileGenerator::printUnknown()
{
	if (m_unknownNodes.size() == 0)
		return;
	std::cerr << "Unknown nodes:" << std::endl;
	for (NameSet::iterator node = m_unknownNodes.begin(); node != m_unknownNodes.end(); ++node)
		std::cerr << "\t" << *node << std::endl;
}

inline int TileGenerator::getImageX(int val, bool absolute) const
{
	if (absolute)
		val = (val - m_xMin * 16);
	return (m_zoom*val) + m_xBorder;
}

inline int TileGenerator::getImageY(int val, bool absolute) const
{
	if (absolute)
		val = m_mapHeight - (val - m_zMin * 16); // Z axis is flipped on image
	return (m_zoom*val) + m_yBorder;
}

inline void TileGenerator::setZoomed(int x, int y, Color color)
{
	m_image->drawFilledRect(getImageX(x), getImageY(y), m_zoom, m_zoom, color);
}


void TileGenerator::sortPositionsIntoTiles()
{
	m_numTilesX = round_multiple_nosign(m_xMax - m_xMin + 1, m_tileW) / m_tileW;
	m_numTilesY = round_multiple_nosign(m_zMax - m_zMin + 1, m_tileH) / m_tileH;

	for (PositionsList::iterator p = m_positions.begin(); p != m_positions.end(); p++)
	{
		int xtile = (p->first - m_xMin) / m_tileW;
		int ytile = (p->second - m_zMin) / m_tileH;

		int key = xtile + (ytile << 16);

		TileMap::iterator t = m_tiles.find(key);

		if (t == m_tiles.end())
		{
			PositionsList l;
			m_tiles.insert(std::pair<int, PositionsList>(key, l));
			t = m_tiles.find(key);
		}

		t->second.push_back(std::pair<int, int>(p->first, p->second));
	}
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


void TileGenerator::outputLeafletCode(std::string const &output, int maxLevel)
{
	if (m_tileH != m_tileW)
	{
		throw std::runtime_error("For a leaflet map the tiles must be square!");
		return;
	}

	if (m_tileH == INT_MAX)
	{
		throw std::runtime_error("Can only use --leaflet with --tilesize!");
		return;
	}

	ostringstream fn;
	fn << output << ".html";

	// I use fopen instead of ostr because I need fprintf to put the correct values into the
	// html static string above. ostream is nice and C++ and all, but formatted output handling
	// for streams is retarded. Use the best tool for the job.
	FILE *out = fopen(fn.str().c_str(), "w");

	if (!out)
	{
		cout << "error opening file:" << fn.str() << endl;
		return;
	}

	fprintf(out, leafletMapHtml, maxLevel, output.c_str(), maxLevel, m_tileW*16, output.c_str(), 1.0/pow(2,maxLevel));

	fclose(out);
}

void TileGenerator::buildPyramid(std::string const &fileName, int startLevel)
{
	TileSet tilesToGenerate;
	int level = startLevel;

	while (true)
	{
		for (TileSet::iterator t = m_availableTiles.begin(); t != m_availableTiles.end(); t++)
		{
			int x = t->first;
			int y = t->second;
			if (x >=0)
				x++;
			if (y >=0)
				y++;
			x = round_multiple_nosign(x, 2) / 2;
			y = round_multiple_nosign(y, 2) / 2;
			if (x > 0)
				x--;
			if (y > 0)
				y--;

			tilesToGenerate.insert(Coords(x, y));
		}

		int halfW = m_mapWidth/2;
		int halfH = m_mapHeight/2;
		for (TileSet::iterator g = tilesToGenerate.begin(); g != tilesToGenerate.end(); g++)
		{
			m_image->fill(m_bgColor);
			int x = g->first;
			int y = g->second;
			for (int i = 0; i < 2 ; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					if (m_availableTiles.find(Coords(2*x+i, 2*y+j)) != m_availableTiles.end())
					{
						std::ostringstream f;
						f << level << "_" << (2*x+i) << "_" << (2*y+j) << "_" << fileName;
						Image src(f.str());
						src.scaleBlit(m_image, i*halfW, j*halfH, halfW, halfH);
					}
				}
			}
			std::ostringstream f;
			f << (level-1) << "_" << x << "_" << y << "_" << fileName;
			m_image->save(f.str());
			cout << "Wrote image " << f.str() << endl;
		}

		std::cout << "generated pyramid level " << (level -1) << endl;
		level--;

		if (!level)
		{
			return; // we're done
		}

		m_availableTiles = tilesToGenerate;
		tilesToGenerate.clear();

	}

}
