#include <chthon2/pathfinding.h>
#include <chthon2/map.h>
#include <chthon2/log.h>
#include <chthon2/format.h>
#include <chthon2/util.h>
#include <ncurses.h>
#include <fstream>
#include <cstdlib>

enum {
	BATTLEFIELD_SIZE = 5,
	BATTLE_FOREST_COUNT = 5,
	MAP_SIZE = 25,
	PUZZLE_SIZE = 5,
	PUZZLE_RADIUS = PUZZLE_SIZE / 2,
	VIEW_SIZE = 5,
	VIEW_RADIUS = VIEW_SIZE / 2,
	LEFT_STATUS_BAR = VIEW_SIZE * 9,
	FIGHTLOG_SIZE = 10,

	PLAYER_BASE_HP = 10,
	ENEMY_BASE_HP = 10,
	PLAYER_DAMAGE_RANGE = 3,
	ENEMY_DAMAGE_RANGE = 3,
	DAYS_LEFT = 300,
	MAX_ENEMY_COUNT = 3,
	BASE_MONEY_FOR_BATTLE = 100,
	MAX_MONEY_FOR_ONE_ENEMY = 200,
	TREASURE_MONEY = 100,

	COUNT
};

const Chthon::Point BATTLE_MAP(0, 0);
const Chthon::Point VIEW_MAP(0, 0);
const Chthon::Point PUZZLE_CENTER(LEFT_STATUS_BAR + 6, 4);

struct Character {
	Chthon::Point pos;
	int hp;
	Character(const Chthon::Point & char_pos, int char_hp)
		: pos(char_pos), hp(char_hp)
	{}
};

struct Evil {
	Chthon::Point pos;
	int count;
	Evil(const Chthon::Point & evil_pos = Chthon::Point(), int evil_count = 1)
		: pos(evil_pos), count(evil_count)
	{}
};

struct Cell {
	char sprite;
	bool seen;
	Cell(char cell_sprite = ' ', bool cell_seen = false)
		: sprite(cell_sprite), seen(cell_seen)
	{}
};

typedef int MiniSprite;
typedef Chthon::Map<int> Sprite;

template<class T, class Generator, class Check>
T generate_value(int tries, Generator generator, Check check)
{
	T value = generator();
	while(!check(value) && tries --> 0) {
		value = generator();
	}
	return value;
}

Chthon::Point get_random_free_pos(const Chthon::Map<Cell> & map)
{
	return generate_value<Chthon::Point>(MAP_SIZE * MAP_SIZE,
			[](){ return Chthon::Point(rand() % MAP_SIZE, rand() % MAP_SIZE); },
			[map](const Chthon::Point & p){ return map.cell(p).sprite == '.'; }
			);
}

void draw_sprite(const Chthon::Point & start, const Chthon::Point & pos, const Sprite & sprite)
{
	for(int x = 0; x < sprite.width(); ++x) {
		for(int y = 0; y < sprite.height(); ++y) {
			mvaddch(
					start.y + pos.y * sprite.height() + y,
					start.x + pos.x * sprite.width() + x,
					sprite.cell(x, y));
		}
	}
}

int fibonacci(int n)
{
	if(n <= 1) {
		return 1;
	}
	return fibonacci(n - 1) + fibonacci(n - 2);
}

Chthon::Point get_shift(int control)
{
	switch(control) {
		case 'h' : return Chthon::Point(-1,  0);
		case 'j' : return Chthon::Point( 0,  1);
		case 'k' : return Chthon::Point( 0, -1);
		case 'l' : return Chthon::Point( 1,  0);
		case 'y' : return Chthon::Point(-1, -1);
		case 'u' : return Chthon::Point( 1, -1);
		case 'b' : return Chthon::Point(-1,  1);
		case 'n' : return Chthon::Point( 1,  1);
		default: return Chthon::Point();
	}
}

class Game {
public:
	Game();
	virtual ~Game();
	void create();
	int run();
private:
	Chthon::Map<Cell> map;
	Chthon::Point player, artifact;
	Chthon::Map<char> puzzle;
	std::list<Evil> evil;
	int days_left;
	int money;
	bool quit;
	int strength, endurance;
	std::map<char, MiniSprite> minisprites;
	std::map<char, Sprite> sprites;
	Sprite statusbar;

