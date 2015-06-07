#include "symtab.h"
#include "atom_val.h"

struct id_param
{
	struct mergetab * m;
	struct symtab * a;
	struct symtab * b;
	int *ctr;
	char * nm;
};


struct s_link
{
	SListE e;
	char s[];
};

char * gen_id(int * ctr_p)
{
	int i=*ctr_p;
	int j=0;
	char * rv=calloc(10,sizeof(char));
	while(i)
	{
		assert(j<9);
		rv[j]=(i%94)+33;
		i=(i/94);
		j++;
	}
	assert(j<10);
	rv[j]='\0';
	*ctr_p=(*ctr_p)+1;
	return rv;
}

struct s_link *gen_sl(char *s)
{
	struct s_link * rv;
	rv=calloc(1,sizeof(s_link)+strlen(s)+1);
	strcpy(rv->s,s);
	return rv;
}

static void add_id2(DHash *h,char * nm,char * cid)
{
	struct id2id *p=NULL;
	struct id2id ref={0};
	ref.id1=nm;
	p=dhashGet(h,&ref);
	if(!p)
	{
		p=calloc(1,sizeof(struct id2id));
		p->id1=strdup(nm);
		slistInsertLast(&p->l,gen_sl(cid));
		dhashPut(h,p);
	}
	else
	{
		slistInsertLast(&p->l,gen_sl(cid));
	}
}

void add_id(void * d,char *idb)
{
	struct id_parm *v=d;
	char *cid=gen_id(v->ctr);
	
	add_id2(&v->m->cmap[0],v->nm,cid);
	add_id2(&v->m->cmap[1],idb,cid);
	free(cid);
}

struct fnparm
{
	void (*func)(void * d,char *idb);
	void * x;
}

void fnf (void *x, SListE * e)
{
	struct fnparm *p=x;
	struct e_str *s=e;
	p->func(p->x,s->s);
}

static void for_each_name(SList ids,void *v,void (*func)(void * d,char *idb))
{
	struct fnparm p={func,v};
	slistForEach(&ids,&p,fnf);
}

void attach_ids(char *id,SList ids,struct id_parm *v)
{
	v->nm=id;
	for_each_name(ids,v,add_id);
}


void attach_names(void *d,char *idb)
{
	struct id_parm *v=d;
	SList an;
	SList bn;
	SList cn;
	SList aid;
	SList bid;
	SList cid;
	char *ida=v->nm;
	an=stGetNamesForId(v->a,ida);
	bn=stGetNamesForId(v->b,idb);
	cn=intersect_names(an,bn);
	aid=get_ids_for_names(cn,v->a);
	bid=get_ids_for_names(cn,v->b);
	cid=intersect_names(aid,bid);
	assert(slistCount(cid)==1);
	attach_names2(cid->s,cn,v->b);
	
}

static SList ids_for_names(SList names,struct symtab *s)
{
	SList rv={0};
	struct e_str *s=slistFirst(&names);
	struct e_str *r;
	while(s)
	{
		char * id;
		id=stGetIdForName(s,s->s);
		r=calloc(strlen(id)+1+sizeof(struct e_str),1);
		assert(r);
		strcpy(r->s,id);
		slistInsertLast(&rv,r);
		slistENext(&s->e);
	}
	return rv;
}

void gen_ids(char * id,struct id_parm * v)
{
	SList names;
	SList ids;
	names=stGetNamesForId(id,v->a);
	ids=ids_for_names(names,v->b);
/*
a:| 0 | 1  |
c:|0|1|2|3 | <-generates these
b:|0| 1 | 2|
*/
	attach_ids(id,ids,v);
	/*
	I still need the name allocation for c...
	need 2 enumerate the intersections...
	*/
	v->nm=id;
	for_each_name(ids,v,attach_names);
}

struct idfparm
{
	void (*func)(char * id,struct id_parm * v);
	void *x;
};

void idfunc (void *x, void *this)
{
	struct idfparm *p=x;
	struct vcd_value *val=this;
	p->func(val->id,p->x);
}

static void for_all_ids(struct symtab * a,
	void (*func)(char * id,struct id_parm * v),
	struct id_parm *v)
{
	struct idfparm p={func,v};
	dhashForEach(a->values,&p,idfunc);
}
/*

how to find the atomic value sets?
grab all the names for id i in file a,
then get all the ids for those names in file b.
generate equiv-ids for file c, repeat for the other
ids in a.

*/
void avsGenerate(struct mergetab * out, struct symtab * a, struct symtab * b)
{
	int ctr=1;//has to start at 1, else empty string...
	struct id_parm v=
	{out,a,b,&ctr};
	for_all_ids(a,gen_ids,&v);
}

