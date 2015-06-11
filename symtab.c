#include "symtab.h"
#include <string.h>
#include <assert.h>

SList *
stGetNamesForId (struct symtab *s, char *id)
{
  struct id2ids ref;
  struct id2ids *rv;
  memset (&ref, 0, sizeof (ref));
  ref.id1 = id;
  rv = dhashGet (&s->by_id, &ref);
  if (rv)
    return &rv->l;
  return NULL;
}

char *
stGetIdForName (struct symtab *s, char *name)
{
  struct id2id ref;
  struct id2id *rv = dhashGet (&s->by_name, &ref);
  memset (&ref, 0, sizeof (ref));
  ref.id1 = name;
  if (NULL == rv)
    return NULL;
  return rv->id2;
}

void
stAddNameForId (struct symtab *s, char *id, char *name, struct vcd_var *var)
{
  struct id2id *a;
  struct id2ids ref;
  struct id2ids *rv;
  memset (&ref, 0, sizeof (ref));
  ref.id1 = id;

  a = calloc (sizeof (struct id2id), 1);
  assert (a);
  a->id1 = strdup (name);
  a->id2 = strdup (id);
  dhashPut (&s->by_name, a);

  rv = dhashGet (&s->by_id, &ref);
  if (NULL == rv)
  {
    rv = calloc (sizeof (struct id2ids), 1);
    rv->id1 = strdup (id);
    slistInit (&rv->l);
    dhashPut (&s->by_id, rv);
  }

  {
    struct e_str *s;
    s = calloc (strlen (name) + 1 + sizeof (struct e_str), 1);
    assert (s);
    strcpy (s->s, name);
    s->var = var;
    slistInsertLast (&rv->l, &s->e);
  }
}

int
stPutValue (struct symtab *s, struct vcd_value *v)
{
  return dhashPut (&s->values, v);
}

struct vcd_value *
stGetValue (struct symtab *s, char *id)
{
  struct vcd_value *v;
  struct vcd_value ref;
  memset (&ref, 0, sizeof (ref));
  ref.id = id;
  v = dhashGet (&s->values, &ref);
  return v;
}

struct vcd_value *
stRmvValue (struct symtab *s, char *id)
{
  struct vcd_value *v;
  struct vcd_value ref;
  memset (&ref, 0, sizeof (ref));
  ref.id = id;
  v = dhashRemove (&s->values, &ref);
  return v;
}

#define frob(x_,i_) (((x_)*5)<<((i_)&15))
uint32_t
id2id_hash (void *d)
{
  struct id2id *i_a = d;
  char *p = i_a->id1;
  uint32_t rv;
  int i = 0;
  rv = 0;
  while (*p)
  {
    rv += frob (*p, i);
    i++;
    p++;
  }
  return rv;
}

int
id2id_equals (void *a, void *b)
{
  struct id2id *i_a = a, *i_b = b;
  return strcmp (i_a->id1, i_b->id1);
}

uint32_t
id2ids_hash (void *d)
{
  struct id2ids *i_a = d;
  char *p = i_a->id1;
  uint32_t rv;
  int i = 0;
  rv = 0;
  while (*p)
  {
    rv += frob (*p, i);
    i++;
    p++;
  }
  return rv;
}

int
id2ids_equals (void *a, void *b)
{
  struct id2ids *i_a = a, *i_b = b;
  return strcmp (i_a->id1, i_b->id1);
}

uint32_t
values_hash (void *d)
{
  struct vcd_value *i_a = d;
  char *p = i_a->id;
  uint32_t rv;
  int i = 0;
  rv = 0;
  while (*p)
  {
    rv += frob (*p, i);
    i++;
    p++;
  }
  return rv;

}

int
values_equals (void *a, void *b)
{
  struct vcd_value *i_a = a, *i_b = b;
  return strcmp (i_a->id, i_b->id);
}

int
stInit (struct symtab *s)
{
  assert (s);
  dhashInit (&s->by_name, id2id_hash, id2id_equals);
  dhashInit (&s->by_id, id2ids_hash, id2ids_equals);
  dhashInit (&s->values, values_hash, values_equals);
  return 0;
}

void
id2id_destroy (void *d)
{
  struct id2id *i_a = d;
  free (i_a->id1);
  i_a->id1 = NULL;
  free (i_a->id2);
  i_a->id2 = NULL;
  free (i_a);
}

void
idse_destroy (void *d NOTUSED, SListE * e)
{
  free (e);
}

void
id2ids_destroy (void *d)
{
  struct id2ids *i_a = d;
  free (i_a->id1);
  i_a->id1 = NULL;
  slistForEach (&i_a->l, NULL, idse_destroy);
  slistInit (&i_a->l);
  free (i_a);
}

struct vcd_value *
vcdValueNew (struct vcd_var *v, char init_val)
{
  struct vcd_value *rv;
  rv = calloc (1, sizeof (struct vcd_value) + v->width + 1);
  rv->id = strdup (v->id);
  rv->width = v->width;
  memset (rv->bitz, init_val, v->width * sizeof (char));
  rv->bitz[v->width] = '\0';
  return rv;
}

void
vcdValueDel (struct vcd_value *v)
{
  free (v->id);
  v->id = NULL;
  free (v);
}

static void
values_destroy (void *d)
{
  struct vcd_value *i_a = d;
  vcdValueDel (i_a);
}

void
stClear (struct symtab *s)
{
  assert (s);
  dhashClear (&s->by_name, id2id_destroy);
  dhashClear (&s->by_id, id2ids_destroy);
  dhashClear (&s->values, values_destroy);
}

struct e_str *
estrClone (struct e_str *e)
{
  struct e_str *s;
  s = calloc (strlen (e->s) + 1 + sizeof (struct e_str), 1);
  assert (s);
  strcpy (s->s, e->s);
  s->var = e->var;
  return s;
}

int
estrEqual (struct e_str *a, struct e_str *b)
{
  return (0 == strcmp (a->s, b->s));
}
