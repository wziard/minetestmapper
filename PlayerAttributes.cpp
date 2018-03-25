/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  01.09.2012 14:38:05
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <dirent.h>
#include <fstream>
#include <sstream>

#include "config.h"
#include "PlayerAttributes.h"
#include "util.h"

using namespace std;

PlayerAttributes::PlayerAttributes(const std::string &sourceDirectory)
{

	string playersPath = sourceDirectory + "players";
	DIR *dir;
	dir = opendir (playersPath.c_str());
	if (dir == NULL)
		return;

	struct dirent *ent;
	while ((ent = readdir (dir)) != NULL) {
		if (ent->d_name[0] == '.')
			continue;

		string path = playersPath + PATH_SEPARATOR + ent->d_name;
		ifstream in(path.c_str());
		if(!in.good())
			continue;

		string name, position;
		name = read_setting("name", in);
		in.seekg(0);
		position = read_setting("position", in);

		Player player;
		istringstream iss(position);
		char tmp;
		iss >> tmp; // '('
		iss >> player.x;
		iss >> tmp; // ','
		iss >> player.y;
		iss >> tmp; // ','
		iss >> player.z;
		iss >> tmp; // ')'
		if(tmp != ')')
			continue;
		player.name = name;

		player.x /= 10.0;
		player.y /= 10.0;
		player.z /= 10.0;

		m_players.push_back(player);
	}
	closedir(dir);
}

PlayerAttributes::Players::iterator PlayerAttributes::begin()
{
	return m_players.begin();
}

PlayerAttributes::Players::iterator PlayerAttributes::end()
{
	return m_players.end();
}

