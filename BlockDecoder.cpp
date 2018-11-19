#include <stdint.h>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>

#include "BlockDecoder.h"
#include "ZlibDecompressor.h"

static inline uint16_t readU16(const unsigned char *data)
{
	return data[0] << 8 | data[1];
}

static inline uint32_t readU32(const unsigned char *data)
{
	return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}


static int readBlockContent(const unsigned char *mapData, u8 version, unsigned int datapos)
{
	if (version >= 24) {
		size_t index = datapos << 1;
		return (mapData[index] << 8) | mapData[index + 1];
	} else if (version >= 20) {
		if (mapData[datapos] <= 0x80)
			return mapData[datapos];
		else
			return (int(mapData[datapos]) << 4) | (int(mapData[datapos + 0x2000]) >> 4);
	}
	std::ostringstream oss;
	oss << "Unsupported map version " << version;
	throw std::runtime_error(oss.str());
}

BlockDecoder::BlockDecoder(bool withMetaData)
	: m_withMetaData(withMetaData)
{
	reset();
}

void BlockDecoder::reset()
{
	m_blockAirId = -1;
	m_blockIgnoreId = -1;
	m_nameMap.clear();

	m_metaData.clear();
	m_version = 0;
	m_mapData = ustring();
}


static inline bool CheckRange(void const *p, void const *end, char const *msg)
{
	if (p > end)
	{
		std::cout << msg << std::endl;
		return false;
	}

	return true;
}
#define CHECKMD(range) (m_metaDataValid = CheckRange(md + range, metaDataEnd, "Premature end of metadata block, incomplete or currupted metadata!"))

