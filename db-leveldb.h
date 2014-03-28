#ifndef _DB_LEVELDB_H
#define _DB_LEVELDB_H

#include "db.h"
#include <leveldb/db.h>

class DBLevelDB : public DB {
public:
	DBLevelDB(const std::string &mapdir);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlockList getBlocksOnZ(int zPos);
	~DBLevelDB();
private:
	void loadPosCache();

	leveldb::DB *db;

	bool posCacheLoaded;
	std::vector<int64_t> posCache;
};

#endif // _DB_LEVELDB_H
