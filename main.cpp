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

bool fight(int strength, int endurance, int enemy_count)
{
	bool done = false;
	Chthon::Map<char> battlefield(5, 5, '.');
	int forest_count = rand() % 5;
	for(int i = 0; i < forest_count; ++i) {
		battlefield.cell(1 + rand() % 3, rand() % 5) = '#';
	}
	Character player({0, 2}, 10 + endurance);
	std::vector<Character> enemies;
	for(int i = 0; i < enemy_count; ++i) {
		enemies << Character({4, (3 - enemy_count) + i * 2}, 10);
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
				int damage = strength + rand() % 3;
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

struct Cell {
	char sprite;
	bool seen;
	Cell(char cell_sprite = ' ', bool cell_seen = false)
		: sprite(cell_sprite), seen(cell_seen)
	{}
};

int fibonacci(int n)
{
	if(n <= 1) {
		return 1;
	}
	return fibonacci(n - 1) + fibonacci(n - 2);
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

	Chthon::Map<Cell> map(25, 25, '.');
	for(int i = 0; i < 250; ++i) {
		map.cell(rand() % 25, rand() % 25) = '#';
	}
	for(int i = 0; i < 25; ++i) {
		map.cell(rand() % 25, rand() % 25) = '*';
	}
	for(int i = 0; i < 25; ++i) {
		map.cell(rand() % 25, rand() % 25) = 'A';
	}

	Chthon::Point player(rand() % 25, rand() % 25);
	int tries = 625;
	while(map.cell(player).sprite == '#' && tries --> 0) {
		player = Chthon::Point(rand() % 25, rand() % 25);
	}

	Chthon::Point artifact = Chthon::Point(2 + rand() % 21, 2 + rand() % 21);
	tries = 625;
	while(map.cell(artifact).sprite == '#' && tries --> 0) {
		artifact = Chthon::Point(2 + rand() % 21, 2 + rand() % 21);
	}
	Chthon::Map<char> puzzle(5, 5, 0);

	int days_left = 300;
	int money = 0;
	bool quit = false;
	int strength = 0, endurance = 0;
	while(!quit) {
		erase();
		mvprintw(0, 0, "Money: %d     Days left: %d", money, days_left);
		for(int x = -2; x <= 2; ++x) {
			for(int y = -2; y <= 2; ++y) {
				Chthon::Point pos = player + Chthon::Point(x, y);
				if(map.valid(pos)) {
					mvaddch(13 + y, 35 + x, map.cell(pos).sprite);
					map.cell(pos).seen = true;
				} else {
					mvaddch(13 + y, 35 + x, ' ');
				}
			}
		}
		mvaddch(13, 35, '@');

		for(int x = -2; x <= 2; ++x) {
			for(int y = -2; y <= 2; ++y) {
				Chthon::Point pos = artifact + Chthon::Point(x, y);
				if(map.valid(pos) && puzzle.cell(x + 2, y + 2)) {
					mvaddch(13 + y, 65 + x, map.cell(pos).sprite);
				} else {
					mvaddch(13 + y, 65 + x, ' ');
				}
			}
		}
		mvaddch(13, 65, 'X');

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
						if(map.cell(x, y).seen) {
							mvaddch(y, x, map.cell(x, y).sprite);
						}
					}
				}
				mvaddch(player.y, player.x, '@');
				while(getch() != 'm');
				break;
			}
			case 'c':
			{
				erase();
				bool done = false;
				while(!done) {
					int strength_cost = fibonacci(strength + 1) * 100;
					int endurance_cost = fibonacci(endurance + 1) * 100;
					mvprintw(0, 0, "Money: %d      ", money);
					mvprintw(1, 0, "Strength: %d (%d to increase)", strength, strength_cost);
					mvprintw(2, 0, "Endurance: %d (%d to increase)", endurance, endurance_cost);
					mvprintw(3, 0, "Increase strength (a), increase endurance (2) or exit (space)");
					int ch = getch();
					switch(ch) {
						case 'a':
							if(money >= strength_cost) {
								++strength;
								money -= strength_cost;
							} else {
								mvprintw(4, 0, "Not enough money to increase strength! ");
							}
							break;
						case 'b':
							if(money >= endurance_cost) {
								++endurance;
								money -= endurance_cost;
							} else {
								mvprintw(4, 0, "Not enough money to increase endurance!");
							}
							break;
						case ' ': done = true; break;
					}
				}
				break;
			}
			case 'd':
			{
				if(player == artifact) {
					quit = true;
				}
				break;
			}
		}
		if(map.valid(player + shift) && map.cell(player + shift).sprite != '#') {
			if(map.cell(player + shift).sprite == 'A') {
				int enemy_count = 1 + rand() % 3;
				mvprintw(17, 0, "There are %d enemies. Do you want to fight them? (y/n)", enemy_count);
				int answer = 0;
				while(answer != 'y' && answer != 'n') {
					answer = getch();
				}
				if(answer == 'y') {
					if(fight(strength, endurance, enemy_count)) {
						map.cell(player + shift) = '.';
						player += shift;
						money += 100 + rand() % 200 * enemy_count;

						Chthon::Point piece = Chthon::Point(rand() % 5, rand() % 5);
						tries = 625;
						while(puzzle.cell(piece) && tries --> 0) {
							piece = Chthon::Point(rand() % 5, rand() % 5);
						}
						puzzle.cell(piece) = 1;
					} else {
						quit = true;
					}
				}
			} else {
				player += shift;
			}
			--days_left;
			if(days_left <= 0) {
				quit = true;
			}
		}
		if(map.cell(player).sprite == '*') {
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