	bool fight(int enemy_count);
	void map_mode();
	void character_mode();
};

bool Game::fight(int enemy_count)
{
	bool done = false;
	Chthon::Map<char> battlefield(BATTLEFIELD_SIZE, BATTLEFIELD_SIZE, '.');
	int forest_count = rand() % BATTLE_FOREST_COUNT;
	for(int i = 0; i < forest_count; ++i) {
		battlefield.cell(1 + rand() % (BATTLEFIELD_SIZE - 2), rand() % BATTLEFIELD_SIZE) = '#';
	}
	Character player({0, BATTLEFIELD_SIZE / 2}, PLAYER_BASE_HP + endurance);
	std::vector<Character> enemies;
	for(int i = 0; i < enemy_count; ++i) {
		enemies << Character({BATTLEFIELD_SIZE - 1, (BATTLEFIELD_SIZE / 2 + 1 - enemy_count) + i * 2}, ENEMY_BASE_HP);
	}
	std::vector<std::string> fightlog;
	while(!done) {
		erase();
		for(int x = 0; x < battlefield.width(); ++x) {
			for(int y = 0; y < battlefield.height(); ++y) {
				draw_sprite(BATTLE_MAP, Chthon::Point(x, y), sprites[battlefield.cell(x, y)]);
			}
		}
		draw_sprite(BATTLE_MAP, player.pos, sprites['@']);
		for(const Character & enemy : enemies) {
			draw_sprite(BATTLE_MAP, enemy.pos, sprites['A']);
		}
		mvprintw(0, LEFT_STATUS_BAR, "HP: %d", player.hp);
		int start_line = std::max(int(fightlog.size()) - FIGHTLOG_SIZE, 0);
		for(int i = start_line; i < fightlog.size(); ++i) {
			mvprintw(1 + i - start_line, LEFT_STATUS_BAR, "%s", fightlog[i].c_str());
		}

		Chthon::Point shift;
		while(true) {
			char control = getch();
			shift = get_shift(control);
			switch(control) {
				case 'q' : return false;
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
				int damage = strength + rand() % PLAYER_DAMAGE_RANGE;
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
					int damage = rand() % ENEMY_DAMAGE_RANGE;
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

void Game::map_mode()
{
	erase();
	for(int x = 0; x < map.width(); ++x) {
		for(int y = 0; y < map.height(); ++y) {
			if(map.cell(x, y).seen) {
				mvaddch(y, x, minisprites[map.cell(x, y).sprite]);
			}
		}
	}
	mvaddch(player.y, player.x, minisprites['@']);
	while(getch() != 'm');
}

void Game::character_mode()
{
	erase();
	while(true) {
		int strength_cost = fibonacci(strength + 1) * 100;
		int endurance_cost = fibonacci(endurance + 1) * 100;
		mvprintw(0, 0, "Money: %d      ", money);
		mvprintw(1, 0, "Strength: %d (%d to increase)", strength, strength_cost);
		mvprintw(2, 0, "Endurance: %d (%d to increase)", endurance, endurance_cost);
		mvprintw(3, 0, "Increase strength (a), increase endurance (b) or exit (space)");
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
			case ' ': case 'c': return;
		}
	}
}

Game::Game()
	: map(MAP_SIZE, MAP_SIZE, '.'), puzzle(PUZZLE_SIZE, PUZZLE_SIZE, 0),
	days_left(DAYS_LEFT), money(0), quit(false), strength(0), endurance(0)
{
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	curs_set(0);


	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_YELLOW, COLOR_BLACK);
	minisprites[' '] = ' ';
	minisprites['.'] = '"' | COLOR_PAIR(1);
	minisprites['#'] = '#' | COLOR_PAIR(1) | A_BOLD;
	minisprites['@'] = '@' | COLOR_PAIR(2) | A_BOLD;
	minisprites['A'] = 'A' | COLOR_PAIR(3) | A_BOLD;
	minisprites['*'] = '*' | COLOR_PAIR(4) | A_BOLD;
	minisprites['X'] = 'X' | COLOR_PAIR(2) | A_BOLD;

	Chthon::InterleavedCharMap sprites_data = { 9, 5, 6, {
	"  '      ", "@@@@@@@@@", "    @    ",  "   38    ",  " /=====\\ ", "  \\   /  ", 
	" '    '  ", "@@@@@@@@@", "   / \\ | ", "   ###   ",  "/       \\", "   \\ /   ", 
	"  ' '    ", "@@@@@@@@@", "  /|_|\\+ ", " 3/#3/   ",  "+-------+",  "    X    ", 
	" '  '  ' ", "@@@@@@@@@", "   | |   ",  "   S S\\  ", "|   9   |",  "   / \\   ", 
	"     ;   ", " # # # # ", "   L L   ",  "   S S ^ ",  "L_______J",  "  /   \\  ", 
	}};
	sprites[' '] = Sprite(sprites_data.width, sprites_data.height, ' ');
	sprites['.'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(0), sprites_data.end(0));
	for(int & cell : sprites['.']) cell |= COLOR_PAIR(1);
	sprites['#'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(1), sprites_data.end(1));
	for(int & cell : sprites['#']) cell |= COLOR_PAIR(1) | A_BOLD;
	sprites['@'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(2), sprites_data.end(2));
	for(int & cell : sprites['@']) cell |= COLOR_PAIR(2) | A_BOLD;
	sprites['A'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(3), sprites_data.end(3));
	for(int & cell : sprites['A']) cell |= COLOR_PAIR(3) | A_BOLD;
	sprites['*'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(4), sprites_data.end(4));
	for(int & cell : sprites['*']) cell |= COLOR_PAIR(4) | A_BOLD;
	sprites['X'] = Sprite(sprites_data.width, sprites_data.height, sprites_data.begin(5), sprites_data.end(5));
	for(int & cell : sprites['X']) cell |= COLOR_PAIR(2) | A_BOLD;

	const std::string statusbar_data = 
		"+===========+===============================+"
		"|Puzzle map |                               |"
		"|           |                               |"
		"|           |                               |"
		"|           |                               |"
		"|           |                               |"
		"|           |                               |"
		"+===========+===============================+"
		"| (hjklyubn) Move                           |"
		"| (m) Show map                              |"
		"| (c) Character screen                      |"
		"| (d) Dig for artifact                      |"
		"|                                           |"
		"| Walk onto enemy to fight with them.       |"
		"| Walk onto treasure to collect it.         |"
		"| Be careful not to fight with forces       |"
		"| greater than you can handle.              |"
		"| But do not waste any time! It's running   |"
		"| out.                                      |"
		"| Check map often. Once you're sure about   |"
		"| artifact location, go there and dig.      |"
		"| Don't forget to spend money on your       |"
		"| character stats! It could reaaly help.    |"
		"|                                           |"
		"+===========================================+"
		;
	statusbar = Sprite(45, 25, statusbar_data.begin(), statusbar_data.end());


	for(int i = 0; i < MAP_SIZE * MAP_SIZE * 2 / 5; ++i) {
		map.cell(rand() % MAP_SIZE, rand() % MAP_SIZE) = '#';
	}
	player = get_random_free_pos(map);
	for(int i = 0; i < MAP_SIZE; ++i) {
		map.cell(get_random_free_pos(map)) = '*';
	}
	for(int i = 0; i < PUZZLE_SIZE * PUZZLE_SIZE; ++i) {
		evil.push_back(Evil(get_random_free_pos(map), 1 + rand() % MAX_ENEMY_COUNT));
	}

	artifact = generate_value<Chthon::Point>(MAP_SIZE * MAP_SIZE,
			[](){ return Chthon::Point(
				PUZZLE_SIZE / 2 + rand() % (MAP_SIZE - PUZZLE_SIZE / 2),
				PUZZLE_SIZE / 2 + rand() % (MAP_SIZE - PUZZLE_SIZE / 2)
			); },
			[this](const Chthon::Point & p){ return this->map.cell(p).sprite != '#'; }
			);
}

int Game::run()
{
	while(!quit) {
		erase();
		for(int x = 0; x < statusbar.width(); ++x) {
			for(int y = 0; y < statusbar.height(); ++y) {
				mvaddch(y, LEFT_STATUS_BAR + x, statusbar.cell(x, y));
			}
		}
		mvprintw(1, LEFT_STATUS_BAR + 14, "Money: %d", money);
		mvprintw(2, LEFT_STATUS_BAR + 14, "Days left: %d", days_left);
		for(int x = -VIEW_RADIUS; x <= VIEW_RADIUS; ++x) {
			for(int y = -VIEW_RADIUS; y <= VIEW_RADIUS; ++y) {
				Chthon::Point pos = player + Chthon::Point(x, y);
				int sprite = ' ';
				if(map.valid(pos)) {
					sprite = map.cell(pos).sprite;
					map.cell(pos).seen = true;
				}
				draw_sprite(VIEW_MAP, Chthon::Point(x + 2, y + 2), sprites[sprite]);
				for(const Evil & e : evil) {
					if(e.pos == pos) {
						draw_sprite(VIEW_MAP, Chthon::Point(x + 2, y + 2), sprites['A']);
					}
				}
			}
		}
		draw_sprite(VIEW_MAP, Chthon::Point(2, 2), sprites['@']);

		for(int x = -PUZZLE_RADIUS; x <= PUZZLE_RADIUS; ++x) {
			for(int y = -PUZZLE_RADIUS; y <= PUZZLE_RADIUS; ++y) {
				Chthon::Point pos = artifact + Chthon::Point(x, y);
				int sprite = ' ';
				if(map.valid(pos) && puzzle.cell(x + PUZZLE_RADIUS, y + PUZZLE_RADIUS)) {
					sprite = map.cell(pos).sprite;
				}
				mvaddch(PUZZLE_CENTER.y + y, PUZZLE_CENTER.x + x, minisprites[sprite]);
			}
		}
		mvaddch(PUZZLE_CENTER.y, PUZZLE_CENTER.x, minisprites['X']);

		char control = getch();
		Chthon::Point shift = get_shift(control);;
		switch(control) {
			case 'q' : quit = true; break;
			case 'm' : map_mode(); break;
			case 'c' : character_mode(); break;
			case 'd':
			{
				if(player == artifact) {
					quit = true;
				}
				break;
			}
		}
		if(!shift.null() && map.valid(player + shift) && map.cell(player + shift).sprite != '#') {
			Chthon::Point new_player_pos = player + shift;
			std::list<Evil>::iterator e;
			e = std::find_if(evil.begin(), evil.end(),
					[new_player_pos](const Evil & e) { return e.pos == new_player_pos; }
					);
			if(e != evil.end()) {
				mvprintw(11, 25, "+----------------------------+");
				mvprintw(12, 25, "|    There are %d enemies     |", e->count);
				mvprintw(13, 25, "| Do you want to fight them? |");
				mvprintw(14, 25, "|           (y/n)            |");
				mvprintw(15, 25, "+----------------------------+");
				int answer = 0;
				while(answer != 'y' && answer != 'n') {
					answer = getch();
				}
				if(answer == 'y') {
					if(fight(e->count)) {
						evil.erase(e);
						player += shift;
						money += BASE_MONEY_FOR_BATTLE + rand() % MAX_MONEY_FOR_ONE_ENEMY * e->count;

						Chthon::Point piece = generate_value<Chthon::Point>(PUZZLE_SIZE * PUZZLE_SIZE,
								[](){ return Chthon::Point(rand() % PUZZLE_SIZE, rand() % PUZZLE_SIZE); },
								[this](const Chthon::Point & p){ return !this->puzzle.cell(p); }
								);
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
			money += TREASURE_MONEY;
			map.cell(player) = '.';
		}
	}
	return 0;
}

Game::~Game()
{
	cbreak();
	echo();
	curs_set(1);
	endwin();
}

int main()
{
	std::ofstream log_file("wted.log");
	Chthon::direct_log(&log_file);
	srand(time(NULL));

	Game game;
	return game.run();
}
