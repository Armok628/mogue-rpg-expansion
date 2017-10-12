// To-do: Fix code formatting
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "display.h"
#include "rpg.h"
#define NEW(x) malloc(sizeof(x))
#define BETW(x,min,max) (min<x&&x<max)
#define WIDTH 80
#define HEIGHT 24
#define AREA (WIDTH*HEIGHT)
#define CHECKER(x) (x%2^(x/WIDTH%2))
#define ABS(x) ((x)<0?-(x):(x))
#define OUT_OF_BOUNDS(dest,pos) (ABS(dest%WIDTH-pos%WIDTH)==WIDTH-1||0>dest||dest>AREA-1)
#define NEXT_LINE() move_cursor(0,HEIGHT+lines_printed); lines_printed++;
// bool type definition
typedef enum {false,true} bool;
// Tile type definition
typedef struct tile_t {
	creature_t *c;
	creature_t *corpse;
	char wall,*wall_c,bg,*bg_c;
} tile_t;
// Important creature definitions
static creature_t player_c={.name="Player",.symbol='@',.color=9,.type=NULL
	,.max_hp=50,.hp=25,.res=10,.agi=5,.wis=7,.str=6 // To-do: Randomize a little
	,.friends=".",.enemies="&",.surface='B',.dimension='B'};
static creature_t *player=&player_c;
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
// Spell definitions (possibly temporary)
spell_t missile={.name="Magic Missile",.cost=30,.target=TARGET,.effect=DAMAGE};
spell_t mend={.name="Mend Self",.cost=20,.target=SELF,.effect=HEAL/**/,.next=&missile/**/};
spell_t touch={.name="Deathly Touch",.cost=100,.target=TOUCH,.effect=DAMAGE/**/,.next=&mend/**/};
spell_t raise={.name="Raise Dead",.cost=100,.target=TARGET,.effect=RESURRECT/**/,.next=&touch/**/};
// Function prototypes
void draw_tile(tile_t *tile);
void draw_pos(tile_t *zone,int pos);
void draw_board(tile_t *zone);
bool char_in_string(char c,char *string);
int dir_offset(char dir);
void set_wall (tile_t *tile,char wall,char *wall_c);
void set_bg (tile_t *tile,char bg,char *bg_c);
void set_tile (tile_t *tile,char wall,char *wall_c,char bg,char *bg_c);
int random_empty_coord(tile_t *zone);
int random_grass_coord(tile_t *zone);
int random_floor_coord(tile_t *zone);
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
int dig_staircase(tile_t *zone,char dir);
void create_dungeon(tile_t *dungeon);
int dist_to_wall(tile_t *zone,int pos,char dir);
bool make_path(tile_t *zone,int pos);
bool visible(tile_t *zone,int c1,int c2);
int look_mode(tile_t *zone,int look_coord);
void player_cast_spell(tile_t *c_z,int p_c);
void cast_spell(tile_t *zone,int caster_coord,spell_t *spell,int target_coord);
void hide_invisible_tiles(tile_t *zone,int coord);
void free_creature(creature_t *c);
void free_creatures(tile_t *zone);
void free_typelist(type_t *type);
// Global definitions
static char
    	*grass_colors[2]={TERM_COLORS_40M[GREEN],TERM_COLORS_40M[LGREEN]},
	*floor_colors[2]={TERM_COLORS_40M[WHITE],TERM_COLORS_40M[LGRAY]},
	*wall_colors[2]={TERM_COLORS_40M[RED],TERM_COLORS_40M[LRED]},
	*rock_colors[2]={TERM_COLORS_40M[GRAY],TERM_COLORS_40M[LGRAY]},
	*grass_chars="\"\',.`";
