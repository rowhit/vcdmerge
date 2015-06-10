#include "val_init.h"
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
/*
initializes the symbol table with 
values from the dumpvars section.
fp must be already positioned at the beginning of $dumpvars
*/

static int
is_log_val (char c)
{
  return (c == '0' ||
          c == '1' || c == 'x' || c == 'z' || c == 'X' || c == 'Z');
}


static struct vcd_val_e *
set_val (char *v, struct symtab *st)
{
  char *s1 = NULL, *s2 = NULL;
  struct vcd_val_e *rv;
  struct vcd_value *val;
  char extend;
  size_t i;
  vcdParseVal (v, &s1, &s2);
  val = stGetValue (st, s2);
  assert (val);
  assert (strlen (s1) <= val->width);   // doesn't match exactly... often smaller
  for (i = 0; i < strlen (s1); i++)
  {
    assert (is_log_val (s1[i]));
    val->bitz[strlen (s1) - i - 1] = s1[i];
  }
  extend = (s1[0] == '1') ? '0' : s1[0];
  for (; i < val->width; i++)
  {
    val->bitz[i] = extend;
  }
  rv = calloc (sizeof (struct vcd_val_e), 1);
  rv->vp = val;
  free (s1);
  free (s2);
  return rv;
}

int
vcdParseChange (SList * rvl, struct symtab *vals, struct vcd_hdr *h,
                FILE * fp)
{
  char *tmp;
  long pos;
  size_t n;
  int rv = 0;
  slistInit (rvl);
  if (EOF == fscanf (fp, "%ms\n", &tmp))
    return 1;
  assert (tmp);
  assert (tmp[0] == '#');
  errno = 0;
  h->start_time = strtoul (&tmp[1], NULL, 10);
  assert (0 == errno);
  pos = ftell (fp);
  free (tmp);
  tmp = NULL;
  n = 0;
  rv = getline (&tmp, &n, fp);
  while ((tmp[0] != '#') && (rv != -1))
  {
    slistInsertLast (rvl, &set_val (tmp, vals)->e);
    pos = ftell (fp);
    rv = getline (&tmp, &n, fp);
  }
  fseek (fp, pos, SEEK_SET);
  free (tmp);
  return (rv < 0);
}

int
vcdParseInits (SList * rvl, struct symtab *vals, struct vcd_hdr *h, FILE * fp)
{
  char *tmp;
  size_t n;
  int rv = 0;
  slistInit (rvl);
  if (EOF == fscanf (fp, "%ms", &tmp))
    return 1;
  assert (tmp);
  if (tmp[0] == '#')
  {
    errno = 0;
    h->start_time = strtoul (&tmp[1], NULL, 10);
    assert (0 == errno);
    free (tmp);
    if (EOF == fscanf (fp, "%ms", &tmp))
      return 1;
    assert (tmp);
  }
  assert (!strcmp (tmp, "$dumpvars"));
  free (tmp);
  tmp = NULL;
  n = 0;
  rv = getline (&tmp, &n, fp);
  rv = getline (&tmp, &n, fp);
  while (strncmp ("$end", tmp, 4) && (rv >= 0))
  {
    slistInsertLast (rvl, &set_val (tmp, vals)->e);
    rv = getline (&tmp, &n, fp);
  }
  free (tmp);
  return (rv < 0);
}

static size_t
n_strlen (char *s)
{
  if (NULL == s)
    return 0;
  return strlen (s);
}

static char *
gen_scoped_name (char *scope, char *name)
{
  char *rv;
  rv = calloc (1, n_strlen (scope) + n_strlen (name) + 2);
  if ((NULL == scope) || (0 == strcmp ("", scope)))
  {                             //no scopename, no dot...
    if (NULL != name)
    {
      strcpy (rv, name);
    }
  }
  else
  {
    strcpy (rv, scope);
    if (NULL != name)
    {
      strcat (rv, ".");
      strcat (rv, name);
    }
  }
#ifdef TEST
  puts (rv);
#endif
  return rv;
}

