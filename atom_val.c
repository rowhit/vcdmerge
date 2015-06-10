#include "symtab.h"
#include "atom_val.h"
#include "val_init.h"
#include "hdr_cmp.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct id_parm
{
  struct symtab *a;
  struct symtab *b;
  struct symtab *c;
  struct mergetab *m;
  int *ctr;
  char *nm;
  int do_diff;
};

char *
gen_id (int *ctr_p)
{
  int i = *ctr_p;
  int j = 0;
  char *rv = calloc (10, sizeof (char));
  do
  {
    assert (j < 9);
    rv[j] = (i % 94) + 33;
    i = (i / 94);
    j++;
  }
  while (i);

  assert (j < 10);
  rv[j] = '\0';
  *ctr_p = (*ctr_p) + 1;
  return rv;
}

struct e_str *
gen_sl (char *s)
{
  struct e_str *rv;
  rv = calloc (1, sizeof (struct e_str) + strlen (s) + 1);
  strcpy (rv->s, s);
  return rv;
}

static void
add_id2 (DHash * h, char *nm, char *cid)
{
  struct id2ids *p = NULL;
  struct id2ids ref = { 0 };
  ref.id1 = nm;
  p = dhashGet (h, &ref);
  if (!p)
  {
    p = calloc (1, sizeof (struct id2ids));
    p->id1 = strdup (nm);
    dhashPut (h, p);
  }
  slistInsertLast (&p->l, &gen_sl (cid)->e);
}

void
add_id (void *d, char *idb)
{
  struct id_parm *v = d;
  char *cid = gen_id (v->ctr);
  struct vcd_value *vl;
  struct e_str *es;
  struct vcd_val_e *ve;
  SList *nl;

  add_id2 (&v->m->idmap[0], v->nm, cid);
  add_id2 (&v->m->idmap[1], idb, cid);

  nl = stGetNamesForId (v->a, v->nm);
  es = (struct e_str *) slistFirst (nl);
  vl = vcdValueNew (es->var, v->do_diff ? '0' : 'x');
  free (vl->id);
  vl->id = cid;

  vl->a_val = stGetValue (v->a, v->nm);
  ve = calloc (sizeof (struct vcd_val_e), 1);
  ve->vp = vl;
  slistInsertLast (&vl->a_val->c_val, &ve->e);

  vl->b_val = stGetValue (v->b, idb);
  ve = calloc (sizeof (struct vcd_val_e), 1);
  ve->vp = vl;
  slistInsertLast (&vl->b_val->c_val, &ve->e);

  stPutValue (v->c, vl);
}

struct fnparm
{
  void (*func) (void *d, char *idb);
  void *x;
};

void
fnf (void *x, SListE * e)
{
  struct fnparm *p = x;
  struct e_str *s = (struct e_str *) e;
  p->func (p->x, s->s);
}

static void
for_each_name (SList ids, void *v, void (*func) (void *d, char *idb))
{
  struct fnparm p = { func, v };
  slistForEach (&ids, &p, fnf);
}

void
attach_ids (char *id, SList ids, struct id_parm *v)
{
  v->nm = id;
  for_each_name (ids, v, add_id);
}

static SList
intersect_names (SList a, SList b)
{
  SList rv = { 0 };
  struct e_str *a_n;
  a_n = (struct e_str *) slistFirst (&a);
  while (a_n)
  {
    struct e_str *b_n;
    b_n = (struct e_str *) slistFirst (&b);
    while (b_n)
    {
      if (estrEqual (a_n, b_n))
      {
        struct e_str *r_n;
        r_n = estrClone (a_n);
        slistInsertLast (&rv, &r_n->e);
      }
      b_n = (struct e_str *) slistENext (&b_n->e);
    }
    a_n = (struct e_str *) slistENext (&a_n->e);
  }
  return rv;
}

static void
insert_uniq (SList * l, struct e_str *r)
{
  struct e_str *s = (struct e_str *) slistFirst (l);
  while (s)
  {
    if (0 == strcmp (r->s, s->s))
    {
      free (r);
      return;
    }
    s = (struct e_str *) slistENext (&s->e);
  }
  slistInsertLast (l, &r->e);
}

static SList
ids_for_names (SList names, struct symtab *t)
{
  SList rv = { 0 };
  struct e_str *s = (struct e_str *) slistFirst (&names);
  struct e_str *r;
  while (s)
  {
    char *id;
    id = stGetIdForName (t, s->s);
    r = calloc (strlen (id) + 1 + sizeof (struct e_str), 1);
    assert (r);
    strcpy (r->s, id);
    insert_uniq (&rv, r);
    s = (struct e_str *) slistENext (&s->e);
  }
  return rv;
}

static void
just_free (void *x, SListE * e)
{
  free (e);
}

void
slistFree (SList * l)
{
  slistForEach (l, NULL, just_free);
  slistInit (l);
}

