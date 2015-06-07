#ifndef __hdr_prs_h
#define __hdr_prs_h
#include <stdio.h>

struct vcd_var
{
	struct vcd_scope *prev;///<backptr...
	char *tp;///<integer,wire ..etc
	size_t width;///<a number
	char *id;///<shorthand id
	char *name;///<just the name without the scope
	char *arr;///<array expression.. not always there...
};

struct vcd_scope
{
	struct vcd_scope *prev;///<backptr...
	char *s_type;///<function,module etc...
	char *s_name;///<a name
	
	size_t n_vars;///<number of variables at this level
	struct vcd_var *vars;///<at this nesting level
	
	size_t n_scopes;///<number of contained scopes
	struct vcd_scope *scopes;///<contained scopes
};

struct vcd_hdr
{
	struct vcd_scope base;///<simplifies things..
	char *tscale;///<100ps or somesuch...
	unsigned start_time;///<this is at the start of the dumpvars...
};

int vcdReadHeader(struct vcd_hdr *hdr,FILE *fp);
void vcdClearHeader(struct vcd_hdr *hdr);

void vcdParseVal(char *v,char ** bitz_ret,char ** id_ret);
#endif

