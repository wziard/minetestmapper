#ifndef _DB_SQLITE3_H
#define _DB_SQLITE3_H

#include "db.h"
#include <sqlite3.h>

class DBSQLite3 : public DB {
public:
	DBSQLite3(const std::string &mapdir);
	virtual std::vector<int64_t> getBlockPos();
	virtual DBBlockList getBlocksOnZ(int zPos);
	~DBSQLite3();
private:
	sqlite3 *m_db;
};

#endif // _DB_SQLITE3_H
