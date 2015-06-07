#include <string.h>
#include <stdio.h>
#include "dhash.h"

#define utlFree(p_) do{if((p_)!=NULL)free(p_);}while(0)
#define utlFAN(p_) do{utlFree(p_);p_=NULL;}while(0)

int
dhashInit (DHash * h, uint32_t (*hash) (void *d),
           int (*equals) (void *a, void *b))
{
  h->tbl = calloc (32, sizeof (SList));
  if (!h->tbl)
    return 1;

  h->hash = hash;
  h->equals = equals;
  h->tbl_size = 32;
  return 0;
}

///calls func() on every element. unordered.
void
dhashForEach (DHash * h, void *x, void (*func) (void *x, void *this))
{
  uint32_t i;
  for (i = 0; i < h->tbl_size; i++)
  {
    SList *l = &(h->tbl[i]);
    SListE *e;
    e = slistFirst (l);
    while (e)
    {
      func (x, *((void **) slistEPayload (e)));
      e = slistENext (e);
    }
  }
}


void
dhashClear (DHash * h, void (*destroy) (void *d))
{
  uint32_t i;
  uint32_t longest_cluster = 0;
  for (i = 0; i < h->tbl_size; i++)
  {
    SList *l = &(h->tbl[i]);
    SListE *e;
    uint32_t j;
    j = 0;
    e = slistRemoveFirst (l);
    while (e)
    {
      j++;
      destroy (*((void **) slistEPayload (e)));
      utlFAN (e);
      e = slistRemoveFirst (l);
    }
    if (j > longest_cluster)
    {
      longest_cluster = j;
    }
  }
  utlFAN (h->tbl);
  memset (h, 0, sizeof (DHash));
}

///find element
void *
dhashGet (DHash * h, void *ref)
{
  uint32_t hv;
  SList *l;
  SListE *e;
  hv = h->hash (ref);
  hv = hv % h->tbl_size;
  l = &(h->tbl[hv]);

  e = slistFirst (l);
  while (e)
  {
    void **r;
    void *p;
    r = slistEPayload (e);
    p = *r;
    if (!h->equals (p, ref))
      return p;
    e = slistENext (e);
  }
  return NULL;
}

static int
dhashResize (DHash * h, uint32_t new_size)
{
  SList *new_tbl;
  uint32_t i;
  new_tbl = calloc (new_size, sizeof (SList));
  if (!new_tbl)
    return 1;

  for (i = 0; i < h->tbl_size; i++)
  {
    SList *l = &(h->tbl[i]);
    SListE *e;
    e = slistRemoveFirst (l);
    while (e)
    {
      void **xp = slistEPayload (e);
      void *x = *xp;
      uint32_t idx;
      idx = h->hash (x) % new_size;
      slistInsertLast (&new_tbl[idx], e);
      e = slistRemoveFirst (l);
    }
  }
  utlFAN (h->tbl);
  h->tbl = new_tbl;
  h->tbl_size = new_size;
  return 0;
}

static int
dhashGrow (DHash * h)
{
  return dhashResize (h, h->tbl_size * 2);
}

static int
dhashShrink (DHash * h)
{
  return dhashResize (h, h->tbl_size / 2);
}

void *
dhashRemove (DHash * h, void *ref)
{
  uint32_t hv;
  SList *l;
  SListE *prev, *e;
  hv = h->hash (ref);
  hv = hv % h->tbl_size;
  l = &(h->tbl[hv]);

  prev = NULL;
  e = slistFirst (l);
  while (e)
  {
    void **r;
    void *rv;
    r = slistEPayload (e);
    rv = *r;
    if (!h->equals (rv, ref))
    {
      slistRemoveNext (l, prev);
      utlFAN (e);
      h->num_entries--;
      if ((h->num_entries < (h->tbl_size / 3)) && (h->tbl_size > 32))
        dhashShrink (h);
      return rv;
    }
    prev = e;
    e = slistENext (e);
  }
  return NULL;
}

///deposit an element in the map TODO: check if identical element already exists. Perhaps we should allow dup elements and create a get funtion to retrieve all of them
int
dhashPut (DHash * h, void *d)
{
  uint32_t hv;
  SList *l;
  SListE *e;
  void **x;
  if (dhashGet (h, d))
    return 1;
  hv = h->hash (d);
  hv = hv % h->tbl_size;
  l = &(h->tbl[hv]);
  e = calloc (1, slistESize (sizeof (void *)));
  if (!e)
    return 1;
  x = slistEPayload (e);
  *x = d;
  slistInsertLast (l, e);
  h->num_entries++;
  if ((h->num_entries > (h->tbl_size * 2 / 3)) && dhashGrow (h))
    return 1;
  return 0;
}
