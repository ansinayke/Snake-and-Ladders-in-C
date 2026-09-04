#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>


#define LENGTH 25
#define WIDTH  10
#define MAX_ENTRIES 100
#define BAWANA_CELLS 12
#define MAX_FLOORS 3
#define MAX_VISITS 100


enum Direction { NORTH, 
                 EAST, 
                 SOUTH, 
                 WEST };

enum Status { NORMAL, 
              FOOD_POISONED, 
              DISORIENTED, 
              TRIGGERED, 
              HAPPY };


// Function signatures
char get_cell(int floor, int x, int y);

int is_playable(int floor, int x, int y);   // To omit invalid cells

void draw_floor(int floor);

void game_loop();

int Droll_movement();

int Droll_direction();

void initialize_cells();       // To initialize cell values

void apply_bawana_effect(int p, int idx);

int move_player(int p, int steps, int dir_roll);


// Variables and structures
int rand_seed;

int flag_captured = 0;

typedef struct {
    int startFloor, startY, startX;
    int endFloor, endY, endX;
} Stair;

typedef struct {
    int startFloor, endFloor;
    int y, x;
} Pole;

typedef struct {
    int floor;
    int startY, startX;
    int endY, endX;
} Wall;

typedef struct {
    int floor, y, x;
} Flag;

typedef struct {
    char name;
    int floor, y, x;  // Current pos
    int startFloor, startY, startX; // Starting pos
    int movement_points;  // Bawana rule points
    int in_maze;  // Bool to detect in game area or not
    int throw_count;  // How many dice throws made
    enum Direction direction;
    enum Status status;  // Bawana status 
    int incapacitated_due;  // Throws left to recover
    int disoriented_turns;
} Player;

int bawana_y[BAWANA_CELLS] = {6,6,6,7,7,7,8,8,8,9,9,9};

int bawana_x[BAWANA_CELLS] = {20,21,22,20,21,22,20,21,22,20,21,22};

int bawana_type[BAWANA_CELLS];

int cell_consumable[3][10][25];

int cell_bonus_type[3][10][25];

int cell_bonus_value[3][10][25];

int stair_direction[MAX_ENTRIES];

int visitCount[MAX_FLOORS][WIDTH][LENGTH][3] = {0};  // For loop stuck detection

char* dir_str[4] = {"North", "East", "South", "West"};


// Arrays
Stair stairs[MAX_ENTRIES];
int stairCount = 0;

Pole poles[MAX_ENTRIES];
int poleCount = 0;

Wall walls[MAX_ENTRIES];
int wallCount = 0;

Flag flag;



// Players
Player players[3] = {
    {'A', 0, 6, 12, 0, 6, 12, 100, 0, 0, NORTH, NORMAL, 0, 0},

    {'B', 0, 9, 8, 0, 9, 8, 100, 0, 0, WEST, NORMAL, 0, 0},
    
    {'C', 0, 9, 16, 0, 9, 16, 100, 0, 0, EAST, NORMAL, 0, 0}
};



// File Loaders
void load_stairs(const char *filename) {

    FILE *fp = fopen(filename, "r");

    if (!fp) return;

    while (fscanf(fp, "%d,%d,%d,%d,%d,%d",

            &stairs[stairCount].startFloor, &stairs[stairCount].startY, &stairs[stairCount].startX,

            &stairs[stairCount].endFloor, &stairs[stairCount].endY, &stairs[stairCount].endX) == 6) {

            stairCount++;
    }
    fclose(fp);
}

void load_poles(const char *filename) {

    FILE *fp = fopen(filename, "r");

    if (!fp) return;

    while (fscanf(fp, "%d,%d,%d,%d",

            &poles[poleCount].startFloor, &poles[poleCount].endFloor,

            &poles[poleCount].y, &poles[poleCount].x) == 4) {

            poleCount++;
    }
    fclose(fp);
}

