#ifndef BLOCKDECODER_H
#define BLOCKDECODER_H

#if __cplusplus >= 201103L
#include <unordered_map>
#else
#include <map>
#endif

#include "types.h"

class BlockDecoder {
public:
	BlockDecoder();

	void reset();
	void decode(const ustring &data);
	bool isEmpty() const;
	std::string getNode(u8 x, u8 y, u8 z) const; // returns "" for air, ignore and invalid nodes

private:
#if __cplusplus >= 201103L
	typedef std::unordered_map<int, std::string> NameMap;
#else
	typedef std::map<int, std::string> NameMap;
#endif
	NameMap m_nameMap;
	int m_blockAirId;
	int m_blockIgnoreId;

	u8 m_version;
	ustring m_mapData;
};

#endif // BLOCKDECODER_H
