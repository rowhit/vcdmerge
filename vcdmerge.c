#include "symtab.h"
#include "hdr_cmp.h"
#include "hdr_prs.h"
#include "val_init.h"
#include "mergetab.h"
#include "atom_val.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

///before the dumpvars #<time>
#define DS_INIT 0
///any dump block (started by #<time>)
#define DS_DUMP 1
///done
#define DS_EOF 2


struct merge_ctx
{
  FILE *fpa, *fpb, *fpc;
  struct vcd_hdr ha, hb;
  struct symtab sa, sb, sc;
//  struct mergetab m;
  int a_st, b_st, c_st;         //parser/dumper states
  int do_diff;
  unsigned now;
};


//return 1 if a is earlier than b
int
change_is_sooner (char *a, char *b)
{
  uint64_t t_a = strtoul (a + 1, NULL, 10), t_b = strtoul (b + 1, NULL, 10);
  return t_a < t_b;
}

//return 1 if they are identical times
int
change_at_same_time (char *a, char *b)
{
  uint64_t t_a = strtoul (a + 1, NULL, 10), t_b = strtoul (b + 1, NULL, 10);
  return t_a == t_b;
}

//just return the timestamp....
char *
peek_change (FILE * fp)
{
  long pos = ftell (fp);
  char *tmp = NULL;
//  size_t n;
  if (EOF == fscanf (fp, "%ms", &tmp))
    return NULL;
  assert (tmp);
  assert (tmp[0] == '#');
  fseek (fp, pos, SEEK_SET);
  return tmp;
}


#define MAXIMUM(a,b) ((a)>(b)?a:b)

static int
read_change (SList * rvl,
             int *what, struct symtab *vals, struct vcd_hdr *h, FILE * fp)
{
  int rv = 0;
  if ((*what) == DS_INIT)
  {
    *what = DS_DUMP;
    rv = vcdParseInits (rvl, vals, h, fp);
    if (rv)
      (*what) = DS_EOF;
    return rv;
  }
  else
  {
    rv = vcdParseChange (rvl, vals, h, fp);
    if (rv)
      (*what) = DS_EOF;
    return rv;
  }
  return rv;
}

/*
process/store the values and record changed ones for later.
process in a way timestamps always increase by a positive, non-zero amount..
So I don't get intervals of length zero..
*/
static void
get_next_change (struct merge_ctx *ctx, SList * a_r, SList * b_r)
{
  FILE *fp_a = ctx->fpa, *fp_b = ctx->fpb;
  char *a,*b;
  a = peek_change (fp_a);
  b = peek_change (fp_b);
  if (change_is_sooner (a, b))
  {
		ctx->now=strtoul (a + 1, NULL, 10);
		slistInit(b_r);
    read_change (a_r, &ctx->a_st, &ctx->sa, &ctx->ha, fp_a);
  }
  else if (change_at_same_time (a, b))
  {
		ctx->now=strtoul (a + 1, NULL, 10);
    read_change (a_r, &ctx->a_st, &ctx->sa, &ctx->ha, fp_a);
    read_change (b_r, &ctx->b_st, &ctx->sb, &ctx->hb, fp_b);
  }
  else
  {
		ctx->now=strtoul (b + 1, NULL, 10);
    slistInit(a_r);
    read_change (b_r, &ctx->b_st, &ctx->sb, &ctx->hb, fp_b);
  }
  free (a);
  free (b);
}

struct change_ctx
{
  SListE e;
  int idx;
  char *id;
};

struct cset_ctx
{
  struct symtab *st;
  int idx;
  SList cl;
};

void
val_dump(void *x, void *this)
{
	struct merge_ctx *ctx=x;
  struct vcd_value *val = this;
  if(val->width==1)
  {
  	fprintf(ctx->fpc,"%c%s\n",val->bitz[0],val->id);
  }
  else
  {
  	size_t i;
  	fprintf(ctx->fpc,"b");
  	for(i=0;i<val->width;i++)
  	{
	  	fprintf(ctx->fpc,"%c",val->bitz[val->width-i-1]);
  	}
  	fprintf(ctx->fpc," %s\n",val->id);
  }
}

