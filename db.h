#ifndef _DB_H
#define _DB_H

#include <stdint.h>
#include <vector>
#include <list>
#include <string>
#include <utility>

typedef std::pair<int64_t, std::basic_string<unsigned char> > DBBlock;
typedef std::list<DBBlock> DBBlockList;

class DB {
public:
	virtual std::vector<int64_t> getBlockPos() = 0;
	virtual DBBlockList getBlocksOnZ(int zPos) = 0;
};

#endif // _DB_H
