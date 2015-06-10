#include "hdr_prs.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct handler
{
  char const *name;
  int (*func) (void *d, FILE * fp);
};

static int process_tag (struct vcd_scope *hdr, FILE * fp);

static int
skip_to_end (FILE * fp)
{
  char *tag = NULL;
  assert (1 == fscanf (fp, "%ms", &tag));
  while (strcmp (tag, "$end"))
  {
    free (tag);
    assert (1 == fscanf (fp, "%ms", &tag));
  }
  free (tag);
  return 0;
}

static int
proc_date (void *d, FILE * fp)
{
  return skip_to_end (fp);
}

static int
proc_ver (void *d, FILE * fp)
{
  return skip_to_end (fp);
}

static int
proc_ts (void *d, FILE * fp)
{
  struct vcd_scope *sl = d;
  struct vcd_hdr *hdr;
  char *tag = NULL;
  assert (sl->prev == NULL);
  hdr = (struct vcd_hdr *) sl;
  assert (1 == fscanf (fp, "%ms", &tag));
  hdr->tscale = tag;
  return skip_to_end (fp);
}

#define SEEN_UPSCOPE 3
#define SEEN_ENDDEF 2
static int
proc_scope (void *d, FILE * fp)
{
  struct vcd_scope *scp = d;
  struct vcd_scope *newscope;
  char *stype, *sname;

  scp->n_scopes++;
  scp->scopes =
    realloc (scp->scopes, scp->n_scopes * sizeof (struct vcd_scope));
  assert (scp->scopes);
  newscope = scp->scopes + (scp->n_scopes - 1);
  memset (newscope, 0, sizeof (struct vcd_scope));

  assert (2 == fscanf (fp, "%ms %ms $end", &stype, &sname));
  newscope->s_type = stype;
  newscope->s_name = sname;
  newscope->prev = scp;
  do
  {
  }
  while (SEEN_UPSCOPE != process_tag (newscope, fp));
  return 0;
}

static int
proc_var (void *d, FILE * fp)
{
  struct vcd_scope *scp = d;
  char *svp = NULL;
  char *vtype, *vid, *vnm, *varr, *tmp, *wid_s;
  size_t n;
  int rv;
  struct vcd_var *newvar;

  scp->n_vars++;
  scp->vars = realloc (scp->vars, scp->n_vars * sizeof (struct vcd_var));
  assert (scp->vars);
  newvar = scp->vars + (scp->n_vars - 1);
  memset (newvar, 0, sizeof (struct vcd_var));

  tmp = NULL;
  n = 0;
  rv = getline (&tmp, &n, fp);
  assert (strcmp ("$end", tmp) && (rv >= 0));

  vtype = strtok_r (tmp, " \n", &svp);
  wid_s = strtok_r (NULL, " \n", &svp);
  vid = strtok_r (NULL, " \n", &svp);
  vnm = strtok_r (NULL, " \n", &svp);
  varr = strtok_r (NULL, " \n", &svp);
  if (!strncmp (varr, "$end", 4))
  {
    varr = NULL;
  }
  newvar->tp = strdup (vtype);
  newvar->width = strtoull (wid_s, NULL, 10);
  newvar->id = strdup (vid);
  newvar->prev = scp;
  newvar->name = strdup (vnm);
  if (varr)
    newvar->arr = strdup (varr);
  free (tmp);
  return 0;
}

static int
proc_upscope (void *d, FILE * fp)
{
  skip_to_end (fp);             //XXX gotta tokenize stuff...

  return SEEN_UPSCOPE;
}

static int
proc_end (void *d, FILE * fp)
{
  assert (0);                   //unhandled
}

static int
proc_defend (void *d, FILE * fp)
{
  skip_to_end (fp);
  return SEEN_ENDDEF;
}


static struct handler vcd_handlers[] = {
  {"$date", proc_date},
  {"$version", proc_ver},
  {"$timescale", proc_ts},
  {"$scope", proc_scope},
  {"$var", proc_var},
  {"$upscope", proc_upscope},
  {"$enddefinitions", proc_defend},
  {"$end", proc_end}
};

static int
process_tag (struct vcd_scope *hdr, FILE * fp)
{
  char *tag = NULL;
  size_t i;
  assert (1 == fscanf (fp, "%ms", &tag));
  for (i = 0; i < sizeof (vcd_handlers) / sizeof (vcd_handlers[0]); i++)
  {
    if (!strcmp (tag, vcd_handlers[i].name))
    {
      free (tag);
      tag = NULL;
      return vcd_handlers[i].func (hdr, fp);
    }
  }
  abort ();
}

/*
reads the header and
populates the scopes and variables
stops after reading the $enddefinitions $end tag
*/
int
vcdReadHeader (struct vcd_hdr *hdr, FILE * fp)
{
  memset (hdr, 0, sizeof (struct vcd_hdr));
  do
  {
  }
  while (SEEN_ENDDEF != process_tag (&hdr->base, fp));
  return 0;
}

static void
clear_var (struct vcd_var *v)
{
  v->prev = NULL;
  free (v->tp);
  v->tp = NULL;
  v->width = 0;
  free (v->id);
  v->id = NULL;
  free (v->name);
  v->name = NULL;
  free (v->arr);
  v->arr = NULL;
}

static void
clear_scope (struct vcd_scope *scp)
{
  size_t i;
  for (i = 0; i < scp->n_vars; i++)
  {
    clear_var (&scp->vars[i]);
  }
  free (scp->vars);
  scp->vars = NULL;
  scp->n_vars = 0;
  for (i = 0; i < scp->n_scopes; i++)
  {
    clear_scope (&scp->scopes[i]);
  }
  free (scp->scopes);
  scp->scopes = NULL;
  scp->n_scopes = 0;
  free (scp->s_type);
  scp->s_type = NULL;
  free (scp->s_name);
  scp->s_name = NULL;
  scp->prev = NULL;
}

void
vcdClearHeader (struct vcd_hdr *hdr)
{
  free (hdr->tscale);
  hdr->tscale = NULL;
  clear_scope (&hdr->base);
}

void
vcdParseVal (char *v, char **bitz_ret, char **id_ret)
{
  char *svp = NULL, *s1, *s2;
  v = strdup (v);
  assert (v);
  s1 = strtok_r (v, " \n", &svp);
  s2 = strtok_r (NULL, " \n", &svp);
  if (NULL == s2)
  {
    s2 = s1 + 1;
    s1 = strdup (s1);
    assert (s1);
    s1[1] = '\0';
  }
  else
  {
    assert ((s1[0] == 'b') || (s1[0] == 'B'));
    s1++;                       //XXX no real values..
    s1 = strdup (s1);
    assert (s1);
  }
  s2 = strdup (s2);
  assert (s2);
  *bitz_ret = s1;
  *id_ret = s2;
  free (v);
}

#ifdef TEST
int
main (int argc, char *argv[])
{
  FILE *fp = NULL;
  struct vcd_hdr h = { 0 };
  fp = fopen ("./tgt.vcd", "rt");
  vcdReadHeader (&h, fp);
  vcdClearHeader (&h);
  fclose (fp);
  return 0;
}
#endif
