#ifndef __symtab_h
#define __symtab_h

#include "hdr_prs.h"

#include "dhash.h"
#include "list.h"
/*
when reading changesets, 
I need to find the full names+values from the ids
the full names are needed to assign the signal in the output,
because I can not expect the ids of a and b file(and c) 
to match.
This also means the the output file will have exactly 1 id
per variable name, as I can not easily determine which ones I 
could alias without reading everything twice...
Or perhaps I can, by generating (smaller) file c id groups which are completely 
contained by both as and bs ids
a:| 0 | 1  |
c:|0|1|2|3 |
b:|0| 1 | 2|
still a, and b may trigger writes 
to multiple values, but there will be fewer
outputs in file c than if its all separate...
how to find the atomic value sets?
grab all the names for id i in file a,
then get all the ids for those names in file b.
generate equiv-ids for file c, repeat for the other
ids in a.
there's one symtab for each input file.
*/
struct symtab
{
	DHash by_name;///<name->id id2id
	DHash by_id;///<id->nameS id2ids
	DHash values;///<id->value vcd_value
};

struct id2ids
{
	char * id1;
	SList l;//e_str
};

struct e_str
{
	SListE e;
	struct vcd_var *var;
	char s[];
};

struct id2id
{
	char * id1;
	char * id2;
};

struct vcd_value
{
	char *id;
	size_t width;
	char bitz[];
};

int stInit(struct symtab *s);
void stClear(struct symtab *s);
SList *stGetNamesForId(struct symtab *s,char *id);
char *stGetIdForName(struct symtab *s,char *name);
void stAddNameForId(struct symtab *s,char *id,char *name,struct vcd_var *var);
int stPutValue(struct symtab *s,struct vcd_value *v);
struct vcd_value * stGetValue(struct symtab *s,char * id);
struct vcd_value * stRmvValue(struct symtab *s,char * id);

struct vcd_value *vcdValueNew(struct vcd_var *v);
void vcdValueDel(struct vcd_value *v);

#endif

