#ifndef _DB_LEVELDB_H
#define _DB_LEVELDB_H

#include "db.h"
#include <leveldb/db.h>
#include <set>

class DBLevelDB : public DB {
public:
	DBLevelDB(const std::string &mapdir);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlockList getBlocksOnZ(int zPos);
	~DBLevelDB();
private:
	leveldb::DB *m_db;
	std::set<int64_t> m_bpcache;
};

#endif // _DB_LEVELDB_H
