#ifndef __val_init_h
#define __val_init_h
#include "symtab.h"
#include "hdr_prs.h"
void vcdInitValues(struct symtab * s,struct vcd_hdr *h,FILE *fp);
void vcdDumpHeader(struct vcd_hdr *s,DHash *idmap,FILE *fp);

#endif