void load_walls(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: could not open walls file %s\n", filename);
        return;
    }
    while (fscanf(fp, "%d,%d,%d,%d,%d",
            &walls[wallCount].floor, &walls[wallCount].startY, &walls[wallCount].startX,
            &walls[wallCount].endY, &walls[wallCount].endX) == 5) {
        
        // check validity
        if (walls[wallCount].floor < 0 || walls[wallCount].floor >= MAX_FLOORS ||
            walls[wallCount].startY < 0 || walls[wallCount].endY >= WIDTH ||
            walls[wallCount].startX < 0 || walls[wallCount].endX >= LENGTH) {
            fprintf(stderr, "Error: invalid wall placement at floor %d, (%d,%d)->(%d,%d)\n",
                    walls[wallCount].floor,
                    walls[wallCount].startY, walls[wallCount].startX,
                    walls[wallCount].endY, walls[wallCount].endX);
        }
        wallCount++;
    }
    fclose(fp);
}

void load_flag(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: could not open flag file %s\n", filename);
        return;
    }
    if (fscanf(fp, "%d,%d,%d", &flag.floor, &flag.y, &flag.x) == 3) {
        if (!is_playable(flag.floor, flag.x, flag.y)) {
            fprintf(stderr, "Error: flag placed at invalid cell (%d,%d) on floor %d\n",
                    flag.y, flag.x, flag.floor);
        }
    }
    fclose(fp);
}

void load_seed(const char *filename) {
    
    FILE *fp = fopen(filename, "r");
    
    if (!fp) return;
    
    fscanf(fp, "%d", &rand_seed);
    
    fclose(fp);
}



// Dice rolls
// Movement dice
int Droll_movement() {

    return (rand() % 6) + 1;
}

// Direction dice
int Droll_direction() {
    
    int face = (rand() % 6) + 1;
    
    switch(face) {
        case 2: return NORTH;
        case 3: return EAST;
        case 4: return SOUTH;
        case 5: return WEST;
        default: return -1; // empty face for face 1 and 6
    }
}



// Printing Floor on the CLI
// Base floor rules
char get_base_cell(int floor, int x, int y) {
   
    if (floor == 0) {
        if (x >= 8 && x <= 16 && y >= 6 && y <= 9) 
            return 's';
        else 
            return '.';
    }
    else if (floor == 1) {
        if ((x >= 0 && x <= 7 && y >= 0 && y <= 9) || (x >= 17 && x <= 24 && y >= 0 && y <= 9))
            return '.';
        if (x >= 8 && x <= 16 && y >= 6 && y <= 9) 
            return '=';
    }
    else if (floor == 2) {
        if (x >= 8 && x <= 16 && y >= 0 && y <= 9)
            return '.';
    }
    return '+'; // invalid cells
}


// Check if a cell is allowed for entities
int is_playable(int floor, int x, int y) {
    
    char base = get_base_cell(floor, x, y);
    
    return (base == '.' || base == '='); 
}


// Final cell with entities/players
char get_cell(int floor, int x, int y) {
    
    char cell = get_base_cell(floor, x, y);

    // Walls
    for (int i = 0; i < wallCount; i++) {
        if (walls[i].floor == floor &&
            
            y >= walls[i].startY && y <= walls[i].endY &&
            x >= walls[i].startX && x <= walls[i].endX &&
            is_playable(floor, x, y)) {
            cell = '#';
        
        }
    }

    // Stairs
    for (int i = 0; i < stairCount; i++) {
        if (((stairs[i].startFloor == floor && stairs[i].startY == y && stairs[i].startX == x) ||
            
            (stairs[i].endFloor == floor && stairs[i].endY == y && stairs[i].endX == x)) &&
            is_playable(floor, x, y)) {
            cell = 'w';
        
        }
    }

    // Poles
    for (int i = 0; i < poleCount; i++) {
        if ((floor >= poles[i].startFloor && floor <= poles[i].endFloor) &&
            
            poles[i].y == y && poles[i].x == x && is_playable(floor, x, y)) {
            cell = 'o';
        
        }
    }

    // Flag
    if (flag.floor == floor && flag.y == y && flag.x == x && is_playable(floor, x, y)) {
        
        cell = 'F';
    
    }

    // Placing players
    for (int i = 0; i < 3; i++) {
        if (players[i].floor == floor && players[i].y == y && players[i].x == x) {
    
            cell = players[i].name;
    
        }
    }

    return cell;
}