static void
nuffin(void *d)
{
}

static void dump_compromise(DHash *cl,struct merge_ctx *ctx)
{
	fprintf(ctx->fpc,"#%u\n",ctx->now);
	if(ctx->c_st==DS_INIT)
	{
		fprintf(ctx->fpc,"$dumpvars\n");
	}
  dhashForEach (cl, ctx, val_dump);
	if(ctx->c_st==DS_INIT)
	{
		fprintf(ctx->fpc,"$end\n");
		ctx->c_st=DS_DUMP;
	}
  dhashClear (cl, nuffin);
	
}

void
generate_output (struct merge_ctx *ctx, SList * c_a, SList * c_b)
{
  DHash cl = { 0 };
  //gotta get a list with id's for c
  //preferrably unique elements(hashmap?)
  //so I know what to dump
  vcdMergeValues (c_a, c_b, &cl, ctx->do_diff);
  
  dump_compromise (&cl,ctx);
}

static void
process_changes (struct merge_ctx *ctx)
{
  SList c_a = { 0 }, c_b =
  {
  0};
  ctx->a_st = DS_INIT;
  ctx->b_st = DS_INIT;
  ctx->c_st = DS_INIT;
  do
  {
    get_next_change (ctx, &c_a, &c_b);
    generate_output (ctx, &c_a, &c_b);
  }
  while (!((ctx->a_st == DS_EOF) && (ctx->b_st == DS_EOF)));
}

void
delcv (void *x, void *this)
{
  struct vcd_value *val = this;
  struct vcd_val_e *ve,*next;
  ve=(struct vcd_val_e *)slistFirst(&val->c_val);
  while(ve)
  {
  	next=(struct vcd_val_e *)slistENext(&ve->e);
  	free(ve);
  	ve=next;
  }
  slistInit(&val->c_val);
}

static void del_cval(struct symtab * st)
{
  dhashForEach (&st->values, NULL, delcv);
}

extern int
vcdmerge (char const *fa, char const *fb, char const *fc, int do_diff)
{
//      FILE *fpa,*fpb,*fpc;
//      struct vcd_hdr ha,hb;
  struct merge_ctx ctx = { 0 };
  int rv = 0;
  ctx.do_diff=do_diff;
  ctx.now=0;
  ctx.fpa = fopen (fa, "rt");
  if (NULL == ctx.fpa)
  {
    rv = 1;
    fprintf (stderr, "fopen(%s,\"rt\") failed. reason: %s\n", fa,
             strerror (errno));
    goto cleanup1;
  }
  ctx.fpb = fopen (fb, "rt");
  if (NULL == ctx.fpb)
  {
    rv = 1;
    fprintf (stderr, "fopen(%s,\"rt\") failed. reason: %s\n", fb,
             strerror (errno));
    goto cleanup2;
  }
  ctx.fpc = fopen (fc, "wt");
  if (NULL == ctx.fpc)
  {
    rv = 1;
    fprintf (stderr, "fopen(%s,\"wtx\") failed. reason: %s\n", fc,
             strerror (errno));
    goto cleanup3;
  }

  vcdReadHeader (&ctx.ha, ctx.fpa);
  vcdReadHeader (&ctx.hb, ctx.fpb);

  assert (0 == vcdCompareHeaders (&ctx.ha, &ctx.hb));   //same timescales, same scopes, vars

  vcdInitValues (&ctx.sa, &ctx.ha);
  vcdInitValues (&ctx.sb, &ctx.hb);

  avsGenerate (&ctx.sa, &ctx.sb, &ctx.sc, do_diff);
  vcdDumpHeader (&ctx.ha, &ctx.sc.by_name, ctx.fpc);

  process_changes (&ctx);
	
	del_cval(&ctx.sa);
  stClear (&ctx.sa);
	del_cval(&ctx.sb);
  stClear (&ctx.sb);
  stClear (&ctx.sc);
	vcdClearHeader(&ctx.ha);
	vcdClearHeader(&ctx.hb);
	
  fclose (ctx.fpc);
cleanup3:
  fclose (ctx.fpb);
cleanup2:
  fclose (ctx.fpa);
cleanup1:
  return rv;
}