static type_t *typelist=NULL;
static int lines_printed=1;
// Main function
int main(int argc,char **argv)
{
	fprintf(stderr,"Compiled on %s at %s\n",__DATE__,__TIME__);
	// Seed the RNG
	unsigned int seed=time(NULL);
	if (argc>1) {
		sscanf(argv[1],"%u",&seed);
	}
	fprintf(stderr,"Seed: %u\n",seed);
	srand(seed);
	// Set terminal attributes
	set_terminal_canon(false);
	set_cursor_visibility(0);
	// Variable definitions
	tile_t *field=calloc(AREA,sizeof(tile_t))
		,*dungeon=calloc(AREA,sizeof(tile_t))
		,*c_z;
	int p_c,fs_c;
	// Initialize field
	create_field(field);
	fs_c=dig_staircase(field,'>');
	c_z=field;
	// Initialize player
	spawn_player(c_z,&p_c);
	// Draw board
	clear_screen();
	draw_board(c_z);
	move_cursor(0,HEIGHT);
	print_creature(player);
	// Control loop
	char input;
	bool has_scepter=false;
	for (;;) {
		input=fgetc(stdin);
		if (input==27&&fgetc(stdin)==91)
			input=fgetc(stdin);
		if (input=='q')
			break;
		if (input=='?') {
			move_cursor(0,HEIGHT);
			clear_line();
			printf("Inspecting tile...");
			look_mode(c_z,p_c);
		} else if (input=='H') {
			hide_invisible_tiles(c_z,p_c);
			continue;
		} else if (input=='>'&&c_z[p_c].bg=='>'&&c_z[p_c].c==player) {
			c_z[p_c].c=NULL;
			create_dungeon(dungeon);
			c_z=dungeon;
			p_c=dig_staircase(dungeon,'<');
			c_z[p_c].c=player;
			draw_board(c_z);
			continue;
		} else if (input=='<'&&c_z[p_c].bg=='<'&&c_z[p_c].c==player) {
			c_z[p_c].c=NULL;
			c_z=field;
			p_c=fs_c;
			c_z[p_c].c=player;
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
			int target=p_c+dir_offset(fgetc(stdin));
			try_summon(&c_z[target],zombie);
			draw_pos(c_z,target);
			update(c_z);
			continue;
		} else if (input=='o'&&has_scepter) {
			input=fgetc(stdin);
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
				&&player->hp<0) {
			c_z[p_c].c=player;
			c_z[p_c].corpse=NULL;
			has_scepter=false;
			player->color=9;
			player->hp=1;
			draw_pos(c_z,p_c);
		} else if (input=='S') {
			clear_screen();
			draw_board(c_z);
			continue;
		}
		switch (move_player(c_z,input,&p_c)) {
			case 'I':
				has_scepter=true;
				player->color=10;
				draw_pos(c_z,p_c);
				break;
			case 'O':
				free_creatures(field);
				create_field(field);
				fs_c=dig_staircase(field,'>');
				spawn_player(field,&p_c);
				field[p_c].c=player;
				draw_board(field);
				c_z=field;
				continue;

		}
		update(c_z);
	}
	free_creatures(field);
	free(field);
	free_creatures(dungeon);
	free(dungeon);
	free_typelist(typelist);
	// Clean up terminal
	set_terminal_canon(true);
	printf("%s",RESET_COLOR);
	set_cursor_visibility(1);
	NEXT_LINE();
	// Exit (success)
	return 0;
}
// Function definitions
void draw_tile(tile_t *tile)
{
	if (tile->c)
		printf("%s%c",TERM_COLORS_40M[tile->c->color]
				,tile->c->symbol);
	else if (tile->wall)
		printf("%s%c",tile->wall_c,tile->wall);
	else if (tile->corpse)
		printf("%s%c",TERM_COLORS_41M[tile->corpse->color]
				,tile->corpse->symbol);
	else
		printf("%s%c",tile->bg_c,tile->bg);
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
int random_empty_coord(tile_t *zone)
{
	int c;
	do
		c=rand()%AREA;
	while (zone[c].wall||zone[c].c);
	return c;
}
int random_grass_coord(tile_t *zone)
{
	int c;
	do
		c=random_empty_coord(zone);
	while (!char_in_string(zone[c].bg,grass_chars));
	return c;
}
int random_floor_coord(tile_t *zone)
{
	int c;
	do
		c=random_empty_coord(zone);
	while (zone[c].bg!='#');
	return c;
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
	if (OUT_OF_BOUNDS(dest,pos))
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
				NEXT_LINE();
				printf("%s%c%s attacked %s%c%s but missed"
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR
						,TERM_COLORS_40M[to->c->color]
						,to->c->symbol
						,RESET_COLOR);
			} else if (damage==0) {
				NEXT_LINE();
				printf("%s%c%s attacked %s%c%s but was too weak to cause damage"
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR
						,TERM_COLORS_40M[to->c->color]
						,to->c->symbol
						,RESET_COLOR);
			} else {
				NEXT_LINE();
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
				printf(". It has %i health left.\n",to->c->hp);
				return '\0';
			}
			else { // To-do: Special cases for entering portals or collecting scepters
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
				if (from->c!=player) // If the player didn't enter it
					free_creature(from->c);
				from->c=NULL;
				free_creature(to->c);
				to->c=NULL;
				draw_pos(zone,pos);
				draw_pos(zone,dest);
				return 'O';
			} else if (to->c->symbol=='I') {
				free_creature(to->c);
				to->c=from->c;
				from->c=NULL;
				draw_pos(zone,pos);
				draw_pos(zone,dest);
				return 'I';
			}
			killed=to->c->symbol; // Remember what was killed
			if (to->corpse)
				free(to->corpse);
			// If it's not on stairs, place a corpse
			if (!char_in_string(to->bg,"<>")) {
				to->corpse=to->c;
			}
			to->c=NULL;
			if (!(rand()%5)&&from->c->str<10) {
				NEXT_LINE();
				printf("%s%c%s is now stronger from it."
						,TERM_COLORS_40M[from->c->color]
						,from->c->symbol
						,RESET_COLOR);
				from->c->str++;
			}
		} else if (to->c&&to->c->hp>0) {
			return '\0';
		}
		// Move the creature
		/*
		if (to->c&&to->c!=player)
			free_creature(to->c);
		*/
		to->c=from->c;
		from->c=NULL;
		// Redraw the changed positions
		draw_pos(zone,pos);
		draw_pos(zone,dest);
		if (!(rand()%300)&&to->c->agi<10) {
			NEXT_LINE();
			printf("%s%c%s is now more agile from moving."
					,TERM_COLORS_40M[to->c->color]
					,to->c->symbol
					,RESET_COLOR);
			to->c->agi++;
		}
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
char alternate_direction(tile_t *zone,int pos,char dir)
{
	int alts[10][2]={{0,0},{2,4},{1,3},{2,6},{1,7},{5,5},{3,9},{4,8},{7,9},{6,8}};
	char tries[2];
	tries[0]=alts[dir-'0'][0]+'0';
	tries[1]=alts[dir-'0'][1]+'0';
	bool occupied[2]={false,false};
	for (int i=0;i<2;i++) {
		int dest=pos+dir_offset(tries[i]);
		if (OUT_OF_BOUNDS(dest,pos))
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
char decide_move_direction(tile_t *zone,int i)
{
	char dir='\0';
	for (int j=1;j<=9;j++) {
		if (j==5)
			continue;
		int char_dir=j+'0';
		int dest=i+dir_offset(char_dir);
		if (OUT_OF_BOUNDS(dest,i))
			continue;
		// If they are not the adjacent creature's friend and fail a flee check
		if (zone[dest].c&&!char_in_string(zone[i].c->symbol,zone[dest].c->friends)
				&&will_flee(zone[i].c,zone[dest].c)) {
			dir=(5-(j-5))+'0'; // Reverse direction
			dest=i+dir_offset(dir);
			if (OUT_OF_BOUNDS(dest,i)||zone[dest].c||zone[dest].wall) { // If the creature is blocked
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
		if (OUT_OF_BOUNDS(dest,i))
			continue;
		// If an adjacent creature is their enemy
		if (zone[dest].c&&char_in_string(zone[dest].c->symbol,zone[i].c->enemies)) {
			dir=j+'0'; // Go that direction
			return dir;
		}
	}
	return '1'+rand()%9; // If there isn't a good direction to pick, move randomly
}
bool creature_cast_spell(tile_t *zone,int coord)
{
	// Pick a spell
	int spell_count=0;
	spell_t *spell=zone[coord].c->spell;
	for (spell_t *s=spell;s;s=s->next)
		spell_count++;
	int chosen_spell=rand()%spell_count;
	for (int i=0;i<chosen_spell;i++)
		spell=spell->next;
	// Pick a target
	int target,targets[AREA/48],index=0;
	switch (spell->target) {
		case SELF:
			target=coord;
			break;
		case TARGET:
			for (int i=0;i<AREA;i++)
				if (i!=coord&&(spell->target==RESURRECT?zone[i].corpse:zone[i].c)
						&&visible(zone,coord,i)) {
					targets[index]=i;
					index++;
				}
			break;
		case TOUCH:
			for (int i=1;i<=9;i++) {
				int os=dir_offset(i+'0');
				if (os!=0&&(spell->target==RESURRECT?zone[coord+os].corpse
							:zone[coord+os].c)) {
					targets[index]=i;
					index++;
				}
			}
	}
	if (spell->target!=SELF) {
		// If there is no target
		if (!index)
			return false; // Fail
		target=targets[rand()%index];
		// If the target is inappropriate
		if ((char_in_string(zone[target].c->symbol,zone[coord].c->enemies)&&spell->effect!=DAMAGE)
				||(char_in_string(zone[target].c->symbol,zone[coord].c->friends)&&spell->effect==DAMAGE))
			return false; // Fail
	}
	cast_spell(zone,coord,spell,target);
	return true;
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
		if (creature_copies[i]&&creature_copies[i]==zone[i].c
				&&creature_copies[i]->hp>0
				&&creature_copies[i]!=player) {
			// Give it a small chance to regenerate 1 HP
			if (zone[i].c->hp<zone[i].c->max_hp)
				zone[i].c->hp+=0==rand()%10;
			// Decide whether to cast a spell
			if (zone[i].c->spell&&rand()%128<zone[i].c->wis) {
				if (creature_cast_spell(zone,i))
					continue;
			}
			// Figure out what direction it should go in
			char dir=decide_move_direction(zone,i);
			// Perform extra actions based on collision
			switch (move_tile(zone,i,dir)) {
				case 'I':
					zone[i+dir_offset(dir)].c->color=PURPLE;
					draw_pos(zone,i+dir_offset(dir));
					break;
				case 'O':
					draw_pos(zone,i+dir_offset(dir));
			}
		}
	// Redraw the player's stats
	move_cursor(0,HEIGHT);
	clear_line();
	print_creature(player);
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
		zone[*pc].c=player;
	else
		spawn_player(zone,pc);
}
char move_player(tile_t *zone,char dir,int *pc)
{
	// Give the player a small chance to regenerate 1 HP
	if (player->hp<player->max_hp&&player->hp>0)
		player->hp+=0==rand()%10;
	// Clear lines under screen and reset counter
	for (int i=1;i<=lines_printed;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
	lines_printed=1;
	// Player can't do anything without health
	if (player->hp<0)
		return '\0';
	if (dir=='m') { // If the player is casting something
		player_cast_spell(zone,*pc);
	}
	char result='\0';
	if (zone[*pc].c!=player) // If the player coordinate has the wrong address
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
		for (int i=WIDTH+1;i<AREA-WIDTH-1;i++)
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
			return &zone[random_grass_coord(zone)];
		case 'F':
			return &zone[random_floor_coord(zone)];
		default:
			return &zone[random_empty_coord(zone)];
	}
}
void create_field(tile_t *field)
{
	int typelist_length=0;
	free_creatures(field);
	// Set all tiles to grass
	for (int i=0;i<AREA;i++)
		set_tile(&field[i],'\0',NULL,grass_chars[rand()%5]
				,grass_colors[rand()%2]);
	for (int i=0;i<AREA/96;i++)
		make_random_building(field);
	cull_walls(field);
	if (!typelist) {
		typelist=read_type_list("index");
		//add_type(random_type(),typelist); // Get some wildcards
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
			for (int j=0;j<pops[i];j++) {
				tile_t *tile=find_surface(field,t->surface);
				tile->c=make_creature(t);
				/* Temporary. To-do: Determine which creatures get what spells */
				tile->c->spell=tile->c->wis>3?&mend:NULL;
				/**/

			}
			i++;
		}
	}
	free(pops);
	// Summon a random beast
	field[random_empty_coord(field)].c=random_creature();
	// Place a portal
	field[random_grass_coord(field)].c=make_creature(portal);
}
int dig_staircase(tile_t *zone,char dir)
{
	int c=random_floor_coord(zone);
	set_bg(&zone[c],dir,TERM_COLORS_40M[BROWN]);
	return c;
}
void create_dungeon(tile_t *dungeon)
{
	int m=AREA/96,b=AREA/96,typelist_length=0;
	free_creatures(dungeon);
	for (int i=0;i<AREA;i++)
		set_tile(&dungeon[i],'%',rock_colors[rand()%2],'\0',NULL);
	for (int i=0;i<b;i++)
		make_random_building(dungeon);
	cull_walls(dungeon);
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
	free(pops);
	dungeon[random_empty_coord(dungeon)].c=make_creature(scepter);
	if (b>1) {
		for (int i=0;i<AREA/240;i++)
			while (!make_path(dungeon,rand()%AREA));
	}
}
int dist_to_wall(tile_t *zone,int pos,char dir)
{
	int dist=0,dest=pos;
	if (dir_offset(dir)==0)
		return -1;
	for (;zone[pos].bg!='%';pos=dest) {
		dest=pos+dir_offset(dir);
		dist++;
		if OUT_OF_BOUNDS(dest,pos)
			return -1;
	}
	return dist;
}
bool make_path(tile_t *zone,int pos)
{
	if (!char_in_string(zone[pos].bg,grass_chars)&&zone[pos].bg) {
		return false;
	}
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
		return false;
	}
	for (int i=0;i<2;i++) {
		int d=pos+dist[i]*dir_offset(dirs[i]);
		for (int j=0;j<dist[i];j++) {
			int p=pos+j*dir_offset(dirs[i]);
			set_floor(&zone[p],p);
		}
		set_door(&zone[d]);
	}
	return true;
}
bool visible(tile_t *zone,int c1,int c2)
{
	int p[2]={c1%WIDTH,c1/WIDTH};
	int q[2]={c2%WIDTH,c2/WIDTH};
	int vec[2]={q[0]-p[0],q[1]-p[1]};
	float magn=sqrt(pow(vec[0],2)+pow(vec[1],2));
	float unitv[2]={vec[0]/magn,vec[1]/magn};
	for (float x=p[0],y=p[1];round(x)!=q[0]||round(y)!=q[1];x+=unitv[0],y+=unitv[1]) {
		int coord=round(x)+round(y)*WIDTH;
		if (zone[coord].wall)
			return false;
	}
	return true;
}
int look_mode(tile_t *zone,int look_coord)
{
	int start=look_coord;
	bool vis;
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
		if (OUT_OF_BOUNDS(dest,look_coord))
			continue;
		// Get rid of the old X
		draw_pos(zone,look_coord);
		// Update the look cursor's position and draw it
		look_coord=dest;
		vis=visible(zone,start,look_coord);
		move_cursor(look_coord%WIDTH,look_coord/WIDTH);
		printf("%sX%s",TERM_COLORS_40M[vis?7:3],RESET_COLOR);
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
	} while ((input=fgetc(stdin))!='q'&&input!='\n');
	// Get rid of the yellow X
	draw_pos(zone,look_coord);
	// Clear the info from the screen
	for (int i=1;i<13;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
	if (input=='\n')
		return (vis?1:-1)*look_coord;
	return -1;
}
void player_cast_spell(tile_t *c_z,int p_c)
{
	creature_t *c=player;
	/* Temporary */
	if (!player->spell)
		player->spell=&raise;
	/**/
	for (int i=1;i<lines_printed;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
	lines_printed=1;
	for (spell_t *s=c->spell;s;s=s->next) {
		NEXT_LINE();
		print_spell(s);
	}
	int num_spells=lines_printed-2,selected=0;
	spell_t *spell;
	char input='.';
	do {
		// Reset the color of the last spell highlighted
		spell=c->spell;
		for (int i=0;i<selected;i++)
			spell=spell->next;
		move_cursor(0,HEIGHT+selected+1);
		printf("%s%s",RESET_COLOR,spell->name);
		if (dir_offset(input)==WIDTH&&selected<num_spells)
			selected++;
		else if (dir_offset(input)==-WIDTH&&selected>0)
			selected--;
		// Reprint the current spell with a brighter color
		spell=c->spell;
		for (int i=0;i<selected;i++)
			spell=spell->next;
		move_cursor(0,HEIGHT+selected+1);
		printf("\e[1;33;44m%s",spell->name); // (yellow on blue)
	} while ((input=fgetc(stdin))!='q'&&input!='\n');
	// Clean up
	for (int i=0;i<lines_printed;i++) {
		move_cursor(0,HEIGHT+i);
		clear_line();
	}
	lines_printed=1;
	int target_coord;
	if (input=='q')
		return;
	move_cursor(0,HEIGHT);
	clear_line();
	printf("Selecting target...");
	switch (spell->target) {
		case SELF:
			target_coord=p_c;
			break;
		case TOUCH:
			target_coord=p_c+dir_offset(fgetc(stdin));
			break;
		case TARGET:
			target_coord=look_mode(c_z,p_c);
	}
	cast_spell(c_z,p_c,spell,target_coord);
}
void cast_spell(tile_t *zone,int caster_coord,spell_t *spell,int target_coord)
{
	creature_t *caster=zone[caster_coord].c;
	// Roll for fail to cast
	if (rand()%10<(spell->cost/10)-caster->wis&&rand()%10) {
		NEXT_LINE();
		printf("%s%c%s tries to cast %s but fails!"
				,TERM_COLORS_40M[caster->color]
				,caster->symbol
				,RESET_COLOR
				,spell->name);
		return;
	}
	tile_t *target=&zone[target_coord];
	// If the target is invalid
	if (target_coord<0||!(spell->effect!=RESURRECT?target->c:target->corpse)
			||(spell->target!=SELF&&caster_coord==target_coord))
		return; // Cancel
	int effect=1+rand()%(spell->cost/(11-caster->wis));
	NEXT_LINE();
	// Print caster and spell name
	printf("%s%c%s casts %s "
			,TERM_COLORS_40M[caster->color]
			,caster->symbol
			,RESET_COLOR
			,spell->name);
	switch (spell->target) {
		case SELF:
			printf("on itself, ");
			break;
		default:
			if (spell->effect!=RESURRECT)
				printf("on %s%c%s, "
						,TERM_COLORS_40M[target->c->color]
						,target->c->symbol
						,RESET_COLOR);
			else
				printf("on %s%c%s, "
						,TERM_COLORS_41M[target->corpse->color]
						,target->corpse->symbol
						,RESET_COLOR);
			break;
	}
	// Enact and print resulting effect
	switch (spell->effect) {
		case HEAL:
			printf("restoring %i health.",effect);
			target->c->hp+=effect;
			if (target->c->hp>target->c->max_hp)
				target->c->hp=target->c->max_hp;
			break;
		case DAMAGE:
			printf("doing %i damage",effect);
			target->c->hp-=effect;
			if (target->c->hp<0) {
				printf(" and killing it!");
				target->corpse=target->c;
				target->c=NULL;
				draw_pos(zone,target_coord);
			} else
				printf(".");
			break;
		case RESURRECT:
			printf("animating the corpse with %i health.",effect);
			if (target->c)
				break;
			target->c=target->corpse;
			target->corpse=NULL;
			target->c->hp=effect;
			if (target->c->hp>target->c->max_hp)
				target->c->hp=target->c->max_hp;
			draw_pos(zone,target_coord);
	}
	if (!(rand()%20)&&caster->wis<10) {
		NEXT_LINE();
		printf("%s%c%s is now wiser from it."
				,TERM_COLORS_40M[caster->color]
				,caster->symbol
				,RESET_COLOR);
		caster->wis++;
	}
}
void hide_invisible_tiles(tile_t *zone,int coord)
{
	clear_screen();
	for (int i=0;i<AREA;i++)
		if (visible(zone,coord,i))
			draw_pos(zone,i);
}
void free_creature(creature_t *c)
{
	if (!c||c==player)
		return;
	if (!c->type)
		free(c->name);
	free(c);
}
void free_creatures(tile_t *zone)
{
	for (int i=0;i<AREA;i++) {
		free_creature(zone[i].c);
		zone[i].c=NULL;
		free_creature(zone[i].corpse);
		zone[i].corpse=NULL;
	}
}
void free_typelist(type_t *type)
{
	while (typelist) {
		type_t *next=typelist->next;
		free(typelist->name);
		free(typelist->friends);
		free(typelist->enemies);
		free(typelist);
		typelist=next;
	}
}
