//static int debug_counter=0; move_cursor(0,HEIGHT+3); printf("DEBUG %i\n",debug_counter); debug_counter++;
// To-do: Fix code formatting
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // Temporary
#include "display.h"
#include "rpg.h"
#define NEW(x) malloc(sizeof(x))
#define BETW(x,min,max) (min<x&&x<max)
#define WIDTH 80
#define HEIGHT 24
#define AREA (WIDTH*HEIGHT)
#define CHECKER(x) (x%2^(x/WIDTH%2))
#define ABS(x) ((x)<0?-(x):(x))
#define IN_BOUNDS(dest,pos) (ABS(dest%WIDTH-pos%WIDTH)==WIDTH-1||0>dest||dest>AREA-1)
// bool type definition
typedef enum {false,true} bool;
// Tile type definition
typedef struct tile_t {
	creature_t *c;
	char wall,*wall_c,bg,*bg_c;
} tile_t;
// Important creature definitions
static creature_t player={.name="Player",.symbol='@',.color=9,.type=NULL
	,.max_hp=50,.hp=25,.res=10,.agi=5,.wis=7,.str=6 // To-do: Randomize a little
	,.friends=".",.enemies="&",.type=NULL,.surface='B',.dimension='B'};
static creature_t *p_addr=&player;
static type_t zombie_type={.name="Zombie",.symbol='Z',.color=12
	,.hp={25,40},.res={10,10},.agi={1,2},.wis={0,0},.str={5,9}
	,.friends=".",.enemies=".",.surface='B',.dimension='B'};
static type_t *zombie=&zombie_type;
// Item "creature" definitions
static type_t portal_type={.name="Portal",.symbol='O',.color=10
	,.hp={0,0},.res={0,0},.agi={0,0},.wis={0,0},.str={0,0}
	,.friends=".",.enemies=".",.surface='G',.dimension='F'};
static type_t *portal=&portal_type;
static type_t scepter_type={.name="Scepter",.symbol='I',.color=10
	,.hp={0,0},.res={0,0},.agi={0,0},.wis={0,0},.str={0,0}
	,.friends=".",.enemies=".",.surface='F',.dimension='D'};
static type_t *scepter=&scepter_type;
// Function prototypes
void draw_tile(tile_t *tile);
void draw_pos(tile_t *zone,int pos);
void draw_board(tile_t *zone);
bool char_in_string(char c,char *string);
int dir_offset(char dir);
void set_wall (tile_t *tile,char wall,char *wall_c);
void set_bg (tile_t *tile,char bg,char *bg_c);
void set_tile (tile_t *tile,char wall,char *wall_c,char bg,char *bg_c);
tile_t *random_tile(tile_t *zone);
tile_t *random_empty_tile(tile_t *zone);
tile_t *random_grass(tile_t *zone);
tile_t *random_floor(tile_t *zone);
int max_damage(creature_t *attacker,creature_t *defender);
int attack_damage(creature_t *attacker,creature_t *defender);
char move_tile(tile_t *zone,int pos,char dir);
bool will_flee(creature_t *flee_r,creature_t *flee_e);
char decide_move_direction(tile_t *zone,int i);
void update (tile_t *zone);
bool try_summon(tile_t *tile,type_t *type);
void spawn_player(tile_t *zone,int *pc);
char move_player(tile_t *zone,char dir,int *pc);
void make_wall(tile_t *tile,int pos);
void set_floor(tile_t *tile,int pos);
void set_door(tile_t *tile);
void make_building(tile_t *zone,int pos,int h,int w);
void make_random_building(tile_t *zone);
void cull_walls(tile_t *zone);
int *rand_fixed_sum(int n,int max);
tile_t *find_surface(tile_t *zone,char surface);
void create_field(tile_t *field);
void create_dungeon(tile_t *dungeon);
int dist_to_wall(tile_t *zone,int pos,char dir);
bool make_path(tile_t *zone,int pos);
void look_mode(tile_t *zone);
// Global definitions
static char
	*blood="\e[1;37;41m",
    	*grass_colors[2]={TERM_COLORS_40M[GREEN],TERM_COLORS_40M[LGREEN]},
	*floor_colors[2]={TERM_COLORS_40M[WHITE],TERM_COLORS_40M[LGRAY]},
	*wall_colors[2]={TERM_COLORS_40M[RED],TERM_COLORS_40M[LRED]},
	*rock_colors[2]={TERM_COLORS_40M[GRAY],TERM_COLORS_40M[LGRAY]},
	*grass_chars="\"\',.`";