// Sketches & floor
void draw_floor(int floor) {
    printf("\n=== FLOOR %d ===\n\n", floor);

    printf("    ");
    for (int x = 0; x < LENGTH; ++x) printf("%2d ", x);
    printf("\n    ");
    for (int x = 0; x < LENGTH; ++x) printf("---");
    printf("\n");

    for (int y = 0; y < WIDTH; y++) {
        printf("%2d |", y);
        for (int x = 0; x < LENGTH; x++) {
            printf(" %c ", get_cell(floor, x, y));
        }
        printf("\n");
    }
    printf("\n");
}



// Player Movements
int move_player(int p, int steps, int dir_roll) {
    
    int cost = 0;
    int bonus_add = 0;
    int bonus_mult = 1;
    int new_floor = players[p].floor;
    int new_y = players[p].y;
    int new_x = players[p].x;
    int effective_steps = steps;

    if (players[p].status == TRIGGERED) effective_steps *= 2;
    if (players[p].status == DISORIENTED) {
        players[p].direction = rand() % 4;
    }

    for (int s = 1; s <= effective_steps; s++) {
        
        int dy = 0, dx = 0;
        
        if (players[p].direction == NORTH) dy = -1;
        else if (players[p].direction == EAST) dx = 1;
        else if (players[p].direction == SOUTH) dy = 1;
        else dx = -1;

        int ny = new_y + dy;
        int nx = new_x + dx;
        if (ny < 0 || ny >= WIDTH || nx < 0 || nx >= LENGTH || get_cell(new_floor, nx, ny) == '#' || get_cell(new_floor, nx, ny) == '+') {
            return 0; // Meetign barriers
        }

        new_y = ny;
        new_x = nx;

        // LOOP DETECTION
        visitCount[new_floor][new_y][new_x][p]++;
        if (visitCount[new_floor][new_y][new_x][p] > MAX_VISITS) {
            fprintf(stderr, "Error: Player %c stuck in loop at (%d,%d) floor %d\n",
                    players[p].name, new_y, new_x, new_floor);
            return 0;
        }
        
        cost += cell_consumable[new_floor][ny][nx];
        
        if (cell_bonus_type[new_floor][ny][nx] == 1) bonus_add += cell_bonus_value[new_floor][ny][nx];
        else if (cell_bonus_type[new_floor][ny][nx] == 2) bonus_mult *= cell_bonus_value[new_floor][ny][nx];
        
        // Check for stairs
        for (int i = 0; i < stairCount; i++) {
            if (stairs[i].startFloor == new_floor && stairs[i].startY == ny && stairs[i].startX == nx) {
                if (stair_direction[i] == 0) {  // Start to end of a stair
                    new_floor = stairs[i].endFloor;
                    new_y = stairs[i].endY;
                    new_x = stairs[i].endX;
                    printf("%c lands on %d,%d which is a stair cell. %c takes the stairs and now placed at %d,%d in floor %d.\n", players[p].name, ny, nx, players[p].name, new_y, new_x, new_floor);
                } else {
                    // Moving from end to start of a stair
                    new_floor = stairs[i].startFloor;
                    new_y = stairs[i].startY;
                    new_x = stairs[i].startX;
                    printf("%c lands on %d,%d which is a stair cell. %c takes the stairs in reverse and now placed at %d,%d in floor %d.\n", players[p].name, ny, nx, players[p].name, new_y, new_x, new_floor);
                }
                break;
            }  // Same logic but reverse application
            else if (stairs[i].endFloor == new_floor && stairs[i].endY == ny && stairs[i].endX == nx) {
                if (stair_direction[i] == 1) {
                    new_floor = stairs[i].startFloor;
                    new_y = stairs[i].startY;
                    new_x = stairs[i].startX;
                    printf("%c lands on %d,%d which is a stair cell. %c takes the stairs in reverse and now placed at %d,%d in floor %d.\n", players[p].name, ny, nx, players[p].name, new_y, new_x, new_floor);
                } else {
                    new_floor = stairs[i].endFloor;
                    new_y = stairs[i].endY;
                    new_x = stairs[i].endX;
                    printf("%c lands on %d,%d which is a stair cell. %c takes the stairs and now placed at %d,%d in floor %d.\n", players[p].name, ny, nx, players[p].name, new_y, new_x, new_floor);
                }
                break;
            }
        }
        
        // Check for poles
        for (int i = 0; i < poleCount; i++) {
            if (poles[i].startFloor <= new_floor && new_floor <= poles[i].endFloor && poles[i].y == ny && poles[i].x == nx) {
                new_floor = poles[i].endFloor;
                new_y = poles[i].y;
                new_x = poles[i].x;
                printf("%c lands on %d,%d which is a pole cell. %c slides down and now placed at %d,%d in floor %d.\n", players[p].name, ny, nx, players[p].name, new_y, new_x, new_floor);
                break;
            }
        }
    }

    // Apply bonuses
    players[p].movement_points += bonus_add;
    players[p].movement_points *= bonus_mult;
    players[p].movement_points -= cost;

    if (players[p].movement_points <= 0) {
        printf("%c movement points are depleted and requires replenishment. Transporting to Bawana.\n", players[p].name);
        int idx = rand() % BAWANA_CELLS;
        players[p].floor = 0;
        players[p].y = bawana_y[idx];
        players[p].x = bawana_x[idx];
        players[p].direction = NORTH;
        apply_bawana_effect(p, idx);
        return 1;
    }

    // Check for player capture
    for (int i = 0; i < 3; i++) {
        if (i != p && players[i].floor == new_floor && players[i].y == new_y && players[i].x == new_x) {
            
            players[i].floor = players[i].startFloor;   // Resetting the values
            players[i].y = players[i].startY;
            players[i].x = players[i].startX;
            players[i].in_maze = 0;
            players[i].movement_points = 100;
            players[i].throw_count = 0;
            players[i].status = NORMAL;
            players[i].incapacitated_due = 0;
            players[i].disoriented_turns = 0;
        }
    }

    // Output movement message
    if (dir_roll != -1) {
        printf("%c rolls %d on the Movement Dice and %s on the Direction Dice, changes direction to %s and moves %d cells and is now at %d,%d.\n", players[p].name, steps, dir_str[dir_roll], dir_str[players[p].direction], effective_steps, new_y, new_x);
    } else if (players[p].status == DISORIENTED) {
        printf("%c rolls %d on the Movement Dice and is disoriented and move in %s and moves %d cells and is placed at %d,%d.\n", players[p].name, steps, dir_str[players[p].direction], effective_steps, new_y, new_x);
    } else if (players[p].status == TRIGGERED) {
        printf("%c is triggered and rolls %d on the Movement Dice and move in %s and moves %d cells and is placed at %d,%d.\n", players[p].name, steps, dir_str[players[p].direction], effective_steps, new_y, new_x);
    } else {
        printf("%c rolls %d on the Movement Dice and moves %s by %d cells and is now at %d,%d.\n", players[p].name, steps, dir_str[players[p].direction], effective_steps, new_y, new_x);
    }
    printf("%c moved %d cells that cost %d movement points and is left with %d and is moving in %s.\n", players[p].name, effective_steps, cost, players[p].movement_points, dir_str[players[p].direction]);
    
    // Handle disoriented recovery
    if (players[p].status == DISORIENTED) {
        
        players[p].disoriented_turns--;
        
        if (players[p].disoriented_turns == 0) {
        
            players[p].status = NORMAL;
            printf("%c has recovered from disorientation.\n", players[p].name);
        }
    }

     // Check for flag capture
    if (new_floor == flag.floor && new_y == flag.y && new_x == flag.x) {
    
        printf("%c captures the flag! %c wins.\nThank You ^ ^\n", players[p].name, players[p].name);
        flag_captured = 1;
    }
    
    players[p].floor = new_floor;
    
    players[p].y = new_y;
    
    players[p].x = new_x;

    return 1;
}



