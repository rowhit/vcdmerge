#include "mergetab.h"
#include <assert.h>
int mtInit(struct mergetab *s)
{
	assert(s);
	dhashInit(&s->idmap[0],id2ids_hash,id2ids_equals);
	dhashInit(&s->idmap[1],id2ids_hash,id2ids_equals);
	return 0;
}

void mtClear(struct mergetab *s)
{
	assert(s);
	dhashClear(&s->idmap[0],id2ids_destroy);
	dhashClear(&s->idmap[1],id2ids_destroy);
}
