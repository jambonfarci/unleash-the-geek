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
#include <cmath>

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
static constexpr int MAX_TURNS = 200;
static constexpr int POPULATION_SIZE = 100;
static constexpr int DEPTH = 6;
static constexpr int MOVE_DISTANCE = 4;

int turn = 1;

class Point {
public:
    int x{-1};
    int y{-1};
    int sx{-1};
    int sy{-1};

    Point() = default;

    Point(int x, int y);

    int distance(Point *point);

    bool isAdjacent(Point *point);

    virtual void reset();
};

class Cell : public Point {
public:
    bool hole{false};
    bool oreVisible{false};
    int ore{0};

    Cell() = default;

    void update(bool hole, bool oreVisible, int ore);
};

class Entity : public Point {
public:
    int id{0};
    Type type{Type::NONE};
    Type item{Type::NONE};
    int owner{0};

    Entity() = default;

    Entity(int id, Type type, Point *point, Type item, int owner);

    void update(int id, Type type, Point *point, Type item, int owner);
};

class Action {
public:
    string type{"WAIT"};
    Point *destination{};
    string item{};

    Action() {
        this->destination = new Point();
    };
};

class Robot : public Entity {
public:
    Action *action{};
    Point *destination{};
    vector<Point *> destinations;

    Robot();

    bool isDead();

    void play();

    void move(int x, int y);

    void dig(int x, int y);

    void request(string item);

    void wait();

    void removeItem();

    void takeAction();

    void printAction();
};

class Player {
public:
    Robot *robots[ROBOTS]{};
    int ore{0};
    int cooldownRadar{0}, cooldownTrap{0};
    int owner{0};

    Player() = default;

    explicit Player(int owner);

    void updateRobot(int id, Point *point, Type item, int owner);

    void updateOre(int ore);

    void updateCooldownRadar(int cooldown);

    void updateCooldownTrap(int cooldown);
};

class Game {
public:
    Cell *grid[WIDTH][HEIGHT]{};
    Player *players[PLAYERS]{};
    vector<Entity *> radars;
    vector<Entity *> traps;

    Game();

    Player *player();

    Player *opponent();

    void updateCell(int x, int y, const string &ore, int hole);

    void updateEntity(int id, int type, int x, int y, int _item);
};

class Individual {
public:
    int playerId{};
    Action *actions[DEPTH][ROBOTS]{};
    float fitness = -INFINITY;

    Individual() = default;

    explicit Individual(int playerId);

    double evaluate();

    void randomize();

    void simulate(Individual *individual);
};

Game *game;

// Function to generate random numbers in given range
int random_num(int start, int end) {
    int range = (end - start) + 1;
    int random_int = start + (rand() % range);
    return random_int;
}

Point::Point(int x, int y) {
    this->x = x;
    this->y = y;
}

int Point::distance(Point *point) {
    return abs(x - point->x) + abs(y - point->y);
}

bool Point::isAdjacent(Point *point) {
    return this->distance(point) <= 1;
}

void Point::reset() {
    this->x = this->sx;
    this->y = this->sy;
}

void Cell::update(bool hole, bool oreVisible, int ore) {
    this->hole = hole;
    this->ore = ore;
    this->oreVisible = oreVisible;
}

Entity::Entity(int id, Type type, Point *point, Type item, int owner) : Point() {
    this->x = point->x;
    this->y = point->y;
    this->id = id;
    this->type = type;
    this->owner = owner;
    this->item = item;
}

void Entity::update(int id, Type type, Point *point, Type item, int owner) {
    this->x = point->x;
    this->y = point->y;
    this->id = id;
    this->type = type;
    this->owner = owner;
    this->item = item;
}

Robot::Robot() {
    this->action = new Action();
    this->destination = new Point();
}

bool Robot::isDead() {
    return this->x == -1 && this->y == -1;
}

void Robot::play() {
    if (this->isDead()) {
        this->wait();
        return;
    }

    if (this->action->type == "MOVE") {
        this->move(this->destination->x, this->destination->y);
    } else if (this->action->type == "DIG") {
        this->dig(this->destination->x, this->destination->y);
    } else if (this->action->type == "REQUEST") {
        this->request(this->action->item);
    } else {
        this->wait();
    }
}