// Maze setup
void initialize_cells() {

    // Same position placement of the players error checking
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (players[i].x == players[j].x &&
                players[i].y == players[j].y &&
                players[i].floor == players[j].floor) {
                fprintf(stderr, "Error: Players %c and %c start at same location (%d,%d) on floor %d.\n",
                        players[i].name, players[j].name, players[i].x, players[i].y, players[i].floor);
            }
        }
    }

    // Value addition to cells
    for (int f = 0; f < 3; f++) {
        for (int y = 0; y < WIDTH; y++) {
            for (int x = 0; x < LENGTH; x++) {
                int r = rand() % 100;
                
                if (r < 25) {
                    cell_consumable[f][y][x] = 0;
                    cell_bonus_type[f][y][x] = 0;
                    cell_bonus_value[f][y][x] = 0;
                } 
                else if (r < 60) {
                    cell_consumable[f][y][x] = (rand() % 4) + 1;
                    cell_bonus_type[f][y][x] = 0;
                    cell_bonus_value[f][y][x] = 0;
                } 
                else if (r < 85) {
                    cell_consumable[f][y][x] = 0;
                    cell_bonus_type[f][y][x] = 1; // add
                    cell_bonus_value[f][y][x] = (rand() % 2) + 1;
                } 
                else if (r < 95) {
                    cell_consumable[f][y][x] = 0;
                    cell_bonus_type[f][y][x] = 1; // add
                    cell_bonus_value[f][y][x] = (rand() % 3) + 3;
                } 
                else {
                    cell_consumable[f][y][x] = 0;
                    cell_bonus_type[f][y][x] = 2; // multiply
                    cell_bonus_value[f][y][x] = (rand() % 2) + 2;
                }
            }
        }
    }
    // Bawana types - 2 each of Food Poisoning, Disoriented, Triggered, Happy, and 4 random movement points
    int types[BAWANA_CELLS] = {0,0,1,1,2,2,3,3,4,4,4,4};
    // Shuffle types
    for (int i = BAWANA_CELLS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = types[i];
        types[i] = types[j];
        types[j] = temp;
    }
    for (int i = 0; i < BAWANA_CELLS; i++) {
        bawana_type[i] = types[i];
    }
}



