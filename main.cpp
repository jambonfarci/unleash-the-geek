#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#include <ios>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;

high_resolution_clock::time_point start;
#define NOW high_resolution_clock::now()
#define TIME duration_cast<duration<double>>(NOW - start).count()

// The watch macro is one of the most useful tricks ever.
#define watch(x) cerr << (#x) << " is " << (x) << "\n"

enum class Type : int {
    NONE = 0, ROBOT, RADAR, TRAP, ORE, HOLE
};

enum class ActionType : int {
    WAIT = 0, MOVE, DIG, REQUEST
};

static constexpr int PLAYERS = 2;
static constexpr int WIDTH = 30;
static constexpr int HEIGHT = 15;
static constexpr int ROBOTS = 5;

// Function to generate random numbers in given range
int random_num(int start, int end) {
    int range = (end - start) + 1;
    int random_int = start + (rand() % range);
    return random_int;
}

class Point;

class Cell;

class Entity;

class Robot;

class Game;

class Player;

class Individual;

Game *game;

class Point {
public:
    int x{-1};
    int y{-1};
    int sx{-1};
    int sy{-1};

    Point() = default;

    Point(int x, int y) {
        this->x = x;
        this->y = y;
    }

    virtual void move(int x, int y) {
        this->x = x;
        this->y = y;
    }

    int distance(Point *point) {
        return abs(x - point->x) + abs(y - point->y);
    }

    virtual void reset() {
        this->x = this->sx;
        this->y = this->sy;
    }
};

class Cell : public Point {
public:
    bool hole{false};
    bool oreVisible{false};
    int ore{0};

    Cell() = default;

    void update(Point *point, bool hole, bool oreVisible, int ore) {
        this->hole = hole;
        this->ore = ore;
        this->oreVisible = oreVisible;
        this->x = point->x;
        this->y = point->y;
    }
};

class Entity : public Point {
public:
    int id{0};
    Type type{Type::NONE};
    Type item{Type::NONE};
    int owner{0};

    Entity() = default;

    Entity(int id, Type type, Point *point, Type item, int owner) : Point() {
        this->x = point->x;
        this->y = point->y;
        this->id = id;
        this->type = type;
        this->owner = owner;
        this->item = item;
    }

    void update(int id, Type type, Point *point, Type item, int owner) {
        this->x = point->x;
        this->y = point->y;
        this->id = id;
        this->type = type;
        this->owner = owner;
        this->item = item;
    }
};

class Robot : public Entity {
public:
    Robot() = default;

    bool isDead() {
        return this->x == -1 && this->y == -1;
    }
};

class Player {
public:
    Robot *robots[ROBOTS];
    int ore{0};
    int cooldownRadar{0}, cooldownTrap{0};
    int owner{0};

    Player() = default;

    void updateRobot(int id, Point *point, Type item, int owner) {
        int idxOffset{0};

        if (id >= ROBOTS) {
            idxOffset = ROBOTS;
        }

        this->robots[id - idxOffset]->update(id, Type::ROBOT, point, item, owner);
        this->owner = owner;
    }

    void updateOre(int ore) {
        this->ore = ore;
    }

    void updateCooldownRadar(int cooldown) {
        this->cooldownRadar = cooldown;
    }

    void updateCooldownTrap(int cooldown) {
        this->cooldownTrap = cooldown;
    }
};

class Game {
public:
    Cell *grid[WIDTH][HEIGHT];
    Player *players[PLAYERS];
    vector<Entity *> radars;
    vector<Entity *> traps;

    Game() = default;

    Player *player() {
        return this->players[0];
    }

    Player *opponent() {
        return this->players[1];
    }

    void updateCell(int x, int y, const string &ore, int hole) {
        int oreAmount{0};
        bool oreVisible{false};
        Point *point = new Point{x, y};

        if (ore != "?") {
            oreAmount = stoi(ore);
            oreVisible = true;
        }

        this->grid[x][y]->update(point, (bool) hole, oreVisible, oreAmount);
    }

    void updateEntity(int id, int type, int x, int y, int _item) {
        // item
        Type item{Type::NONE};

        //-1 for NONE, 2 for RADAR, 3 for TRAP, 4 ORE
        switch (_item) {
            case -1:
                item = Type::NONE;
                break;
            case 2:
                item = Type::RADAR;
                break;
            case 3:
                item = Type::TRAP;
                break;
            case 4:
                item = Type::ORE;
                break;
            default:
                break;
        }

        Point *point = new Point{x, y};

        // 0 for your robot, 1 for other robot, 2 for radar, 3 for trap
        switch (type) {
            case 0:
                this->player()->updateRobot(id, point, item, type);
                break;
            case 1:
                this->opponent()->updateRobot(id, point, item, type);
                break;
            case 2: {
                Entity *radar = new Entity(id, Type::RADAR, point, item, 0);
                this->radars.emplace_back(radar);
                break;
            }
            case 3: {
                Entity *trap = new Entity(id, Type::TRAP, point, item, 0);
                this->traps.emplace_back(trap);
                break;
            }
            default:
                break;
        }
    }
};

/**
 * Deliver more ore to hq (left side of the map) than your opponent. Use radars to find ore but beware of traps!
 **/
int main() {
    // size of the map
    int width;
    int height;

    cin >> width >> height;
    cin.ignore();

    game = new Game();

    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            game->grid[x][y] = new Cell();
        }
    }

    game->players[0] = new Player();
    game->players[1] = new Player();

    for (int i = 0; i < ROBOTS; i++) {
        game->player()->robots[i] = new Robot();
        game->opponent()->robots[i] = new Robot();
    }

    // game loop
    while (1) {
        // Amount of ore delivered
        int playerOre;

        int opponentOre;

        cin >> playerOre >> opponentOre;
        cin.ignore();

        game->player()->updateOre(playerOre);
        game->opponent()->updateOre(opponentOre);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // amount of ore or "?" if unknown
                string ore;

                // 1 if cell has a hole
                int hole;

                cin >> ore >> hole;
                cin.ignore();

                game->updateCell(x, y, ore, hole);
            }
        }

        // number of entities visible to you
        int entityCount;

        // turns left until a new radar can be requested
        int radarCooldown;

        // turns left until a new trap can be requested
        int trapCooldown;

        cin >> entityCount >> radarCooldown >> trapCooldown;
        cin.ignore();

        for (int i = 0; i < entityCount; i++) {
            // unique id of the entity
            int id;

            // 0 for your robot, 1 for other robot, 2 for radar, 3 for trap
            int type;

            // position of the entity
            int x;
            int y;

            // if this entity is a robot, the item it is carrying (-1 for NONE, 2 for RADAR, 3 for TRAP, 4 for ORE)
            int item;

            cin >> id >> type >> x >> y >> item;
            cin.ignore();

            game->updateEntity(id, type, x, y, item);
        }

        for (int i = 0; i < ROBOTS; i++) {
            // Write an action using cout. DON'T FORGET THE "<< endl"
            // To debug: cerr << "Debug messages..." << endl;
            // WAIT|MOVE x y|DIG x y|REQUEST item
            cout << "DIG " << random_num(1, WIDTH - 1) << " " << random_num(0, HEIGHT - 1) << endl;
        }
    }
}

#pragma clang diagnostic pop