static SList
xid_to_cid (SList ids, struct id_parm *v, int idx)
{
  SList rv = { 0 };
  struct e_str *i;
  i = (struct e_str *) slistFirst (&ids);
  while (i)
  {
    struct id2ids *p = NULL;
    struct id2ids ref = { 0 };
    struct e_str *j;
    ref.id1 = i->s;
    p = dhashGet (&v->m->idmap[idx], &ref);
    assert (p);
    j = (struct e_str *) slistFirst (&p->l);
    while (j)
    {
      slistInsertLast (&rv, &estrClone (j)->e);
      j = (struct e_str *) slistENext (&j->e);
    }
    i = (struct e_str *) slistENext (&i->e);
  }
  slistFree (&ids);
  return rv;
}

static void
attach_names2 (char *id, SList cn, struct symtab *t)
{
  struct e_str *ne;
  ne = (struct e_str *) slistFirst (&cn);
  while (ne)
  {
    stAddNameForId (t, id, ne->s, ne->var);
    ne = (struct e_str *) slistENext (&ne->e);
  }
}

void
attach_names (void *d, char *idb)
{
  struct id_parm *v = d;
  SList an;
  SList bn;
  SList cn;
  SList aid;
  SList bid;
  SList cid;
  char *ida = v->nm;
  an = *stGetNamesForId (v->a, ida);
  bn = *stGetNamesForId (v->b, idb);
  cn = intersect_names (an, bn);
  //this holds the names that correspond to the id

  aid = ids_for_names (cn, v->a);
  bid = ids_for_names (cn, v->b);
  //the ids for the names

  //need to get the c ids
  aid = xid_to_cid (aid, v, 0);
  bid = xid_to_cid (bid, v, 1);

  cid = intersect_names (aid, bid);
  assert (slistCount (&cid) == 1);

  slistFree (&aid);
  slistFree (&bid);

  /*
     by here, cid should be a 1-element id,
     that maps to the cn names..
   */
  attach_names2 (((struct e_str *) slistFirst (&cid))->s, cn, v->c);
  slistFree (&cid);
  slistFree (&cn);
}


void
gen_ids (char *id, struct id_parm *v)
{
  SList names;
  SList ids;
  names = *stGetNamesForId (v->a, id);
  ids = ids_for_names (names, v->b);
/*
a:| 0 | 1  |
c:|0|1|2|3 | <-generates these
b:|0| 1 | 2|
*/
  attach_ids (id, ids, v);
  /*
     I still need the name allocation for c...
     need 2 enumerate the intersections...
   */
  v->nm = id;
  for_each_name (ids, v, attach_names);
  slistFree (&ids);
}

struct idfparm
{
  void (*func) (char *id, struct id_parm * v);
  void *x;
};

void
idfunc (void *x, void *this)
{
  struct idfparm *p = x;
  struct vcd_value *val = this;
  p->func (val->id, p->x);
}

static void
for_all_ids (struct symtab *a,
             void (*func) (char *id, struct id_parm * v), struct id_parm *v)
{
  struct idfparm p = { func, v };
  dhashForEach (&a->values, &p, idfunc);
}

/*

how to find the atomic value sets?
grab all the names for id i in file a,
then get all the ids for those names in file b.
generate equiv-ids for file c, repeat for the other
ids in a.

*/
void
avsGenerate (struct symtab *a, struct symtab *b,
             struct symtab *c_out, int do_diff)
{
  int ctr = 0;
  struct mergetab m;
  mtInit (&m);
  struct id_parm v = { a, b, c_out, &m, &ctr, NULL, do_diff };
  stInit (c_out);
  for_all_ids (a, gen_ids, &v);
  mtClear (&m);
}

#ifdef TEST
int
main (int argc, char *argv[])
{
  FILE *fpa, *fpb, *fpc;
  struct vcd_hdr ha, hb;
  struct symtab sa, sb, sc;
  fpa = fopen ("./bldc_min.vcd", "rt");
  assert (fpa);
  fpb = fopen ("./bldc_max.vcd", "rt");
  assert (fpb);
  fpc = fopen ("./c2.vcd", "wt");
  assert (fpc);

  vcdReadHeader (&ha, fpa);
  vcdReadHeader (&hb, fpb);

  assert (0 == vcdCompareHeaders (&ha, &hb));   //same timescales, same scopes, vars

  vcdInitValues (&sa, &ha);
  vcdInitValues (&sb, &hb);

  avsGenerate (&sa, &sb, &sc, 0);
  vcdDumpHeader (&ha, &sc.by_name, fpc);

  stClear (&sa);
  stClear (&sb);
  stClear (&sc);

  vcdClearHeader (&ha);
  vcdClearHeader (&hb);

  fclose (fpa);
  fclose (fpb);
  fclose (fpc);
  return 0;
}
#endif