// Bawana rules effecting
void apply_bawana_effect(int p, int idx) {
    
    int type = bawana_type[idx];
    
    if (type == 0) { // Food Poisoning
        players[p].status = FOOD_POISONED;
        players[p].incapacitated_due = 3;
        players[p].movement_points = 0;
        printf("%c eats from Bawana and have a bad case of food poisoning. Will need three rounds to recover.\n", players[p].name);
    } 
    else if (type == 1) { // Disoriented
        players[p].status = DISORIENTED;
        players[p].movement_points = 50;
        players[p].direction = NORTH;
        players[p].disoriented_turns = 4;
        printf("%c eats from Bawana and is disoriented and is placed at the entrance of Bawana with 50 movement points.\n", players[p].name);
    } 
    else if (type == 2) { // Triggered
        players[p].status = TRIGGERED;
        players[p].movement_points = 50;
        players[p].direction = NORTH;
        printf("%c eats from Bawana and is triggered due to bad quality of food. %c is placed at the entrance of Bawana with 50 movement points.\n", players[p].name, players[p].name);
    } 
    else if (type == 3) { // Happy
        players[p].status = HAPPY;
        players[p].movement_points = 200;
        printf("%c eats from Bawana and is happy. %c is placed at the entrance of Bawana with 200 movement points.\n", players[p].name, players[p].name);
    } 
    else { // Random movement points
        int pts = (rand() % 91) + 10;
        players[p].movement_points += pts;
        printf("%c eats from Bawana and earns %d movement points and is placed at %d,%d.\n", players[p].name, pts, players[p].y, players[p].x);
    }
}



