#include <cstdio>
#include <cstdlib>
#include <climits>
#include <fstream>
#include <iostream>
#include <ostream>
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
	m_dontWriteEmpty(false),
	m_isometric(false),
	m_backend(""),
	m_xBorder(0),
	m_yBorder(0),
	m_db(NULL),
	m_image(NULL),
	m_xMin(INT_MAX),
	m_xMax(INT_MIN),
	m_zMin(INT_MAX),
	m_zMax(INT_MIN),
	m_yMin(INT_MAX),
	m_yMax(INT_MIN),
	m_geomX(-2048),
	m_geomY(-2048),
	m_geomX2(2048),
	m_geomY2(2048),
	m_geomH1(-30000),
	m_geomH2(30000),
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

void TileGenerator::setIsometric(bool iso)
{
	m_isometric = iso;
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
	m_geomH1 = y;
}

void TileGenerator::setMaxY(int y)
{
	m_geomH2 = y;
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
	std::pair<int,int> maxImSize(0,0);

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
		m_xMin = round_multiple_nosign(m_xMin, m_tileW);
		m_zMin = round_multiple_nosign(m_zMin, m_tileH);
		int minTileX = m_xMin / m_tileW;
		int minTileY = m_zMin / m_tileH;

		sortPositionsIntoTiles();

		int trueXMin = m_xMin;
		int trueZMin = m_zMin;

		// write info about the number of tiles and the tile sizes to a text file
		// which can be used by another utility to generate zoom pyramids and/or
		// add map annotations for a specific viewer.
		std::ostringstream mfn;
		mfn << "metadata_" << output << ".txt";
		std::ofstream mf;

		mf.open(mfn.str().c_str());

		if (mf.is_open())
		{

			mf << "BaseName: " << output << std::endl;
			mf << "NumTiles: " << m_numTilesX << " " << m_numTilesY << std::endl;
			mf << "MinTile: " << minTileX << " " << minTileY << std::endl;
			mf << "TileSize: " << (m_tileW*16) << " " << (m_tileH*16) << std::endl;
			mf << "Zoom: " << m_zoom << std::endl;
			mf.close();
		}
		else
		{
			std::cerr << "Warning: could not write to '" << mfn.str() << "'!" << std::endl;
		}

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
					fn << "tile_" << (x + minTileX) << '_' << (y + minTileY) << '_' << output;
					writeImage(fn.str());

					if (t!=m_tiles.end() && m_isometric) {
						std::pair<int,int> imSize = renderMapIsometric("iso_" + fn.str(), m_tileW, t->second, m_zoom);

						maxImSize.first = maxImSize.first > imSize.first ? maxImSize.first : imSize.first;
						maxImSize.second = maxImSize.second > imSize.second ? maxImSize.second : imSize.second;
					}
				}
			}
		}

		if (m_isometric)
		{
			ostringstream fn;
			fn << "iso_metadata_" << output << ".txt";

			std::ofstream os;
			os.open(fn.str(), std::ios::out);

			os << "BaseName: " << output << std::endl;
			os << "NumTiles: " << m_numTilesX << " " << m_numTilesY << std::endl;
			os << "MinTile: " << minTileX << " " << minTileY << std::endl;
			os << "TileSize: " << (m_tileW*16) << " " << (m_tileH*16) << std::endl;
			os << "Zoom: " << m_zoom << std::endl;
			os << "MaxImageSize: " << maxImSize.first << " " <<maxImSize.second << std::endl;
			os << "OriginHeight: " << (int)((-m_yMin) * ((4*m_zoom * 2/sqrt(3)) - (int)(m_zoom * 2/sqrt(3))) + (m_tileW * 16 + 1) * m_zoom * 2 / sqrt(3)) << std::endl; // urgh. I really shouldn't repeat this calculation here, but make a proper transform function which is used by both the iso drawing and this

			os.flush();

			os.close();
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
		if (pos.y * 16 < m_geomH1 || pos.y * 16 > m_geomH2)
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

		if (pos.y < m_yMin)
			m_yMin = pos.y;
		if (pos.y > m_yMax)
			m_yMax = pos.y;

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
	int minY = (pos.y * 16 > m_geomH1) ? 0 : m_geomH1 - pos.y * 16;
	int maxY = (pos.y * 16 < m_geomH2) ? 15 : m_geomH2 - pos.y * 16;
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
		if (player->y < m_geomH1 || player->y > m_geomH2)
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

// returns lowest y written (for cropping the image)
static int IsoColoredCube(Image *im, int x, int y, int z, int scale, Color const&c, int yShift)
{
	static double yf = 2 / sqrt(3);

	int xcube[][4] =
	{
		{0,0, 2*scale, 2*scale},
		{2*scale, 2*scale, 4*scale, 4*scale},
		{0, 2 * scale, 4*scale, 2*scale}
	};



	int dy = (int)(scale * yf);
	int dy2 = (int)(scale * 4 * yf)  - 2 * dy; // spread the rounding error of dy to dy2 so the toal height is closer to the correct height
	int cubeH = dy*2 + dy2;

	int ycube[][4] =
	{
		{dy, dy+dy2, dy2, 0},
		{0, dy2, dy2+dy , dy},
		{dy+dy2, 2*dy+dy2, dy+dy2, dy2}
	};

	int ix = (x + z)*scale*2; // total width = 4*scale
	int iy = y * dy2 + (z-x)*dy;
	static Image *cube = NULL;
	static Color prevColor(0,0,0,0);

	int ih = im->GetHeight();

	if (cube && cube->GetHeight() != cubeH)
	{
		delete cube;
		cube = NULL;
		prevColor.a = 0;
	}

	if (!cube)
	{
		cube = new Image(4*scale, cubeH);
		static const Color transparent(0,0,0,0);
		cube->fill(transparent, true);
	}

	if (prevColor != c)
	{
		static double const brightness[] { 0.8, 0.6, 1.0 };
		ImagePoint points[4];
		for (int quad = 0; quad < 3; quad++)
		{
			for (int p = 0; p < 4; p++)
			{
				points[p].x = xcube[quad][p];
				points[p].y = cube->GetHeight() - 1 - ycube[quad][p];
			}
			Color faceCol = c.gamma(brightness[quad]);
			if (faceCol.a < 255)
			{
				faceCol.a /=4; //! hack to make stuff more transparent, because ismetric layers a lot more than flat
			}
			cube->drawFilledPolygon(4, points, faceCol, true);
		}
		prevColor = c;
//		cube->save("test.png");
	}
	cube->blit(im,ix, ih -1 -iy - yShift);

	return ih - 1 - iy -yShift;

}

int TileGenerator::renderMapBlockIsometric(BlockDecoder const &blk, BlockPos const &pos, Image *to, int scale, int yShift)
{
	int minY = INT_MAX;
	for (int z = 15; z >=0; z--) {
		for (int x = 0; x < 16 ; x++) {

			for (int y = 0; y < 16; y++) {
				string name = blk.getNode(x, y, z);
				if (name == "")
					continue;
				ColorMap::const_iterator it = m_colorMap.find(name);

				Color c(1,1,1);
				if (it != m_colorMap.end()) {
					c = it->second.to_color();
				}

				int h = pos.y*16+y - m_yMin;

				c = c.gamma(.25 +h/100.0);
				int my = IsoColoredCube(to, pos.x*16 + x, pos.y*16+y, pos.z*16+z, scale, c, yShift);

				minY = my < minY ? my : minY;
			}
		}
	}

	return minY;

}

std::pair<int,int> TileGenerator::renderMapIsometric(std::string const &fileName, int tileSize, PositionsList &positions,int scale)
{
	Image image(tileSize * 16 * scale*4, (tileSize + (m_yMax - m_yMin + 1)) * 16 * scale * 4 /sqrt(3));
	int yShift = (tileSize * 16 + 1) * scale * 2 / sqrt(3);
	Color transparent(0,0,0,0);
	image.fill(transparent, true);
	BlockDecoder blk;
	std::list<int> zlist = getZValueList(positions);

	int minY = INT_MAX;
	for (std::list<int>::iterator zPosition = zlist.begin(); zPosition != zlist.end(); ++zPosition) {
		int zPos = *zPosition;
		std::map<int16_t, BlockList> blocks;
		std::cout << 'Z';
		m_db->getBlocksOnZ(blocks, zPos);
		for (PositionsList::const_iterator position = positions.begin(); position != positions.end(); ++position) {
			if (position->second != zPos)
				continue;
			std::cout << 'X'  << std::flush;

			int xPos = position->first;
			blocks[xPos].sort();
			const BlockList &blockStack = blocks[xPos];
			for (BlockList::const_reverse_iterator it = blockStack.rbegin(); it != blockStack.rend(); ++it) {
				BlockPos pos = it->first;

				if (pos.y * 16 < m_geomH1 || pos.y * 16 > m_geomH2)
					continue;

				pos.y -= m_yMin;
				pos.z -= m_zMin;
				pos.x -= m_xMin;

				blk.reset();
				blk.decode(it->second);
				if (blk.isEmpty())
					continue;

				int my = renderMapBlockIsometric(blk, pos, &image, scale, yShift);

				minY = my < minY ? my : minY;
			}
		}
		std::cout << std::endl;
	}

	if (minY < INT_MAX)
	{
		image.crop(0, minY, image.GetWidth(), image.GetHeight());
		image.save(fileName);
		std::cout << "Wrote iso " << fileName << std::endl;

		return std::pair<int,int>(image.GetWidth(), image.GetHeight());
	}

	return std::pair<int,int>(0,0);
}


