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

static int is_log_val(char c)
{
	return (c=='0'||
	c=='1'||
	c=='x'||
	c=='z'||
	c=='X'||
	c=='Z');
}


static void set_val(char *v,struct symtab * st)
{
	char *s1=NULL,*s2=NULL;
	struct vcd_value*val;
	size_t i;
	vcdParseVal(v,&s1,&s2);
	val=stGetValue(st,s2);
	assert(val);
//	assert(strlen(s1)==val->width); doesn't match...
	for(i=0;i<strlen(s1);i++)
	{
		assert(is_log_val(s1[strlen(s1)-i-1]));
		val->bitz[i]=s1[strlen(s1)-i-1];
	}
	free(s1);
	free(s2);
}

static void parse_inits(struct symtab *vals,struct vcd_hdr *h,FILE *fp)
{
	char * tmp;
	size_t n;
	int rv;
	fscanf(fp,"%ms",&tmp);
	assert(tmp);
	if(tmp[0]=='#')
	{
		errno=0;
		h->start_time=strtoul(&tmp[1],NULL,10);
		assert(0==errno);
		free(tmp);
		fscanf(fp,"%ms",&tmp);
		assert(tmp);
	}
	assert(!strcmp(tmp,"$dumpvars"));
	free(tmp);
	tmp=NULL;
	n=0;
	rv=getline(&tmp,&n,fp);
	rv=getline(&tmp,&n,fp);
	while(strncmp("$end",tmp,4)&&(rv>=0))
	{
		set_val(tmp,vals);
		rv=getline(&tmp,&n,fp);
	}
	free(tmp);
}


static void for_each_variable(char * pfx,struct vcd_scope *hdr_a,
void * d,
void (*func)(char * scopes,struct vcd_var *v,void *d))
{
	size_t i;
	pfx=strdup(pfx);
	assert(pfx);
	if(hdr_a->s_name)
	{
		pfx=realloc(pfx,strlen(pfx)+strlen(hdr_a->s_name)+2);
		assert(pfx);
		strcat(pfx,".");
		strcat(pfx,hdr_a->s_name);
	}
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


static void generate_value(char * scopes,struct vcd_var *v,void *d)
{
	struct symtab *st=d;
	struct vcd_value *vl=vcdValueNew(v);
	char * fullname;
	assert(vl);
	if(stPutValue(st,vl))
	{//failed.. already there..
		vcdValueDel(vl);
	}
	fullname=calloc(strlen(scopes)+2+strlen(v->name),1);
	strcpy(fullname,scopes);
	strcat(fullname,".");
	strcat(fullname,v->name);
	stAddNameForId(st,v->id,fullname,v);
	free(fullname);
}

void vcdInitValues(struct symtab * s,struct vcd_hdr *h,FILE *fp)
{
	stInit(s);
	for_each_variable("",&h->base,s,generate_value);
	parse_inits(s,h,fp);
}

static char const *resolve_id(DHash *idmap,char* fullname)
{
	struct id2id ref={fullname,NULL};
	struct id2id *rv=dhashGet(idmap,&ref);
	if(NULL==rv)
		return NULL;
	return rv->id2;
}

static void dump_var(char * scopes,struct vcd_var *v,DHash * idmap,FILE *fp)
{
	char * fullname=NULL;
	fullname=calloc(strlen(scopes)+2+strlen(v->name),1);
	strcpy(fullname,scopes);
	strcat(fullname,".");
	strcat(fullname,v->name);
	fprintf(fp,"$var %s %u %s %s ",v->tp,v->width,resolve_id(idmap,fullname),v->name);
	if(NULL!=v->arr)
	{
		fprintf(fp,"%s $end\n",v->arr);
	}
	else
	{
		fprintf(fp,"$end\n");
	}
	free(fullname);
}


static void dump_scope(char * pfx,struct vcd_scope *s,DHash *idmap,FILE *fp)
{
	size_t i;
	pfx=strdup(pfx);
	assert(pfx);
	if(s->s_name)
	{
		pfx=realloc(pfx,strlen(pfx)+strlen(s->s_name)+2);
		assert(pfx);
		strcat(pfx,".");
		strcat(pfx,s->s_name);
	}
	
	fprintf(fp,"$scope %s %s $end\n",s->s_type,s->s_name);
	for(i=0;i<s->n_vars;i++)
	{
		dump_var(pfx,s->vars+i,idmap,fp);
	}
	
	for(i=0;i<s->n_scopes;i++)
	{
		dump_scope(pfx,s->scopes+i,idmap,fp);
	}
	fprintf(fp,"$upscope $end\n");
	free(pfx);
}


void vcdDumpHeader(struct vcd_hdr *s,DHash *idmap,FILE *fp)
{
	time_t t;
	size_t i;
	
	time(&t);
	fprintf(fp,"$date\n\t%s$end\n",ctime(&t));
	fprintf(fp,"$version\n\tvcdmerge\n$end\n");
	fprintf(fp,"$timescale\n\t%s\n$end\n",s->tscale);
	
	for(i=0;i<s->base.n_vars;i++)
	{
		dump_var("",s->base.vars+i,idmap,fp);
	}
	
	for(i=0;i<s->base.n_scopes;i++)
	{
		dump_scope("",s->base.scopes+i,idmap,fp);
	}
}

#ifdef TEST
int main(int argc, char * argv[])
{
	FILE * fp=NULL;
	FILE *fpc=NULL;
	struct vcd_hdr h={0};
	struct symtab s={0};
	fp=fopen("./tgt.vcd","rt");
	assert(fp);
	fpc=fopen("./c.vcd","wt");
	assert(fpc);
	vcdReadHeader(&h,fp);
	vcdInitValues(&s,&h,fp);
	vcdDumpHeader(&h,&s.by_name,fpc);
	stClear(&s);
	vcdClearHeader(&h);
	fclose(fpc);
	fclose(fp);
	return 0;
}
#endif

