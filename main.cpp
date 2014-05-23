#include <chthon2/pathfinding.h>
#include <chthon2/map.h>
#include <chthon2/log.h>
#include <chthon2/format.h>
#include <chthon2/util.h>
#include <ncurses.h>
#include <fstream>
#include <cstdlib>

struct Character {
	Chthon::Point pos;
	int hp;
	Character(const Chthon::Point & char_pos, int char_hp)
		: pos(char_pos), hp(char_hp)
	{}
};

bool fight()
{
	bool done = false;
	Chthon::Map<char> battlefield(5, 5, '.');
	int forest_count = rand() % 5;
	for(int i = 0; i < forest_count; ++i) {
		battlefield.cell(1 + rand() % 3, rand() % 5) = '#';
	}
	Character player({0, 2}, 20);
	std::vector<Character> enemies;
	int enemy_count = 1 + rand() % 3;
	for(int i = 0; i < enemy_count; ++i) {
		enemies << Character({4, (3 - enemy_count) + i * 2}, 5);
	}
	std::vector<std::string> fightlog;
	while(!done) {
		erase();
		for(int x = 0; x < battlefield.width(); ++x) {
			for(int y = 0; y < battlefield.height(); ++y) {
				mvaddch(11 + y, 33 + x, battlefield.cell(x, y));
			}
		}
		mvaddch(11 + player.pos.y, 33 + player.pos.x, '@');
		for(const Character & enemy : enemies) {
			mvaddch(11 + enemy.pos.y, 33 + enemy.pos.x, 'A');
		}
		mvprintw(0, 0, "HP: %d", player.hp);
		int start_line = std::max(int(fightlog.size()) - 10, 0);
		for(int i = start_line; i < fightlog.size(); ++i) {
			mvprintw(1 + i - start_line, 0, "%s", fightlog[i].c_str());
		}

		Chthon::Point shift;
		while(true) {
			char control = getch();
			switch(control) {
				case 'q' : return false;
				case 'h' : shift = Chthon::Point(-1,  0); break;
				case 'j' : shift = Chthon::Point( 0,  1); break;
				case 'k' : shift = Chthon::Point( 0, -1); break;
				case 'l' : shift = Chthon::Point( 1,  0); break;
				case 'y' : shift = Chthon::Point(-1, -1); break;
				case 'u' : shift = Chthon::Point( 1, -1); break;
				case 'b' : shift = Chthon::Point(-1,  1); break;
				case 'n' : shift = Chthon::Point( 1,  1); break;
			}
			bool choice_made = !shift.null();
			bool valid_pos = battlefield.valid(player.pos + shift);
			bool not_forest = battlefield.cell(player.pos + shift) != '#';
			if(choice_made && valid_pos && not_forest) {
				break;
			}
		}

		bool fought = false;
		for(Character & enemy : enemies) {
			if(player.pos + shift == enemy.pos && enemy.hp > 0) {
				int damage = rand() % 3;
				enemy.hp -= damage;
				fightlog << Chthon::format("You hit enemy for {0} hp.", damage);
				if(enemy.hp <= 0) {
					fightlog << "Enemy is dead.";
				}
				fought = true;
				break;
			}
		}
		if(!fought) {
			player.pos += shift;
		}

		enemies.erase(std::remove_if(
					enemies.begin(), enemies.end(),
					[](const Character & enemy){ return enemy.hp <= 0; }
					), enemies.end());
		Chthon::Pathfinder finder;
		for(Character & enemy : enemies) {
			bool ok = finder.lee(enemy.pos, player.pos,
					[&](const Chthon::Point & p) {
					if(p == enemy.pos) {
						return true;
					}
					for(const Character & other : enemies) {
						if(other.pos == p) {
							return false;
						}
					}
					return battlefield.valid(p) && battlefield.cell(p) == '.';
					}
					);
			if(ok) {
				Chthon::Point new_pos = enemy.pos + finder.directions.front();
				if(new_pos == player.pos) {
					int damage = rand() % 2;
					player.hp -= damage;
					fightlog << Chthon::format("Enemy hit you for {0} hp.", damage);
					if(player.hp <= 0) {
						fightlog << "You are dead.";
					}
					if(player.hp <= 0) {
						return false;
					}
				} else {
					enemy.pos = new_pos;
				}
			}
		}
		if(enemies.empty()) {
			return true;
		}
	}
	return true;
}

int main()
{
	std::ofstream log_file("wted.log");
	Chthon::direct_log(&log_file);
	srand(time(NULL));
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);

	Chthon::Map<char> map(25, 25, '.');
	for(int i = 0; i < 250; ++i) {
		map.cell(rand() % 25, rand() % 25) = '#';
	}
	for(int i = 0; i < 25; ++i) {
		map.cell(rand() % 25, rand() % 25) = '*';
	}
	for(int i = 0; i < 10; ++i) {
		map.cell(rand() % 25, rand() % 25) = 'A';
	}

	Chthon::Point player(rand() % 25, rand() % 25);
	int tries = 625;
	while(map.cell(player) == '#' && tries --> 0) {
		player = Chthon::Point(rand() % 25, rand() % 25);
	}

	int money = 0;
	bool quit = false;
	while(!quit) {
		erase();
		mvprintw(0, 0, "Money: %d", money);
		for(int x = -2; x <= 2; ++x) {
			for(int y = -2; y <= 2; ++y) {
				Chthon::Point pos = player + Chthon::Point(x, y);
				if(map.valid(pos)) {
					mvaddch(13 + y, 35 + x, map.cell(pos));
				} else {
					mvaddch(13 + y, 35 + x, ' ');
				}
			}
		}
		mvaddch(13, 35, '@');

		char control = getch();
		Chthon::Point shift;
		switch(control) {
			case 'q' : quit = true; break;
			case 'h' : shift = Chthon::Point(-1,  0); break;
			case 'j' : shift = Chthon::Point( 0,  1); break;
			case 'k' : shift = Chthon::Point( 0, -1); break;
			case 'l' : shift = Chthon::Point( 1,  0); break;
			case 'y' : shift = Chthon::Point(-1, -1); break;
			case 'u' : shift = Chthon::Point( 1, -1); break;
			case 'b' : shift = Chthon::Point(-1,  1); break;
			case 'n' : shift = Chthon::Point( 1,  1); break;
			case 'm' :
			{
				erase();
				for(int x = 0; x < map.width(); ++x) {
					for(int y = 0; y < map.height(); ++y) {
						mvaddch(y, x, map.cell(x, y));
					}
				}
				mvaddch(player.y, player.x, '@');
				while(getch() != 'm');
				break;
			}
		}
		if(map.valid(player + shift) && map.cell(player + shift) != '#') {
			if(map.cell(player + shift) == 'A') {
				if(fight()) {
					map.cell(player + shift) = '.';
					player += shift;
					money += 100 + rand() % 500;
				} else {
					quit = true;
				}
			} else {
				player += shift;
			}
		}
		if(map.cell(player) == '*') {
			money += 100;
			map.cell(player) = '.';
		}
	}

	cbreak();
	echo();
	curs_set(1);
	endwin();
	return 0;
}