// Game - Main
void game_loop() {

    int round = 0;

    while (!flag_captured) {
        
        round++;
        
        printf("\n\n== Round %d ==\n",round);
        
        if (round % 5 == 0) {       // Shifting directions of stairs in each 5 rounds
            
            for (int i = 0; i < stairCount; i++) {
                stair_direction[i] = rand() % 2;
            }
        }

        for (int p = 0; p < 3; p++) {
            if (players[p].status == FOOD_POISONED && players[p].incapacitated_due > 0) {
                
                printf("%c is still food poisoned and misses the turn.\n", players[p].name);
                players[p].incapacitated_due--;
                if (players[p].incapacitated_due == 0) { // Rocovering from food poisoning
                    int idx = rand() % BAWANA_CELLS;
                    players[p].floor = 0;
                    players[p].y = bawana_y[idx];
                    players[p].x = bawana_x[idx];
                    players[p].direction = NORTH;
                    apply_bawana_effect(p, idx);
                }
                continue;
            }

            int mov = Droll_movement();

            int dir_roll = -1;

            // Direction dice rolling
            if (players[p].in_maze && (players[p].throw_count % 4) == 0) {
                dir_roll = Droll_direction();
                if (dir_roll != -1) {
                    players[p].direction = dir_roll;
                } else {
                    printf("%c player rolled EMPTY side on the Direction Dice. So no change in the direction.",players[p].name);
                }
            }

            // Initial roll to eneter the game block
            if (!players[p].in_maze) {
                if (mov == 6) {
                    players[p].in_maze = 1;
                    players[p].floor = 0;
                    if (p == 0) { players[p].y = 5; players[p].x = 12; players[p].direction = NORTH; }
                    else if (p == 1) { players[p].y = 9; players[p].x = 7; players[p].direction = WEST; }
                    else { players[p].y = 9; players[p].x = 17; players[p].direction = EAST; }
                    players[p].movement_points = 100;
                    printf("%c is at the starting area and rolls 6 on the movement dice and is placed on %d,%d of the maze.\n", players[p].name, players[p].y, players[p].x);
                }
                else {
                    printf("%c is at the starting area and rolls %d on the movement dice cannot enter the maze.\n", players[p].name, mov);
                }
            } 
            else {
                int can_move = move_player(p, mov, dir_roll);
                if (!can_move) {
                    printf("%c rolls %d on the movement dice and cannot move in %s. Player remains at %d,%d\n", players[p].name, mov, dir_str[players[p].direction], players[p].y, players[p].x);
                    players[p].movement_points -= 2;
                }
                players[p].throw_count++;
            }
            if (flag_captured) {
                break;
            }
        }
        if (flag_captured) {
            break;
        }
    }
}


int main(void) {

    FILE *logfile = freopen("log.txt", "w", stdout);    // Writing the output into log.txt
    if (logfile == NULL) {
        perror("Failed to open log.txt");
        return 1;
    }

    FILE *errfile = freopen("errors.txt", "w", stderr);
    if (errfile == NULL) {
        perror("Failed to open errors.txt");
        return 1;
    }

    printf("Hi there! Welcome to The Maze Runner of UCSC.\n\n");

    printf("Let's have some fun ^ ^\n\n");

    sleep(1);

    printf("Description :\n");

    printf("This game has got 3 levels and you have to hunt for the flag placed in one of the floors.\n");

    sleep(1);

    printf("Here is the floor plan for the 3 levels\n");
    printf("\nLegend:\n");
    printf("  + = Invalid Block\n");
    printf("  s = Standing Area\n");
    printf("  . = Game Block\n");
    printf("  = = Bridge\n");
    printf("  w = Stairs\n");
    printf("  o = Pole\n");
    printf("  # = Wall\n");
    printf("  F = Flag\n");
    printf("  A/B/C = Players\n");    

    sleep(1);

    load_stairs("stairs.txt");
    load_poles("poles.txt");
    load_walls("walls.txt");
    load_flag("flag.txt");
    load_seed("seed.txt");

    srand(rand_seed);

    load_stairs("stairs.txt");

    for (int i = 0; i < stairCount; i++) {
        stair_direction[i] = rand() % 2;
    }
    
    initialize_cells();

    draw_floor(0);
    draw_floor(1);
    draw_floor(2);

    game_loop();

    fclose(logfile);

    fclose(errfile);

    return 0;
}