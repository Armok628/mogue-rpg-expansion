#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "display.h"
#define ARCHETYPE_SCAN_STR \
"%s \'%c\' %i %ih%i %ir%i %ia%i %iw%i %is%i F%s E%s %c%c"
#define RAND_IN_RANGE(min,max) (min+rand()%(max-min+1))
#define D(n) (1+rand()%n)
#define MAX(x,y) (x>y?x:y)
#define MIN(x,y) (x>y?y:x)
typedef unsigned char byte;
/*
enum itemflags_e {WEAPON=1,ARMOR=2,MAGIC=4,CURSED=8};
typedef struct {
	char *name;
	int value;
	byte flags;
} item_t;
*/
enum spellflags_target_e {SELF=1,TOUCH=2,TARGET=4};
enum spellflags_effect_e {DAMAGE=1,HEAL=2,RESURRECT=255};
typedef struct spell_e {
	char *name;
	int cost;
	byte target;
	byte effect;
	struct spell_e *next;
} spell_t;
typedef struct type_s {
	int res[2],agi[2],wis[2],str[2],hp[2],color;
	char *name,symbol,*friends,*enemies,surface,dimension;
	struct type_s *next;
} type_t;
typedef struct {
	int res,agi,wis,str,max_hp,hp,color;
	char *name,symbol,*friends,*enemies,surface,dimension;
	type_t *type;
	spell_t *spell;
} creature_t;
// Note: Friends can not be attacked. Enemies will be attacked.
// Anything unmentioned can be attacked incidentally
type_t *add_type(type_t *arg,type_t *list)
{
	if (!list)
		return arg;
	type_t *a;
	for (a=list;a->next;a=a->next);
	a->next=arg;
	return list;
}
void draw_creature(creature_t *c)
{
	printf("%s%c%s",TERM_COLORS_40M[c->color]
			,c->symbol
			,RESET_COLOR);
}
void draw_corpse(creature_t *c)
{
	printf("%s%c%s",TERM_COLORS_41M[c->color]
			,c->symbol
			,RESET_COLOR);
}
void print_type(type_t *type)
{
	printf("%s: %s%c%s\n",type->name,TERM_COLORS_40M[type->color],type->symbol,RESET_COLOR);
	printf("HP: %i-%i\nRES: %i-%i\nAGI: %i-%i\nWIS: %i-%i\nSTR: %i-%i\n"
			,type->hp[0],type->hp[1]
			,type->res[0],type->res[1]
			,type->agi[0],type->agi[1]
			,type->wis[0],type->wis[1]
			,type->str[0],type->str[1]);
	if (type->friends)
		printf("Friends: %s\n",type->friends);
	else
		printf("No friends\n");
	if (type->enemies)
		printf("Enemies: %s\n",type->enemies);
	else
		printf("No enemies\n");
	switch (type->surface) {
		case 'G':
			printf("Spawns on grass");
			break;
		case 'F':
			printf("Spawns on floors");
			break;
		case 'B':
			printf("Spawns on any surface");
	}
	switch (type->dimension) {
		case 'F':
			printf(" above ground");
			break;
		case 'D':
			printf(" in dungeons");
	}
	putchar('\n');
}
void print_type_list(type_t *type)
{
	for (type_t *t=type;t;t=t->next)
		print_type(t);
}
void print_spell(spell_t *spell)
{
	printf("%s (%i): ",spell->name,spell->cost);
	switch (spell->effect) {
		case DAMAGE:
			printf("Damage ");
			break;
		case HEAL:
			printf("Heal ");
			break;
		case RESURRECT:
			printf("Resurrect ");
			break;
	}
	switch (spell->target) {
		case SELF:
			printf("caster\n");
			break;
		case TOUCH:
			printf("touched creature\n");
			break;
		case TARGET:
			printf("targeted creature\n");
	}
}
type_t *read_type (char *filename)
{
	printf("Scanning file \'%s\'\n",filename);
	FILE *file=fopen(filename,"r");
	if (!file) {
		printf("%s does not exist.\n",filename);
		return NULL;
	}
	type_t *type=malloc(sizeof(type_t));
	int scanned=0;
	char *name=calloc(16,1),*friends=calloc(16,1),*enemies=calloc(16,1);
	if (17==(scanned=fscanf(file,ARCHETYPE_SCAN_STR
					,name,&(type->symbol)
					,&(type->color)
					,&(type->hp[0]),&(type->hp[1])
					,&(type->res[0]),&(type->res[1])
					,&(type->agi[0]),&(type->agi[1])
					,&(type->wis[0]),&(type->wis[1])
					,&(type->str[0]),&(type->str[1])
					,friends,enemies
					,&(type->surface),&(type->dimension)))) {
		type->name=realloc(name,strlen(name)+1);
		type->friends=realloc(friends,strlen(friends)+1);
		type->enemies=realloc(enemies,strlen(enemies)+1);
		type->next=NULL;
		printf("Finished scanning %s\n",type->name);
		fclose(file);
		if (!strcmp(type->friends,"none")) {
			free(type->friends);
			type->friends=NULL;
		}
		if (!strcmp(type->enemies,"none")) {
			free(type->enemies);
			type->enemies=NULL;
		}
		print_type(type);
		return type;
	}
	printf("Error scanning \'%s\'\nScanned stats: %i\n",filename,scanned);
	free(type);
	free(friends);
	free(enemies);
	fclose(file);
	return NULL;
}
void print_creature_stats(creature_t *creature)
{
	printf("%s%s ",RESET_COLOR,creature->name);
	draw_creature(creature);
	printf(" HP: %i/%i | RES: %i | AGI: %i | WIS: %i | STR: %i\n"
			,creature->hp,creature->max_hp
			,creature->res,creature->agi,creature->wis,creature->str);
}
creature_t *make_creature(type_t *type)
{
	creature_t *creature=malloc(sizeof(creature_t));
	creature->name=type->name;
	creature->symbol=type->symbol;
	creature->color=type->color;
	creature->max_hp=RAND_IN_RANGE(type->hp[0],type->hp[1]);
	creature->hp=creature->max_hp;
	creature->res=RAND_IN_RANGE(type->res[0],type->res[1]);
	creature->agi=RAND_IN_RANGE(type->agi[0],type->agi[1]);
	creature->wis=RAND_IN_RANGE(type->wis[0],type->wis[1]);
	creature->str=RAND_IN_RANGE(type->str[0],type->str[1]);
	creature->friends=type->friends;
	creature->enemies=type->enemies;
	creature->type=type;
	creature->spell=NULL;
	return creature;
}
static char *consonants[33]={
	"b","c","d","f","g","h","j","k","l","m","n","p","qu","r","s","t","v","w","x","y","z"
		,"sh","ch","zh","ng","fr","st","sp","sk","tr","kr","fl","gn"};