void BlockDecoder::decode(const ustring &datastr)
{
	const unsigned char *data = datastr.c_str();
	size_t length = datastr.length();
	// TODO: bounds checks

	uint8_t version = data[0];
	//uint8_t flags = data[1];
	m_version = version;

	size_t dataOffset = 0;
	if (version >= 27)
		dataOffset = 6;
	else if (version >= 22)
		dataOffset = 4;
	else
		dataOffset = 2;

	ZlibDecompressor decompressor(data, length);
	decompressor.setSeekPos(dataOffset);
	m_mapData = decompressor.decompress();
	ustring metaData = decompressor.decompress(); // unused metadata
	dataOffset = decompressor.seekPos();

	m_metaDataValid = false;

	if (m_withMetaData && version >= 27)
	{
		m_metaDataValid = true;
		unsigned char const *md = metaData.c_str();
		unsigned char const *metaDataEnd = md + metaData.size();
		if (CHECKMD(1))
		{
			int metaDataVersion = md[0];
			md++;

			if (metaDataVersion) {
				if (CHECKMD(2))
				{
					int metaDataCount = readU16(md);
					md+=2;
					for (int i = 0; i < metaDataCount ; i++)
					{
						if (!CHECKMD(2)) {
							break;
						}
						int position = readU16(md);
						if (position >= 0xFFF)
						{
							std::cout << "invalid position in NodeMetaData. Corrupted metadata?, skipping the rest of the metadata for this block." << std::endl;
							m_metaDataValid = false;
							break;
						}
						md+=2;

						if (!CHECKMD(4)) {
							break;
						}
						unsigned int numVars = readU32(md);
						md+=4;

						NodeMetaData nmd;
						for (unsigned var = 0; var < numVars; var++)
						{
							if (!CHECKMD(2)) {
								break;
							}
							size_t keyLen = readU16(md);
							md+=2;
							if (!CHECKMD(keyLen)) {
								break;
							}

							std::string key((char const *)md, keyLen);
							md+= keyLen;
							if (!CHECKMD(4)) {
								break;
							}
							size_t valueLen = readU32(md);
							md+=4;
							if (!CHECKMD(valueLen)) {
								break;
							}
							std::string value((char const *)md, valueLen);
							nmd.push_back(std::pair<std::string, std::string>(key, value));
							md+= valueLen;
							if (metaDataVersion > 1) {
								if (!CHECKMD(1)) {
									break;
								}
								md++; // skip priv flag
							}
						}

						// Sometimes there's an "EndInventory\n" at the end of the metadata strings without preceding "List".
						// I can't really se why that happens from minetest's code, but it happens, so skip it.
						while (md+13 <= metaDataEnd && !strncmp("EndInventory\n",reinterpret_cast<const char *>(md), 13))
						{
							md+=13;
						}

						if (md + 4 <= metaDataEnd && !strncmp("List", reinterpret_cast<const char *>(md), 4))   // skip inventory for now
						{
							while (true)
							{
								unsigned char const *start = md;
								while (*md != '\n')
								{
									if (!CHECKMD(1)) {
										break;
									}
									md++;
								}
								CHECKMD(1);
								md++;

								if (!m_metaDataValid)
								{
									break;
								}

								if (!strncmp("EndInventory\n", reinterpret_cast<char const *>(start), 13))
								{
									break;
								}
							}
						}
						m_metaData[position] = std::move(nmd);
						if (!m_metaDataValid) {
							break;
						}
					}
				}
			}
		}
	}

	// Skip unused data
	if (version <= 21)
		dataOffset += 2;
	if (version == 23)
		dataOffset += 1;
	if (version == 24) {
		uint8_t ver = data[dataOffset++];
		if (ver == 1) {
			uint16_t num = readU16(data + dataOffset);
			dataOffset += 2;
			dataOffset += 10 * num;
		}
	}

	// Skip unused static objects
	dataOffset++; // Skip static object version
	int staticObjectCount = readU16(data + dataOffset);
	dataOffset += 2;
	for (int i = 0; i < staticObjectCount; ++i) {
		dataOffset += 13;
		uint16_t dataSize = readU16(data + dataOffset);
		dataOffset += dataSize + 2;
	}
	dataOffset += 4; // Skip timestamp

	// Read mapping
	if (version >= 22) {
		dataOffset++; // mapping version
		uint16_t numMappings = readU16(data + dataOffset);
		dataOffset += 2;
		for (int i = 0; i < numMappings; ++i) {
			uint16_t nodeId = readU16(data + dataOffset);
			dataOffset += 2;
			uint16_t nameLen = readU16(data + dataOffset);
			dataOffset += 2;
			std::string name(reinterpret_cast<const char *>(data) + dataOffset, nameLen);
			if (name == "air")
				m_blockAirId = nodeId;
			else if (name == "ignore")
				m_blockIgnoreId = nodeId;
			else
				m_nameMap[nodeId] = name;
			dataOffset += nameLen;
		}
	}

	// Node timers
	if (version >= 25) {
		dataOffset++;
		uint16_t numTimers = readU16(data + dataOffset);
		dataOffset += 2;
		dataOffset += numTimers * 10;
	}
}

bool BlockDecoder::isEmpty() const
{
	// only contains ignore and air nodes?
	return m_nameMap.empty();
}

std::string BlockDecoder::getNode(u8 x, u8 y, u8 z) const
{
	unsigned int position = x + (y << 4) + (z << 8);
	int content = readBlockContent(m_mapData.c_str(), m_version, position);
	if (content == m_blockAirId || content == m_blockIgnoreId)
		return "";
	NameMap::const_iterator it = m_nameMap.find(content);
	if (it == m_nameMap.end()) {
		std::cerr << "Skipping node with invalid ID." << std::endl;
		return "";
	}
	return it->second;
}


BlockDecoder::NodeMetaData const &BlockDecoder::getNodeMetaData(u8 x, u8 y, u8 z) const
{
	unsigned int position = x + (y << 4) + (z << 8);

	MetaData::const_iterator m = m_metaData.find(position);

	if (m == m_metaData.end())
	{
		return s_emptyMetaData;
	}

	return m->second;

}

BlockDecoder::NodeMetaData BlockDecoder::s_emptyMetaData;

