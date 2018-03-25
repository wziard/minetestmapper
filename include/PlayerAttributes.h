#ifndef PLAYERATTRIBUTES_H_D7THWFVV
#define PLAYERATTRIBUTES_H_D7THWFVV

#include <list>
#include <string>

struct Player
{
	std::string name;
	double x, y, z;
};

class PlayerAttributes
{
public:
	typedef std::list<Player> Players;

	PlayerAttributes(const std::string &worldDir);
	Players::iterator begin();
	Players::iterator end();

private:
	void readFiles(const std::string &playersPath);
	void readSqlite(const std::string &db_name);

	Players m_players;
};

#endif /* end of include guard: PLAYERATTRIBUTES_H_D7THWFVV */