void Robot::move(int x, int y) {
    if (x > WIDTH - 1) {
        x = WIDTH - 1;
    } else if (x < 0) {
        x = 0;
    }

    if (y > HEIGHT - 1) {
        y = HEIGHT - 1;
    } else if (y < 0) {
        y = 0;
    }

    int xDiff = x - this->x;
    int yDiff = y - this->y;

    for (int i = 0; i < MOVE_DISTANCE; i++) {
        if (xDiff == 0) {
            if (yDiff < 0) {
                this->y--;
                yDiff++;
            } else if (yDiff > 0) {
                this->y++;
                yDiff--;
            }
        } else if (yDiff == 0) {
            if (xDiff < 0) {
                this->x--;
                xDiff++;
            } else if (xDiff > 0) {
                this->x++;
                xDiff--;
            }
        } else {
            if (random_num(0, 1) == 0) {
                if (xDiff < 0) {
                    this->x--;
                    xDiff++;
                } else if (xDiff > 0) {
                    this->x++;
                    xDiff--;
                }
            } else {
                if (yDiff < 0) {
                    this->y--;
                    yDiff++;
                } else if (yDiff > 0) {
                    this->y++;
                    yDiff--;
                }
            }
        }
    }
}

void Robot::dig(int x, int y) {
    if (x > WIDTH - 1) {
        x = WIDTH - 1;
    } else if (x < 0) {
        x = 0;
    }

    if (y > HEIGHT - 1) {
        y = HEIGHT - 1;
    } else if (y < 0) {
        y = 0;
    }

    // Si la case n'est pas adjacente, le robot effectuera une commande MOVE pour se rapprocher de la destination
    // à la place.
    if (!this->isAdjacent(new Point(x, y))) {
        this->move(x, y);
        return;
    }

    // Si la case ne contient pas déjà un trou, un nouveau trou est creusé.
    if (!game->grid[x][y]->hole) {
        game->updateCell(x, y, to_string(game->grid[x][y]->ore), true);
        return;
    }

    // Si le robot contient un objet, l'objet est enterré dans le trou (et retiré de l'inventaire du robot).
    if (this->item != Type::NONE) {
        this->removeItem();
        return;
    }

    // Si la case contient un filon d'Amadeusium (et qu'un cristal n'a pas été enterré à l'étape 2),
    // un cristal est extrait du filon et ajouté à l'inventaire du robot.
    if (game->grid[x][y]->ore > 0) {
        game->grid[x][y]->update(game->grid[x][y]->hole, true, game->grid[x][y]->ore - 1);
        this->update(this->id, Type::ROBOT, new Point(this->x, this->y), Type::ORE, this->owner);
    }
}

void Robot::request(string item) {

}

void Robot::wait() {}

void Robot::removeItem() {
    this->item = Type ::NONE;
}

void Robot::takeAction() {
    // Le premier tour, le robot va au moins à x = 8 pour trouver du cristal.
    if (turn == 1) {
        this->action->type = "MOVE";
        this->destination->x = 8;
        this->destination->y = this->y;
        return;
    }

    // Après avoir livré du cristal au QG, le robot retourne à un endroit où il y a potentiellement du cristal.
    if (this->x == 0) {
        this->action->type = "MOVE";
        this->destination->x = 4 * random_num(1, 7);
        this->destination->y = this->y;
        return;
    }

    // Si le robot est mort, il attend.
    if (this->isDead()) {
        this->action->type = "WAIT";
        return;
    }

    // Si le robot porte un cristal, il retourne au QG.
    if (this->item == Type::ORE) {
        cerr << "Robot " << this->id << " a trouvé un cristal !" << endl;
        this->action->type = "MOVE";
        this->destination->x = 0;
        this->destination->y = this->y;
        return;
    }

    if (this->destination->x == this->x && this->destination->y == this->y && this->action->type != "DIG") {
        this->action->type = "DIG";
    } else {
        this->action->type = "MOVE";
//        this->updateDestination(this->destination->x + 1, this->destination->y);
    }
}