static type_t *typelist=NULL;
static int lines_printed=1;
static FILE *debug_log;
// Main function
int main(int argc,char **argv)
{
	// Open the debug log
	debug_log=fopen("debug.log","w+");
	// Seed the RNG
	unsigned int seed=time(NULL);
	if (argc>1) {
		sscanf(argv[1],"%u",&seed);
	}
	fprintf(debug_log,"Seed: %u\n",seed);
	srand(seed);
	// Set terminal attributes
	set_terminal_canon(false);
	set_cursor_visibility(0);
	// Variable definitions
	tile_t *field=malloc(AREA*sizeof(tile_t))
		,*dungeon=malloc(AREA*sizeof(tile_t))
		,*c_z;
	int p_c;
	// Initialize field
	create_field(field);
	c_z=field;
	// Initialize player
	spawn_player(c_z,&p_c);
	// Draw board
	clear_screen();
	draw_board(c_z);
	move_cursor(0,HEIGHT);
	print_creature(p_addr);
	// Control loop
	char input;
	bool has_scepter=false;
	for (;;) {
		input=fgetc(stdin);
		if (input==27&&fgetc(stdin)==91)
			input=fgetc(stdin);
		//fprintf(debug_log,"Input registered: \'%c\'\n",input);
		if (input=='q')
			break;
		if (input=='?')
			look_mode(c_z);
		else if (input=='>'&&c_z[p_c].bg=='>'&&c_z[p_c].c==p_addr) {
			fprintf(debug_log,"Entering dungeon!\n");
			c_z[p_c].c=NULL;
			create_dungeon(dungeon);
			c_z=dungeon;
			for (p_c=0;c_z[p_c].bg!='<';p_c++);
			c_z[p_c].c=p_addr;
			draw_board(c_z);
			continue;
		} else if (input=='<'&&c_z[p_c].bg=='<'&&c_z[p_c].c==p_addr) {
			fprintf(debug_log,"Exiting dungeon!\n");
			c_z[p_c].c=NULL;
			c_z=field;
			for (p_c=0;c_z[p_c].bg!='>';p_c++);
			c_z[p_c].c=p_addr;
			draw_board(c_z);
			continue;
		} else if (input=='c') {
			input=fgetc(stdin);
			int target=p_c+dir_offset(input);
			if (c_z[target].bg=='-'&&!c_z[target].c)
				set_wall(&(c_z[target]),'+',TERM_COLORS_40M[BROWN]);
			draw_pos(c_z,target);
			update(c_z);
			continue;
		} else if (input=='z'&&has_scepter) {
			fprintf(debug_log,"Summoning zombie!\n");
			int target=p_c+dir_offset(fgetc(stdin));
			try_summon(&c_z[target],zombie);
			draw_pos(c_z,target);
			update(c_z);
			continue;
		} else if (input=='o'&&has_scepter) {
			input=fgetc(stdin);
			fprintf(debug_log,"Opening portal!\n");
			int target=p_c+2*dir_offset(input);
			if (!c_z[p_c+dir_offset(input)].wall
					&&!c_z[p_c+dir_offset(input)].c
					&&!c_z[target].wall
					&&!c_z[target].c) {
				c_z[target].c=make_creature(portal);
				draw_pos(c_z,target);
			}
			update(c_z);
			continue;
		} else if (input=='R'&&has_scepter
				&&p_addr->hp<0) {
			fprintf(debug_log,"Resurrecting player!\n");
			c_z[p_c].c=p_addr;
			has_scepter=false;
			p_addr->color=9;
			p_addr->hp=1;
			draw_pos(c_z,p_c);
		} else if (input=='S') {
			clear_screen();
			draw_board(c_z);
			continue;
		}
		switch (move_player(c_z,input,&p_c)) {
			case 'I':
				has_scepter=true;
				p_addr->color=10;
				draw_pos(c_z,p_c);
				break;
			case 'O':
				fprintf(debug_log,"Entering portal!\n");
				create_field(field);
				fprintf(debug_log,"Spawning player...\n");
				spawn_player(field,&p_c);
				field[p_c].c=p_addr;
				draw_board(field);
				c_z=field;
				fprintf(debug_log,"Done!\n");
				continue;

		}
		update(c_z);
	}
	// Clean up terminal
	set_terminal_canon(true);
	printf("%s",RESET_COLOR);
	set_cursor_visibility(1);
	move_cursor(0,HEIGHT+lines_printed);
	// Exit (success)
	return 0;
}
// Function definitions
void draw_tile(tile_t *tile)
{
	if (tile->c)
		printf("%s%c",TERM_COLORS_40M[tile->c->color],tile->c->symbol);
	else if (tile->wall)
		printf("%s%c",tile->wall_c,tile->wall);
	else
		printf("%s%c",tile->bg_c,tile->bg);
	/*
	if (tile->c)
		printf("%s%c",tile->c->hp>0?TERM_COLORS_40M[tile->c->color]:blood
				,tile->c->symbol);
	else
		printf("%s%c",tile->bg_c,tile->bg);
	*/
}
void draw_pos(tile_t *zone,int pos)
{
	move_cursor(pos%WIDTH,pos/WIDTH);
	draw_tile(&zone[pos]);
}
void draw_board(tile_t *zone)
{
	for (int i=0;i<AREA;i++)
		draw_pos(zone,i);
}
bool char_in_string(char c,char *string)
{
	for (;*string;string++)
		if (c==*string)
			return true;
	return false;
}
int dir_offset(char dir)
{
	int offset=0;
	if (char_in_string(dir,"kyuA789")) // North
		offset-=WIDTH;
	else if (char_in_string(dir,"jbnB123")) // South
		offset+=WIDTH;
	if (char_in_string(dir,"hybD147")) // West
		offset-=1;
	else if (char_in_string(dir,"lunC369")) // East
		offset+=1;
	return offset;
}
void set_wall (tile_t *tile,char wall,char *wall_c)
{
	tile->wall=wall;
	tile->wall_c=wall_c;
}
void set_bg (tile_t *tile,char bg,char *bg_c)
{
	tile->bg=bg;
	tile->bg_c=bg_c;
}
void set_tile (tile_t *tile,char wall,char *wall_c,char bg,char *bg_c)
{
	set_wall(tile,wall,wall_c);
	set_bg(tile,bg,bg_c);
}
tile_t *random_tile(tile_t *zone)
{
	return &zone[rand()%AREA];
}
tile_t *random_empty_tile(tile_t *zone)
{
	tile_t *tile=random_tile(zone);
	if (!tile->wall)
		return tile;
	return random_empty_tile(zone);
}
tile_t *random_grass(tile_t *zone)
{
	tile_t *tile=random_empty_tile(zone);
	if (char_in_string(tile->bg,grass_chars))
		return tile;
	return (random_grass(zone));
}
tile_t *random_floor(tile_t *zone)
{
	tile_t *tile=random_empty_tile(zone);
	if (tile->bg=='#')
		return tile;
	return (random_floor(zone));
}
int max_damage(creature_t *attacker,creature_t *defender)
{
	return 5*attacker->str-(defender->str/2);
}
int attack_damage(creature_t *attacker,creature_t *defender)
{
	int damage;
	if (D(20)>10+defender->agi-attacker->agi) {
		damage=max_damage(attacker,defender);
		return damage<0?0:D(damage);
	} else
		return -1;
}
char move_tile(tile_t *zone,int pos,char dir)
{
	int dest=pos+dir_offset(dir);
	if (IN_BOUNDS(dest,pos))
		return '\0';
	tile_t *from=&zone[pos],*to=&zone[dest];
	if ((to->wall&&to->wall!='+')||(to->c&&char_in_string(to->c->symbol,from->c->friends)))
		// Creatures can not pass through walls or attack creatures they like
		return '\0';
	// If the destination is not the source
	if (dest!=pos) {
		if (to->c) {
			// To-do: Abstract this
			int damage=attack_damage(from->c,to->c);
			if (damage<0) {
				fprintf(debug_log,"%s attacked %s but missed"
						,from->c->name
						,to->c->name);
				move_cursor(0,HEIGHT+lines_printed);
				clear_line();
				lines_printed++;
				printf("%s%c%s attacked %s%c%s but missed"
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR
						,TERM_COLORS_40M[to->c->color]
						,to->c->symbol
						,RESET_COLOR);
			} else if (damage==0) {
				fprintf(debug_log,"%s attacked %s but was too weak to cause damage"
						,from->c->name
						,to->c->name);
				move_cursor(0,HEIGHT+lines_printed);
				clear_line();
				lines_printed++;
				printf("%s%c%s attacked %s%c%s but was too weak to cause damage"
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR
						,TERM_COLORS_40M[to->c->color]
						,to->c->symbol
						,RESET_COLOR);
			} else {
				fprintf(debug_log,"%s attacked %s for %i damage"
						,from->c->name
						,to->c->name
						,damage);
				move_cursor(0,HEIGHT+lines_printed);
				clear_line();
				lines_printed++;
				printf("%s%c%s attacked %s%c%s for %i damage"
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR
						,TERM_COLORS_40M[to->c->color]
						,to->c->symbol
						,RESET_COLOR
						,damage);
				to->c->hp-=damage;
			}
			if (to->c->hp>0) {
				fprintf(debug_log,". It has %i health left.\n",to->c->hp);
				printf(". It has %i health left.\n",to->c->hp);
				return '\0';
			}
			else { // To-do: Special cases for entering portals or collecting scepters
				fprintf(debug_log,", killing it!\n");
				printf(", killing it!\n");
			}
		}
		char killed='\''; // (Represents grass)
		// If the destination is a door
		if (to->wall=='+') {
			// Open it, not moving the creature
			set_wall(to,'\0',NULL);
			draw_pos(zone,dest);
			// Report failure to move
			return '\0';
		}
		// If there was a creature there and it is dead now
		if (to->c&&to->c->hp<=0) {
			if (to->c->symbol=='O') { // If it was a portal
				fprintf(debug_log,"%c entered a portal!\n",from->c->symbol);
				if (from->c!=p_addr) // If the player didn't enter it
					free(from->c);
				from->c=NULL;
				free(to->c);
				to->c=NULL;
				draw_pos(zone,pos);
				draw_pos(zone,dest);
				return 'O';
			} else if (to->c->symbol=='I') {
				fprintf(debug_log,"%c picked up a scepter!\n",from->c->symbol);
				free(to->c);
				to->c=from->c;
				from->c=NULL;
				draw_pos(zone,pos);
				draw_pos(zone,dest);
				return 'I';
			}
			killed=to->c->symbol; // Remember what was killed
			fprintf(debug_log,"%c killed a %c at [%i]\n",from->c->symbol,to->c->symbol,dest);
			// If it's not on stairs, place a corpse
			if (!char_in_string(to->bg,"<>"))
				set_bg(to,to->c->symbol,blood);
		} else if (to->c&&to->c->hp>0) {
			return '\0';
		}
		// Move the symbol and color
		//set_wall(to,from->wall,from->wall_c);
		/**/
		if ((to->c)&&to->c!=p_addr)
			free(to->c);
		to->c=from->c;
		from->c=NULL;
		/**/
		set_wall(from,'\0',NULL);
		// Redraw the changed positions
		draw_pos(zone,pos);
		draw_pos(zone,dest);
		// Return the value of what was killed
		return killed;
	} else
		return '\0';
}
bool will_flee(creature_t *flee_r,creature_t *flee_e)
{
	// Smart creatures should be better at knowing when to flee.
	// Others just make a guess.
	int estimated_damage=flee_r->wis>5?max_damage(flee_e,flee_r):attack_damage(flee_e,flee_r);
	return estimated_damage>flee_r->hp||flee_r->res*2<flee_e->str+flee_e->agi;
}
///////////////////////////////////////////////////////
char alternate_direction(tile_t *zone,int pos,char dir)
{
	int alts[10][2]={{0,0},{2,4},{1,3},{2,6},{1,7},{5,5},{3,9},{4,8},{7,9},{6,8}};
	char tries[2];
	tries[0]=alts[dir-'0'][0]+'0';
	tries[1]=alts[dir-'0'][1]+'0';
	bool occupied[2]={false,false};
	for (int i=0;i<2;i++) {
		int dest=pos+dir_offset(tries[i]);
		if (IN_BOUNDS(dest,pos))
			continue;
		tile_t tile=zone[dest];
		if (tile.c||tile.wall)
			occupied[i]=true;
	}
	if (occupied[0]^occupied[1])
		return occupied[1]?tries[0]:tries[1];
	else
		return occupied[0]?'\0':(rand()%2?tries[0]:tries[1]);
}
///////////////////////////////////////////////////////
char decide_move_direction(tile_t *zone,int i)
{
	char dir='\0';
	for (int j=1;j<=9;j++) {
		if (j==5)
			continue;
		int char_dir=j+'0';
		int dest=i+dir_offset(char_dir);
		if (IN_BOUNDS(dest,i))
			continue;
		// If they are not the adjacent creature's friend and fail a flee check
		if (zone[dest].c&&!char_in_string(zone[i].c->symbol,zone[dest].c->friends)
				&&will_flee(zone[i].c,zone[dest].c)) {
			dir=(5-(j-5))+'0'; // Reverse direction
			dest=i+dir_offset(dir);
			if (zone[dest].c||zone[dest].wall) { // If the creature is blocked
				dir=alternate_direction(zone,i,dir); // Try an alternate direction
				dir=dir?dir:char_dir; // If cornered, fight back
			}
			return dir;
		}
	}
	for (int j=1;j<=9;j++) {
		if (j==5)
			continue;
		int char_dir=j+'0';
		int dest=i+dir_offset(char_dir);
		if (IN_BOUNDS(dest,i))
			continue; // If an adjacent creature is their enemy
		if (zone[dest].c&&char_in_string(zone[dest].c->symbol,zone[i].c->enemies)) {
			dir=j+'0'; // Go that direction
			return dir;
		}
	}
	return '1'+rand()%9; // If there isn't a good direction to pick, move randomly
}
void update(tile_t *zone)
{
	// Make a matrix of the creatures
	char dir;
	creature_t *creature_copies[AREA];
	for (int i=0;i<AREA;i++)
		creature_copies[i]=zone[i].c;
	// For each occupied space, if it hasn't moved yet, move it
	for (int i=0;i<AREA;i++)
		if (creature_copies[i]&&creature_copies[i]==zone[i].c&&creature_copies[i]->hp>0&&creature_copies[i]!=p_addr) {
			// Give it a small chance to regenerate 1 HP
			if (zone[i].c->hp<zone[i].c->max_hp)
				zone[i].c->hp+=0==rand()%10;
			// Figure out what direction it should go in
			char dir=decide_move_direction(zone,i);
			// Perform extra actions based on collision
			switch (move_tile(zone,i,dir)) {
				case 'I':
					//zone[i+dir_offset(dir)].wall_c=TERM_COLORS_40M[PURPLE];
					zone[i+dir_offset(dir)].c->color=PURPLE;
					draw_pos(zone,i+dir_offset(dir));
					break;
				case 'O':
					draw_pos(zone,i+dir_offset(dir));
			}
		}
	/* Redraw the player's stats */
	move_cursor(0,HEIGHT);
	clear_line();
	print_creature(p_addr);
	/**/
}
bool try_summon(tile_t *tile,type_t *type)
{
	if (!tile->wall&&!tile->c) {
		tile->c=make_creature(type);
		return true;
	}
	return false;
}
void spawn_player(tile_t *zone,int *pc)
{
	*pc=rand()%AREA;
	if (!zone[*pc].wall&&!zone[*pc].c)
		zone[*pc].c=p_addr;
	else
		spawn_player(zone,pc);
}
char move_player(tile_t *zone,char dir,int *pc)
{
	// Give the player a small chance to regenerate 1 HP
	if (p_addr->hp<p_addr->max_hp&&p_addr->hp>0)
		p_addr->hp+=0==rand()%10;
	// Clear lines under screen and reset counter
	for (int i=1;i<=lines_printed;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
	lines_printed=1;
	// Player can't move without health
	if (p_addr->hp<0)
		return '\0';
	char result='\0';
	if (zone[*pc].c!=p_addr) // If the player coordinate has the wrong address
		return '\0'; // Don't move
	result=move_tile(zone,*pc,dir); // Move the player
	if (result)
		*pc+=dir_offset(dir); // Correct the player coordinate
	return result; // Report movement result
}
void make_wall(tile_t *tile,int pos)
{
	set_tile(tile,'%',wall_colors[CHECKER(pos)]
			,'%',wall_colors[CHECKER(pos)]);
}
void set_floor(tile_t *tile,int pos)
{
	set_tile(tile,'\0',NULL,'#',floor_colors[CHECKER(pos)]);
}
void set_door(tile_t *tile)
{
	set_tile(tile,'+',TERM_COLORS_40M[BROWN],'-',TERM_COLORS_40M[BROWN]);
}
void make_building(tile_t *zone,int pos,int w,int h)
{
	// Floors
	for (int y=pos;y<=pos+h*WIDTH;y+=WIDTH)
		for (int x=y;x<=y+w;x++)
			set_floor(&zone[x],x);
	// Horizontal walls
	for (int i=pos;i<=pos+w;i++) {
		make_wall(&zone[i],i);
		make_wall(&zone[i+h*WIDTH],i+h*WIDTH);
	}
	// Vertical walls
	for (int i=pos;i<=pos+h*WIDTH;i+=WIDTH) {
		make_wall(&zone[i],i);
		make_wall(&zone[i+w],i+w);
	}
	// Door
	switch (rand()%4) {
		case 0: // North
			set_door(&zone[pos+w/2]);
			break;
		case 1: // South
			set_door(&zone[pos+h*WIDTH+w/2]);
			break;
		case 2: // West
			set_door(&zone[pos+h/2*WIDTH]);
			break;
		case 3: // East
			set_door(&zone[pos+w+h/2*WIDTH]);
	}
}
void make_random_building(tile_t *zone)
{
	int w=3+rand()%(WIDTH/4),h=3+rand()%(HEIGHT/4);
	int pos=(1+rand()%(WIDTH-w-2))+(1+rand()%(HEIGHT-h-2))*WIDTH;
	make_building(zone,pos,w,h);
}
void cull_walls(tile_t *zone)
{
	// Beware: ugly formatting ahead
	int walls_removed;
	do {
		walls_removed=0;
		for (int i=0;i<AREA;i++)
			if (char_in_string(zone[i].wall,"%+")
					&&((zone[i+1].bg=='#'
					&&zone[i-1].bg=='#')
					||(zone[i+WIDTH].bg=='#'
					&&zone[i-WIDTH].bg=='#'))) {
				set_floor(&zone[i],i);
				walls_removed++;
			}
	} while (walls_removed>0);
}
int *rand_fixed_sum(int n,int max)
{
	int *arr=malloc(sizeof(int)*(n+1));
	for (int i=0;i<n-1;i++)
		arr[i]=rand()%max;
	arr[n-1]=0;
	arr[n]=max;
	// Bubblesort
	int sorted=0;
	do {
		sorted=1;
		for (int i=1;i<n+1;i++)
			if (arr[i]<arr[i-1]) {
				sorted=0;
				int tmp=arr[i];
				arr[i]=arr[i-1];
				arr[i-1]=tmp;
			}
	} while (!sorted);
	// Difference transform
	int *tmp=arr;
	int *new_arr=malloc(sizeof(int)*n);
	for (int i=0;i<n;i++)
		new_arr[i]=arr[i+1]-arr[i];
	arr=new_arr;
	free(tmp);
	return arr;
}
tile_t *find_surface(tile_t *zone,char surface)
{
	switch (surface)
	{
		case 'G':
			return random_grass(zone);
		case 'F':
			return random_floor(zone);
		default:
			return random_empty_tile(zone);
	}
}
void create_field(tile_t *field)
{
	int typelist_length=0;
	fprintf(debug_log,"Growing grass...\n");
	for (int i=0;i<AREA;i++) {
		set_tile(&field[i],'\0',NULL,grass_chars[rand()%5]
				,grass_colors[rand()%2]);
		free(field[i].c);
		field[i].c=NULL;// (and removing old creatures)
	}
	fprintf(debug_log,"Building buildings...\n");
	for (int i=0;i<AREA/96;i++)
		make_random_building(field);
	fprintf(debug_log,"Culling walls...\n");
	cull_walls(field);
	fprintf(debug_log,"Summoning creatures...\n");
	if (!typelist) {
		typelist=read_type_list("index");
		//add_type(random_type(),typelist); // Get some wildcards
		/*
		// Print out the creature types
		clear_screen();
		move_cursor(0,0);
		print_type_list(typelist);
		sleep(1);
		*/
	}
	// Figure out how many creature types can spawn in this zone
	for (type_t *t=typelist;t;t=t->next)
		if (t->dimension!='D')
			typelist_length++;
	// Find that many random numbers such that the sum is AREA/48
	int *pops=rand_fixed_sum(typelist_length,AREA/48);
	int i=0;
	// Spawn them appropriately
	for (type_t *t=typelist;t;t=t->next) {
		if (t->dimension!='D') {
			for (int j=0;j<pops[i];j++)
				find_surface(field,t->surface)->c=make_creature(t);
			i++;
		}
	}
	// Summon a random beast
	random_empty_tile(field)->c=random_creature();
	fprintf(debug_log,"Digging stairwell...\n");
	set_bg(random_floor(field),'>',TERM_COLORS_40M[BROWN]);
	fprintf(debug_log,"Opening portal...");
	random_grass(field)->c=make_creature(portal);
	fprintf(debug_log,"Done!\n");
}
void create_dungeon(tile_t *dungeon)
{
	int m=AREA/96,b=AREA/96,typelist_length=0;
	fprintf(debug_log,"Placing rocks...\n");
	for (int i=0;i<AREA;i++) {
		set_tile(&dungeon[i],'%',rock_colors[rand()%2],'\0',NULL);
		free(dungeon[i].c);
		dungeon[i].c=NULL; // (and removing old creatures)
	}
	fprintf(debug_log,"Carving rooms...\n");
	for (int i=0;i<b;i++)
		make_random_building(dungeon);
	fprintf(debug_log,"Culling walls...\n");
	cull_walls(dungeon);
	fprintf(debug_log,"Spawning creatures...\n");
	for (type_t *t=typelist;t;t=t->next)
		if (t->dimension!='F')
			typelist_length++;
	int *pops=rand_fixed_sum(typelist_length,m);
	int i=0;
	for (type_t *t=typelist;t;t=t->next) {
		if (t->dimension!='F') {
			for (int j=0;j<pops[i];j++)
				find_surface(dungeon,t->surface)->c=make_creature(t);
			i++;
		}
	}
	fprintf(debug_log,"Crafting scepter...\n");
	random_empty_tile(dungeon)->c=make_creature(scepter);
	fprintf(debug_log,"Digging stairwell...\n");
	set_bg(random_floor(dungeon),'<',TERM_COLORS_40M[BROWN]);
	if (b>1) {
		fprintf(debug_log,"Mapping tunnels...\n");
		for (int i=0;i<AREA/240;i++)
			while (!make_path(dungeon,rand()%AREA));
	}
	fprintf(debug_log,"Done!\n");
}
int dist_to_wall(tile_t *zone,int pos,char dir)
{
	int dist=0,dest=pos;
	if (dir_offset(dir)==0)
		return -1;
	for (;zone[pos].bg!='%';pos=dest) {
		dest=pos+dir_offset(dir);
		dist++;
		if (ABS(pos%WIDTH-dest%WIDTH)==WIDTH-1
				||dest<0||dest>AREA)
			return -1;
	}
	return dist;
}
bool make_path(tile_t *zone,int pos)
{
	fprintf(debug_log,"Trying to make a path at %i.\n",pos);
	if (!char_in_string(zone[pos].bg,grass_chars)&&zone[pos].bg) {
		fprintf(debug_log,"Invalid path origin.\n");
		return false;
	}
	fprintf(debug_log,"Start position looks okay...\n");
	fprintf(debug_log,"Trying to determining path direction...\n");
	int dist[2]={AREA,AREA};
	char dirs[2]={'\0','\0'};
	for (int i=1;i<=4;i++) {
		int d=dist_to_wall(zone,pos,2*i+'0');
		if (d>0&&d<dist[1]) {
			if (d<dist[0]) {
				dist[1]=dist[0];
				dist[0]=d;
				dirs[1]=dirs[0];
				dirs[0]=2*i+'0';
			}
			else {
				dist[1]=d;
				dirs[1]=2*i+'0';
			}
		}
	}
	if (dist[1]==AREA||!dirs[1]) {
		fprintf(debug_log,"No valid path direction. Stopping.\n");
		return false;
	}
	fprintf(debug_log,"Placing path tiles...\n");
	for (int i=0;i<2;i++) {
		int d=pos+dist[i]*dir_offset(dirs[i]);
		for (int j=0;j<dist[i];j++) {
			int p=pos+j*dir_offset(dirs[i]);
			set_floor(&zone[p],p);
		}
		set_door(&zone[d]);
	}
	fprintf(debug_log,"Finished making a path.\n");
	return true;
}
void look_mode(tile_t *zone)
{
	move_cursor(0,HEIGHT);
	clear_line();
	printf("Inspecting tile...");
	int look_coord=0;
	for (;look_coord<AREA;look_coord++)
		if (zone[look_coord].c==p_addr)
			break;
	if (look_coord==AREA)
		look_coord=AREA/2+WIDTH/2;
	move_cursor(look_coord%WIDTH,look_coord/WIDTH);
	printf("%sX%s",TERM_COLORS_40M[7],RESET_COLOR);
	char input='.';
	do {
		// Clear the info from the screen
		for (int i=1;i<13;i++) {
			move_cursor(0,HEIGHT+i);
			clear_line();
		}
		// Check if the requested movement is out of bounds
		int dest=look_coord+dir_offset(input);
		if (IN_BOUNDS(dest,look_coord))
			continue;
		// Get rid of the old yellow X
		draw_pos(zone,look_coord);
		// Update the look cursor's position and draw it
		look_coord=dest;
		move_cursor(look_coord%WIDTH,look_coord/WIDTH);
		printf("%sX%s",TERM_COLORS_40M[7],RESET_COLOR);
		move_cursor(0,HEIGHT+1);
		// Draw whatever is at the position under the X
		if (zone[look_coord].c) {
			print_creature(zone[look_coord].c);
			putchar('\n');
			if (zone[look_coord].c->type) {
				print_type(zone[look_coord].c->type);
				int count=0;
				for (int i=0;i<AREA;i++)
					if (zone[i].c&&zone[i].c->type==zone[look_coord].c->type)
						count++;
				printf("# of this type remaining: %i",count);
			} else
				printf("[UNIQUE]");
		}
	} while ('q'!=(input=fgetc(stdin)));
	// Get rid of the yellow X
	draw_pos(zone,look_coord);
	// Clear the info from the screen
	for (int i=1;i<13;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
}
