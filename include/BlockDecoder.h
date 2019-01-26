#ifndef BLOCKDECODER_H
#define BLOCKDECODER_H

#if __cplusplus >= 201103L
#include <unordered_map>
#else
#include <map>
#endif

#include <vector>

#include "types.h"


class BlockDecoder {
public:
	typedef std::vector<std::pair<std::string, std::string> > NodeMetaData;
#if __cplusplus >= 201103L
	typedef std::unordered_map<int, NodeMetaData> MetaData;
	typedef std::unordered_map<int, std::string> NameMap;
#else
	typedef std::map<int, NodeMetaData> MetaData;
	typedef std::map<int, std::string> NameMap;
#endif

	BlockDecoder(bool withMetaData = false);

	void reset();
	void decode(const ustring &data);
	bool isEmpty() const;
	std::string getNode(u8 x, u8 y, u8 z) const; // returns "" for air, ignore and invalid nodes
	int getLightLevel(u8 x, u8 y, u8 z) const; // returns "" for air, ignore and invalid nodes

	NodeMetaData const &getNodeMetaData(u8 x, u8 y, u8 z) const;

private:

	static NodeMetaData s_emptyMetaData;
	NameMap m_nameMap;
	MetaData m_metaData;
	int m_blockAirId;
	int m_blockIgnoreId;

	bool m_withMetaData;
	u8 m_version;
	ustring m_mapData;
};

#endif // BLOCKDECODER_H