static void
for_each_variable (char *pfx, struct vcd_scope *hdr_a,
                   void *d,
                   void (*func) (char *scopes, struct vcd_var * v, void *d))
{
  size_t i;
  pfx = gen_scoped_name (pfx, hdr_a->s_name);
  assert (pfx);
  for (i = 0; i < hdr_a->n_vars; i++)
  {
    struct vcd_var *v;
    v = hdr_a->vars + i;
    func (pfx, v, d);
  }
  for (i = 0; i < hdr_a->n_scopes; i++)
  {
    struct vcd_scope *s;
    s = hdr_a->scopes + i;
    for_each_variable (pfx, s, d, func);
  }
  free (pfx);
}


static void
generate_value (char *scopes, struct vcd_var *v, void *d)
{
  struct symtab *st = d;
  struct vcd_value *vl = vcdValueNew (v, '0');
  char *fullname;
  assert (vl);
  if (stPutValue (st, vl))
  {
    vcdValueDel (vl);
  }
  fullname = gen_scoped_name (scopes, v->name);
  stAddNameForId (st, v->id, fullname, v);
  free (fullname);
}

void
vcdInitValues (struct symtab *s, struct vcd_hdr *h)
{
  stInit (s);
  for_each_variable ("", &h->base, s, generate_value);
}

static char const *
resolve_id (DHash * idmap, char *fullname)
{
  struct id2id ref = { fullname, NULL };
  struct id2id *rv = dhashGet (idmap, &ref);
  if (NULL == rv)
    return NULL;
  return rv->id2;
}

static void
dump_var (char *scopes, struct vcd_var *v, DHash * idmap, FILE * fp)
{
  char *fullname = NULL;
  fullname = gen_scoped_name (scopes, v->name);
  fprintf (fp, "$var %s %u %s %s ", v->tp, v->width,
           resolve_id (idmap, fullname), v->name);
  if (NULL != v->arr)
  {
    fprintf (fp, "%s $end\n", v->arr);
  }
  else
  {
    fprintf (fp, "$end\n");
  }
  free (fullname);
}


static void
dump_scope (char *pfx, struct vcd_scope *s, DHash * idmap, FILE * fp)
{
  size_t i;
  pfx = gen_scoped_name (pfx, s->s_name);
  assert (pfx);

  fprintf (fp, "$scope %s %s $end\n", s->s_type, s->s_name);
  for (i = 0; i < s->n_vars; i++)
  {
    dump_var (pfx, s->vars + i, idmap, fp);
  }

  for (i = 0; i < s->n_scopes; i++)
  {
    dump_scope (pfx, s->scopes + i, idmap, fp);
  }
  fprintf (fp, "$upscope $end\n");
  free (pfx);
}


void
vcdDumpHeader (struct vcd_hdr *s, DHash * idmap, FILE * fp)
{
  time_t t;
  size_t i;

  time (&t);
  fprintf (fp, "$date\n\t%s$end\n", ctime (&t));
  fprintf (fp, "$version\n\tvcdmerge\n$end\n");
  fprintf (fp, "$timescale\n\t%s\n$end\n", s->tscale);

  for (i = 0; i < s->base.n_vars; i++)
  {
    dump_var ("", s->base.vars + i, idmap, fp);
  }

  for (i = 0; i < s->base.n_scopes; i++)
  {
    dump_scope ("", s->base.scopes + i, idmap, fp);
  }
  fprintf (fp, "$enddefinitions $end\n");
}

#ifdef TEST
int
main (int argc, char *argv[])
{
  FILE *fp = NULL;
  FILE *fpc = NULL;
  struct vcd_hdr h = { 0 };
  struct symtab s = { 0 };
  fp = fopen ("./tgt.vcd", "rt");
  assert (fp);
  fpc = fopen ("./c.vcd", "wt");
  assert (fpc);
  vcdReadHeader (&h, fp);
  vcdInitValues (&s, &h);
  vcdDumpHeader (&h, &s.by_name, fpc);
  stClear (&s);
  vcdClearHeader (&h);
  fclose (fpc);
  fclose (fp);
  return 0;
}
#endif