void Robot::printAction() {
    cout << this->action->type << " " << this->destination->x << " " << this->destination->y << endl;
}

Player::Player(int owner) {
    for (int i = 0; i < ROBOTS; i++) {
        this->robots[i] = new Robot();
    }

    this->owner = owner;
}

void Player::updateRobot(int id, Point *point, Type item, int owner) {
    int idxOffset{0};

    if (id >= ROBOTS) {
        idxOffset = ROBOTS;
    }

    this->robots[id - idxOffset]->update(id, Type::ROBOT, point, item, owner);
    this->owner = owner;
}

void Player::updateOre(int ore) {
    this->ore = ore;
}

void Player::updateCooldownRadar(int cooldown) {
    this->cooldownRadar = cooldown;
}

void Player::updateCooldownTrap(int cooldown) {
    this->cooldownTrap = cooldown;
}

Game::Game() {
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            this->grid[x][y] = new Cell();
        }
    }

    this->players[0] = new Player(0);
    this->players[1] = new Player(1);
}

Player *Game::player() {
    return this->players[0];
}

Player *Game::opponent() {
    return this->players[1];
}

void Game::updateCell(int x, int y, const string &ore, int hole) {
    int oreAmount{0};
    bool oreVisible{false};

    if (ore != "?") {
        oreAmount = stoi(ore);
        oreVisible = true;
    }

    this->grid[x][y]->update((bool) hole, oreVisible, oreAmount);
}

void Game::updateEntity(int id, int type, int x, int y, int _item) {
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

    auto *point = new Point{x, y};

    // 0 for your robot, 1 for other robot, 2 for radar, 3 for trap
    switch (type) {
        case 0:
            this->player()->updateRobot(id, point, item, type);
            break;
        case 1:
            this->opponent()->updateRobot(id, point, item, type);
            break;
        case 2: {
            auto *radar = new Entity(id, Type::RADAR, point, item, 0);
            this->radars.emplace_back(radar);
            break;
        }
        case 3: {
            auto *trap = new Entity(id, Type::TRAP, point, item, 0);
            this->traps.emplace_back(trap);
            break;
        }
        default:
            break;
    }
}

Individual::Individual(int playerId) {
    this->playerId = playerId;
}

void Individual::randomize() {
    for (int i = 0; i < DEPTH; i++) {
        for (int j = 0; j < ROBOTS; j++) {
            this->actions[i][j] = new Action();
            this->actions[i][j]->type = "DIG";
            this->actions[i][j]->destination->x = random_num(8, WIDTH);
            this->actions[i][j]->destination->y = random_num(0, HEIGHT);
        }
    }
}

double Individual::evaluate() {

}

void Individual::simulate(Individual *individual) {
    for (int i = 0; i < DEPTH; i++) {
        for (int j = 0; j < ROBOTS; j++) {

        }
    }
}

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

    auto *best = new Individual(0);
    auto *bestOpponent = new Individual(1);

    // game loop
    while (true) {
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

        // *************************************************************************************************************
        // <Genetic Evolution>
        // *************************************************************************************************************

        auto **playerPopulation1 = new Individual *[POPULATION_SIZE];
        auto **playerPopulation2 = new Individual *[POPULATION_SIZE];
        Individual **tempPlayerPopulation;

        auto **opponentPopulation1 = new Individual *[POPULATION_SIZE];
        auto **opponentPopulation2 = new Individual *[POPULATION_SIZE];
        Individual **tempOpponentPopulation;

        playerPopulation1[0] = new Individual(0);
        playerPopulation1[0]->randomize();

        opponentPopulation1[0] = new Individual(1);
        opponentPopulation1[0]->randomize();

        for (auto &robot : game->player()->robots) {
            robot->takeAction();
            robot->printAction();
        }

        delete[] playerPopulation1;
        delete[] playerPopulation2;
        delete[] opponentPopulation1;
        delete[] opponentPopulation2;

        turn++;
    }
}

#pragma clang diagnostic pop