static char *vowels[36]={
	"a","e","i","o","u","y"
		,"aa","ae","ai","ao","au","ay"
		,"ea","ee","ei","eo","eu","ey"
		,"ia","ie","ii","io","iu","iy"
		,"oa","oe","oi","oo","ou","oy"
		,"ua","ue","ui","uo","uu","uy"};
char *random_word(int length)
{
	char *word=calloc(2*length+1,1);
	word[0]='\0';
	int vowel_start=rand()%2;
	for (int i=0;i<length;i++) {
		if (vowel_start^(i%2))
			strcat(word,vowels[rand()%36]);
		else
			strcat(word,consonants[rand()%33]);
	}
	word=realloc(word,strlen(word)+1);
	return word;
}
creature_t *random_creature()
{
	creature_t *creature=malloc(sizeof(creature_t));
	creature->name=random_word(RAND_IN_RANGE(5,7));
	creature->name[0]-=32;
	creature->symbol=(char)RAND_IN_RANGE(33,126);
	creature->color=RAND_IN_RANGE(2,16);
	creature->max_hp=D(50);
	creature->hp=creature->max_hp;
	creature->res=D(10);
	creature->agi=D(10);
	creature->wis=D(10);
	creature->str=D(10);
	// Find some way to randomize:
	creature->friends=".";
	creature->enemies=".";
	creature->surface='B';
	creature->dimension='B';
	creature->type=NULL;
	creature->spell=NULL;
	return creature;
}
int char_in_string(char c,char *str)
{
	if (!str)
		return 0;
	for (;*str;str++)
		if (c==*str)
			return 1;
	return 0;
}
type_t *random_type(type_t *bestiary)
{
	// Give stuff random values
	type_t *type=malloc(sizeof(type_t));
	type->symbol=(char)RAND_IN_RANGE(33,126);
	type->color=RAND_IN_RANGE(1,16);
	type->name=random_word(RAND_IN_RANGE(2,4));
	type->name[0]-=32;
	int i1=0,i2=0;
	i1=D(100); i2=D(100);
	type->hp[0]=MIN(i1,i2);
	type->hp[1]=MAX(i1,i2);
	i1=D(10); i2=D(10);
	type->res[0]=MIN(i1,i2);
	type->res[1]=MAX(i1,i2);
	i1=D(10); i2=D(10);
	type->agi[0]=MIN(i1,i2);
	type->agi[1]=MAX(i1,i2);
	i1=D(10); i2=D(10);
	type->wis[0]=MIN(i1,i2);
	type->wis[1]=MAX(i1,i2);
	i1=D(10); i2=D(10);
	type->str[0]=MIN(i1,i2);
	type->str[1]=MAX(i1,i2);
	// Give type random spawn conditions
	static char svals[3]={'F','G','B'};
	static char dvals[3]={'F','D','B'};
	type->surface=svals[rand()%3];
	type->dimension=dvals[rand()%3];
	// Figure out how long the type list is
	int count=0;
	for (type_t *b=bestiary;b;b=b->next)
		count++;
	// Allocate memory for worst-case scenario
	type->friends=calloc(count,1);
	type->enemies=calloc(count,1);
	// Collect creature symbols
	char syms[count+2],*sptr=syms+2;
	syms[0]='@'; // Take the player into account
	syms[1]=type->symbol; // Take themselves into account
	for (type_t *b=bestiary;b;b=b->next)
		*(sptr++)=b->symbol;
	// Set random enemy values without repeats
	sptr=type->enemies;
	for (int i=0;i<count+2;i++)
		if (!(rand()%3)&&!char_in_string(syms[i],type->enemies))
			*(sptr++)=syms[i];
	// Set random friend values without repeats
	sptr=type->friends;
	for (int i=0;i<count+2;i++)
		if (!(rand()%3)&&!char_in_string(syms[i],type->enemies)
				&&!char_in_string(syms[i],type->friends))
			*(sptr++)=syms[i];
	// Reallocate to save (a likely miniscule amount of) memory
	if (strlen(type->friends))
		type->friends=realloc(type->friends,strlen(type->friends)+1);
	else {
		free(type->friends);
		type->friends=NULL; // Must be NULL if no entries to be displayed correctly
	}
	if (strlen(type->enemies))
		type->enemies=realloc(type->enemies,strlen(type->enemies)+1);
	else {
		free(type->enemies);
		type->enemies=NULL; // Must be NULL if no entries to be displayed correctly
	}
	// Initialize pointer as a formality
	type->next=NULL;
	//print_type(type);
	return type;
}
type_t *read_type_list(const char *filename)
{
	FILE *creatures=fopen(filename,"r");
	if (!creatures)
		return NULL;
	char *name=calloc(64,1);
	type_t *list=NULL;
	while (!feof(creatures)) {
		fscanf(creatures,"%s",name);
		if (!strcmp(name,"end"))
			break;
		list=add_type(read_type(name),list);
	}
	free(name);
	fclose(creatures);
	return list;
}
