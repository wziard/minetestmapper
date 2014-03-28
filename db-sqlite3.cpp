#include "db-sqlite3.h"
#include <stdexcept>
#include <unistd.h> // for usleep

#define SQLRES(f, good) \
	result = (sqlite3_##f);\
	if (result != good) {\
		throw std::runtime_error(sqlite3_errmsg(db));\
	}
#define SQLOK(f) SQLRES(f, SQLITE_OK)

DBSQLite3::DBSQLite3(const std::string &mapdir) {
	int result;
	std::string db_name = mapdir + "map.sqlite";

	SQLOK(open_v2(db_name.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_PRIVATECACHE, 0))

	SQLOK(prepare_v2(db,
			"SELECT pos, data FROM blocks WHERE (pos >= ? AND pos <= ?)",
		-1, &stmt_get_blocks, NULL))

	SQLOK(prepare_v2(db,
			"SELECT pos FROM blocks",
		-1, &stmt_get_block_pos, NULL))
}

DBSQLite3::~DBSQLite3() {
	int result;
	SQLOK(finalize(stmt_get_blocks));
	SQLOK(finalize(stmt_get_block_pos));

	SQLOK(close(db));
}

std::vector<int64_t> DBSQLite3::getBlockPos() {
	std::vector<int64_t> vec;
	int result;
	while ((result = sqlite3_step(stmt_get_block_pos)) != SQLITE_DONE) {
		if (result == SQLITE_ROW) {
			int64_t blockpos = sqlite3_column_int64(stmt_get_block_pos, 0);
			vec.push_back(blockpos);
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			throw std::runtime_error(sqlite3_errmsg(db));
		}
	}
	SQLOK(reset(stmt_get_block_pos));
	return vec;
}

DBBlockList DBSQLite3::getBlocksOnZ(int zPos)
{
	DBBlockList blocks;

	int64_t psMin = (static_cast<sqlite3_int64>(zPos) * 16777216L) - 0x800000;
	int64_t psMax = (static_cast<sqlite3_int64>(zPos) * 16777216L) + 0x7fffff;

	sqlite3_bind_int64(stmt_get_blocks, 1, psMin);
	sqlite3_bind_int64(stmt_get_blocks, 2, psMax);

	int result;
	while ((result = sqlite3_step(stmt_get_blocks)) != SQLITE_DONE) {
		if (result == SQLITE_ROW) {
			int64_t blocknum = sqlite3_column_int64(stmt_get_blocks, 0);
			const unsigned char *data = reinterpret_cast<const unsigned char *>(sqlite3_column_blob(stmt_get_blocks, 1));
			int size = sqlite3_column_bytes(stmt_get_blocks, 1);
			blocks.push_back(DBBlock(blocknum, std::basic_string<unsigned char>(data, size)));
		} else if (result == SQLITE_BUSY) { // Wait some time and try again
			usleep(10000);
		} else {
			throw std::runtime_error(sqlite3_errmsg(db));
		}
	}
	SQLOK(reset(stmt_get_blocks));

	return blocks;
}

#undef SQLRES
#undef SQLOK

