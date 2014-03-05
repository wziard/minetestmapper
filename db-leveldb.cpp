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
	options.create_if_missing = false;
	leveldb::Status status = leveldb::DB::Open(options, mapdir + "map.db", &m_db);
	if(!status.ok())
		throw std::runtime_error("Failed to open Database");
}

DBLevelDB::~DBLevelDB() {
	delete m_db;
}

std::vector<int64_t> DBLevelDB::getBlockPos() {
	std::vector<int64_t> vec;
	std::set<int64_t> s;
	leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		vec.push_back(stoi64(it->key().ToString()));
		s.insert(stoi64(it->key().ToString()));
	}
	delete it;
	m_bpcache = s;
	return vec;
}

DBBlockList DBLevelDB::getBlocksOnZ(int zPos)
{
	DBBlockList blocks;
	std::string datastr;
	leveldb::Status status;

	int64_t psMin;
	int64_t psMax;
	psMin = (zPos * 16777216l) - 0x800000;
	psMax = (zPos * 16777216l) + 0x7fffff;

	for(int64_t i = psMin; i <= psMax; i++) { // FIXME: This is still very very inefficent (even with m_bpcache)
		if(m_bpcache.find(i) == m_bpcache.end())
			continue;
		status = m_db->Get(leveldb::ReadOptions(), i64tos(i), &datastr);
		if(status.ok())
			blocks.push_back( DBBlock( i, std::basic_string<unsigned char>( (const unsigned char*) datastr.c_str(), datastr.size() ) ) );
	}

	return blocks;
}

