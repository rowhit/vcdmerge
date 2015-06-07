#include "symtab.h"
#include "hdr_cmp.h"
#include "hdr_prs.h"
#include <stdio.h>
#include <stdlib.h>


/*
there's one of those for the output file
on every change populated with values that need dumping..
(full names, not ids)
need separate ids for output. exactly 1 per var name..
*/
struct mergetab
{
	unsigned now;///<current simulation timestamp
	DHash changeset;///<cleared on every change and filled with values that changed, and may need dumping..
};

/*
one id may alias multiple signals in the model
this struct keeps track of the signals for one id.
*/
struct vcd_id
{
	char *id;///<the shorthand id of the signal(s)
	DHash by_name;///<all the fully qualified names that this Id aliases
};


struct merge_ctx
{
	FILE *fpa,*fpb,*fpc;
	struct vcd_hdr ha,hb;
	struct symtab sa,sb;
};




//pickup inits from the file
static void init_value(char * scopes,struct vcd_var *v,void *d)
{
	struct symtab *st=d;
	struct vcd_val *vl=new_vcd_val(scopes,v);
	struct vcd_val cv={.id=
	assert(vl);
}

static void for_each_variable(char * pfx,struct vcd_scope *hdr_a,
void * d,
void (*func)(char * scopes,struct vcd_var *v,void *d))
{
	size_t i;
	pfx=strdup(pfx);
	assert(pfx);
	pfx=realloc(pfx,strlen(pfx)+strlen(hdr_a->s_name)+2);
	assert(pfx);
	strcat(pfx,".");
	strcat(pfx,hdr_a->s_name);
	for(i=0;i<hdr_a->n_vars;i++)
	{
		struct vcd_var *v;
		v=hdr_a->vars+i;
		func(pfx,v,d);
	}
	for(i=0;i<hdr_a->n_scopes;i++)
	{
		struct vcd_scope *s;
		s=hdr_a->scopes+i;
		for_each_variable(pfx,s,d,func);
	}
	free(pfx);
}


/*
splits value v into bits and id.
returns allocated strings in last two parameters
*/


static void init_values(void)
{
	char *tmp=NULL;
	init_symtab(vals_a);

	init_symtab(vals_b);

	for_each_variable("",hdr_a,vals_a,generate_value);
	parse_inits(vals_a);

	for_each_variable("",hdr_b,vals_b,generate_value);
	parse_inits(vals_b);

/*
argh...
doing this wrong...
gotta find the identical 2 vals in the 2 files,
just dump when bits the same, x for nonmatching ones...
ids might differ in two files...
for every var in file a
find val in file b by /name/
combine, dump

processing the changes:
find next change in both files
if they happen at same time, apply both.
else apply earlier one
apply them by finding values by /id/
and changing value

then dump changed ones
also involves:
for every var in file a
find val in file b by /name/
combine, dump

only want 2 dump changed ones, not all..
gotta memorize names. remove duplicates..

and then there's the thing with signals w 
different names having the same ids coz it's really the same..
vcd_var is unique by the names...
I may have to find the varnames from the id.

what if the sharing is different in the two files?
means I can't reuse the ids for the output file.

*/
	dump_initvals(vals_a,vals_b);
}

//return 1 if a is earlier than b
int change_is_sooner(char *a, char *b)
{
	uint64_t t_a=strtoul(a+1,NULL,10),t_b=strtoul(b+1,NULL,10);
	return t_a<t_b;
}

//return 1 if they are identical times
int change_at_same_time(char *a, char *b)
{
	uint64_t t_a=strtoul(a+1,NULL,10),t_b=strtoul(b+1,NULL,10);
	return t_a==t_b;
}

//just return the timestamp....
char *peek_change(FILE * fp)
{
	long pos=ftell(fp);
	char *tmp=NULL;
	size_t n;
	getline(&tmp,&n,fp);
	assert(tmp);
	assert(tmp[0]=='#');
	fseek(fp,pos,SEEK_SET);
	return tmp;
}


#define MAXIMUM(a,b) ((a)>(b)?a:b)

///concat+realloc unterminated 2nd string
int
utlSmartCatUt (char **str_p, size_t *size_p, char *str2, size_t size2)
{
  char *str = *str_p;
  size_t size = *size_p;
  if ((NULL == str2) || (NULL == str_p) || (NULL == size_p))
    return 1;
  if (NULL == str)
  {
    size = MAXIMUM (128, size2 + 1);
    str = utlCalloc (1, size);
    if (!str)
      return 1;
  }
  else if (size < (strlen (str) + size2 + 1))
  {
    char *p;
    size = strlen (str) + size2 + 1 + 128;
    p = utlRealloc (str, size);
    if (!p)
      return 1;
    str = p;
  }
  strncat (str, str2, size2);
  *str_p = str;
  *size_p = size;
  return 0;
}

///concat+realloc
int
utlSmartCat (char **str_p, size_t *size_p, char *str2)
{
  if(NULL!=str2)
  	return utlSmartCatUt (str_p, size_p, str2, strlen (str2));
  return 1;
}

//grab the whole changeset... multiple lines, including timestamp
char * read_change(FILE * fp)
{
	long pos;
	char *rv=NULL;
	ssize_t e;
	int done=0;
	size_t n=0;
	e=getline(&rv,&n,fp);
	if(-1==e)
		return NULL;
	assert(rv);
	assert(rv[0]=='#');
	pos=ftell(fp);
	do
	{
		char *tmp=NULL;
		size_t n2=0;
		e=getline(&tmp,&n2,fp);
		if((-1==e)||(tmp[0]=='#'))
		{
			done=1;
		}
		else
		{
			pos=ftell(fp);
			utlSmartCat(&rv,&n,tmp);
			free(tmp);
		}
	}while(!done);
	fseek(fp,pos,SEEK_SET);
	return rv;
}

void get_next_change(FILE * fp_a,FILE * fp_b,char **a_r,char **b_r)
{
	char *a,*b;
	a=peek_change(fp_a);
	b=peek_change(fp_b);
	if(change_is_sooner(a,b))
	{
		*b_r=NULL;
		*a_r=read_change(fp_a);
	}
	else if(changes_at_same_time(a,b))
	{
		*a_r=read_change(fp_a);
		*b_r=read_change(fp_b);
	}
	else
	{
		*a_r=NULL;
		*b_r=read_change(fp_b);
	}
	free(a);
	free(b);
}

struct change_ctx{
	SListE e;
	int idx;
	char * id;
};

struct cset_ctx{
	struct symtab * st;
	int idx;
	SList cl;
};

	/*
	record values in tables for file a or b
	remember which id/name was modified
	*/
void generate_change(void *d,char * c)
{
	char *s1=NULL,*s2=NULL;
	struct cset_ctx *ds=d;
	struct change_ctx *cc;
	set_val(c,ds->st);
	cc=calloc(sizeof(struct change_ctx),1);
	cc->idx=ds->idx;//what file
	parse_val(v,&s1,&s2);
	cc->id=s2;//memorize id
	free(s1);
	slistInsertLast(&ds->cl,&cc->e);
}

/*
does stuff for each change in a changeset
*/
static void for_each_change(char *cs,void *d,void(*func)(void *d,char * c))
{
	char * svp;
	char * s1;
	cs=strdup(cs);
	assert(cs);
	tok=strtok_r(cs,"\n",&svp);
	assert(tok[0]=='#');
	//skip timestamp
	tok=strtok(NULL,"\n",&svp);
	while(tok)
	{
		func(d,tok);
		tok=strtok(NULL,"\n",&svp);
	}
	free(cs);
}


static void merge_values(SList *l)
{
	struct change_ctx * cc=slistRemoveFirst(l);
	while(cc)
	{
		int idx[2]={cc->idx,1-cc->idx};
		struct vcd_value vv[2]={
		stGetValue(idx[0]);
		stGetValue(idx[1])};
		merge_value(vv,idx,mt);
		free(cc);
		cc=NULL;//do I need that list? could just set a flag on the value...
		cc=slistRemoveFirst(l);
	}
}

void generate_output(FILE * fp,char *c_a,char *c_b)
{
	char * tok;
	char * s1,*s2;
	cset_ctx ds={0};
	if(c_a)
	{
		ds->st=st_a;
		ds->idx=0;
		for_each_change(c_a,&ds,generate_change);
		free(c_a);
	}
	if(c_b)
	{
		ds->st=st_b;
		ds->idx=1;
		for_each_change(c_b,&ds,generate_change);
		free(c_b);
	}
	
	merge_values();
	dump_compromise(fp);
	
	
}

static void process_changes(FILE * fp_a,FILE * fp_b,FILE * fp_c)
{
	int i,j;
	char *c_a,*c_b;
	get_next_change(fp_a,fp_b,&c_a,&c_b);//may have 2 get 2 changesets if same time..
	generate_output(fp_c,c_a,c_b);
}


extern int vcdmerge(char const * fa,char const * fb,char const * fc)
{
//	FILE *fpa,*fpb,*fpc;
//	struct vcd_hdr ha,hb;
	struct merge_ctx ctx={0};
	inr rv=0;
	ctx.fpa=fopen(fa,"rt");
	if(NULL==ctx.fpa)
	{
		rv=1;
		fprintf(stderr,"fopen(%s,\"rt\") failed. reason: %s\n",fa,strerror(errno));
		goto cleanup1;
	}
	ctx.fpb=fopen(fb,"rt");
	if(NULL==ctx.fpb)
	{
		rv=1;
		fprintf(stderr,"fopen(%s,\"rt\") failed. reason: %s\n",fb,strerror(errno));
		goto cleanup2;
	}
	ctx.fpc=fopen(fc,"wtx");
	if(NULL==ctx.fpc)
	{
		rv=1;
		fprintf(stderr,"fopen(%s,\"wtx\") failed. reason: %s\n",fc,strerror(errno));
		goto cleanup3;
	}
	
	vcdReadHeader(&ctx.ha,ctx.fpa);
	vcdReadHeader(&ctx.hb,ctx.fpb);
	
	assert(0==vcdCompareHeaders(&ctx.ha,&ctx.hb));//same timescales, same scopes, vars
	
	vcdInitValues(&ctx.sa,&ctx.ha,ctx.fpa);
	vcdInitValues(&ctx.sb,&ctx.hb,ctx.fpb);
	
	avsGenerate();
	dump_header(&ctx.ha,ctx.fpc);
	
	process_changes(&ctx);
	
	fclose(fc);
	cleanup3:
	fclose(fb);
	cleanup2:
	fclose(fa);
	cleanup1:
	return rv;
}
