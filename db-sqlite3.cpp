#include "db-sqlite3.h"
#include <stdexcept>
#include <unistd.h> // for usleep

DBSQLite3::DBSQLite3(const std::string &mapdir) {
	
	std::string db_name = mapdir + "map.sqlite";
	if (sqlite3_open_v2(db_name.c_str(), &m_db, SQLITE_OPEN_READONLY | SQLITE_OPEN_PRIVATECACHE, 0) != SQLITE_OK) {
		throw std::runtime_error(std::string(sqlite3_errmsg(m_db)) + ", Database file: " + db_name);
	}
}

DBSQLite3::~DBSQLite3() {
	sqlite3_close_v2(m_db);
}

std::vector<int64_t> DBSQLite3::getBlockPos() {
	std::vector<int64_t> vec;
	sqlite3_stmt *statement;
	std::string sql = "SELECT pos FROM blocks";
	if (sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &statement, 0) == SQLITE_OK) {
		int result = 0;
		while (true) {
			result = sqlite3_step(statement);
			if(result == SQLITE_ROW) {
				sqlite3_int64 blocknum = sqlite3_column_int64(statement, 0);
				vec.push_back(blocknum);
			} else if (result == SQLITE_BUSY) // Wait some time and try again
				usleep(10000);
			else
				break;
		}
	} else {
		throw std::runtime_error("Failed to get list of MapBlocks");
	}
	return vec;
}

DBBlockList DBSQLite3::getBlocksOnZ(int zPos)
{
	sqlite3_stmt *statement;
	std::string sql = "SELECT pos, data FROM blocks WHERE (pos >= ? AND pos <= ?)";
	if (sqlite3_prepare_v2(m_db, sql.c_str(), sql.length(), &statement, 0) != SQLITE_OK) {
		throw std::runtime_error("Failed to prepare statement");
	}
	DBBlockList blocks;

	sqlite3_int64 psMin;
	sqlite3_int64 psMax;

	psMin = (static_cast<sqlite3_int64>(zPos) * 16777216l) - 0x800000;
	psMax = (static_cast<sqlite3_int64>(zPos) * 16777216l) + 0x7fffff;
	sqlite3_bind_int64(statement, 1, psMin);
	sqlite3_bind_int64(statement, 2, psMax);

	int result = 0;
	while (true) {
		result = sqlite3_step(statement);
		if(result == SQLITE_ROW) {
			sqlite3_int64 blocknum = sqlite3_column_int64(statement, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(statement, 1));
			int size = sqlite3_column_bytes(statement, 1);
			blocks.push_back(DBBlock(blocknum, std::basic_string<unsigned char>(data, size)));
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			break;
		}
	}
	sqlite3_reset(statement);

	return blocks;
}

