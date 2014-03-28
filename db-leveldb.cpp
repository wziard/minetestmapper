#include "db-leveldb.h"
#include <stdexcept>
#include <sstream>

inline int64_t stoi64(const std::string &s) {
	std::stringstream tmp(s);
	long long t;
	tmp >> t;
	return t;
}

inline std::string i64tos(int64_t i) {
	std::ostringstream o;
	o<<i;
	return o.str();
}

DBLevelDB::DBLevelDB(const std::string &mapdir) {
	leveldb::Options options;
	posCacheLoaded = false;
	options.create_if_missing = false;
	leveldb::Status status = leveldb::DB::Open(options, mapdir + "map.db", &db);
	if(!status.ok())
		throw std::runtime_error("Failed to open Database");
}

DBLevelDB::~DBLevelDB() {
	delete db;
}

std::vector<int64_t> DBLevelDB::getBlockPos() {
	loadPosCache();
	return posCache;
}

void DBLevelDB::loadPosCache() {
	if (posCacheLoaded) {
		return;
	}

	leveldb::Iterator * it = db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		posCache.push_back(stoi64(it->key().ToString()));
	}
	delete it;
	posCacheLoaded = true;
}

DBBlockList DBLevelDB::getBlocksOnZ(int zPos) {
	DBBlockList blocks;
	std::string datastr;
	leveldb::Status status;

	int64_t psMin = (zPos * 16777216L) - 0x800000;
	int64_t psMax = (zPos * 16777216L) + 0x7fffff;

	for (std::vector<int64_t>::iterator it = posCache.begin(); it != posCache.end(); ++it) {
		int64_t i = *it;
		if (i < psMin || i > psMax) {
			continue;
		}
		status = db->Get(leveldb::ReadOptions(), i64tos(i), &datastr);
		if (status.ok()) {
			blocks.push_back(
				DBBlock(i,
					std::basic_string<unsigned char>((const unsigned char*) datastr.data(), datastr.size())
				)
			);
		}
	}

	return blocks;
}

