#include "mergetab.h"
#include <assert.h>
int
mtInit (struct mergetab *s)
{
  assert (s);
  dhashInit (&s->idmap[0], id2ids_hash, id2ids_equals);
  dhashInit (&s->idmap[1], id2ids_hash, id2ids_equals);
  return 0;
}

void
mtClear (struct mergetab *s)
{
  assert (s);
  dhashClear (&s->idmap[0], id2ids_destroy);
  dhashClear (&s->idmap[1], id2ids_destroy);
}

static int
log_equals (char a, char b)
{
  return (((a == 'x') && (b == 'X')) ||
          ((a == 'z') && (b == 'Z')) ||
          ((b == 'x') && (a == 'X')) ||
          ((b == 'z') && (a == 'Z')) || (a == b));
}

static void
do_merge (struct vcd_value *cp, int do_diff)
{
  struct vcd_value *ap = cp->a_val;
  struct vcd_value *bp = cp->b_val;
  size_t i;
  assert (cp->width == ap->width);
  assert (cp->width == bp->width);
  for (i = 0; i < cp->width; i++)
  {
    if (!do_diff)
    {
      if (log_equals (ap->bitz[i], bp->bitz[i]))
        cp->bitz[i] = ap->bitz[i];
      else
        cp->bitz[i] = 'x';
    }
    else
    {
      if (log_equals (ap->bitz[i], bp->bitz[i]))
        cp->bitz[i] = '0';
      else
        cp->bitz[i] = '1';
    }
  }
}

//from symtab.h...
uint32_t values_hash (void *d);
int values_equals (void *a, void *b);
//avs... put it somewhere else...
void slistFree (SList * l);

void
vcdMergeValues (SList * c_a, SList * c_b, DHash * cs, int do_diff)
{
/*
changes have been applied to a and b before
generate values for c here.
how 2 merge?
may have changes from one or both files...
add ptrs to the other values inside values?

*/
  struct vcd_val_e *es;
  dhashInit (cs, values_hash, values_equals);
  es = (struct vcd_val_e *) slistFirst (c_a);
  while (es)
  {
    SList *cl = &es->vp->c_val;
    struct vcd_val_e *ce = (struct vcd_val_e *) slistFirst (cl);
    while (ce)
    {
      dhashPut (cs, ce->vp);
      do_merge (ce->vp, do_diff);
      ce = (struct vcd_val_e *) slistENext (&ce->e);
    }
    es = (struct vcd_val_e *) slistENext (&es->e);
  }

  es = (struct vcd_val_e *) slistFirst (c_b);
  while (es)
  {
    SList *cl = &es->vp->c_val;
    struct vcd_val_e *ce = (struct vcd_val_e *) slistFirst (cl);
    while (ce)
    {
      dhashPut (cs, ce->vp);
      do_merge (ce->vp, do_diff);
      ce = (struct vcd_val_e *) slistENext (&ce->e);
    }
    es = (struct vcd_val_e *) slistENext (&es->e);
  }
  slistFree (c_a);
  slistFree (c_b);
}
