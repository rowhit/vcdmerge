#ifndef __val_init_h
#define __val_init_h

#include "symtab.h"
#include "hdr_prs.h"

void vcdInitValues (struct symtab *s, struct vcd_hdr *h);
void vcdDumpHeader (struct vcd_hdr *s, DHash * idmap, FILE * fp);
int vcdParseChange (SList * rvl, struct symtab *vals, struct vcd_hdr *h,
                    FILE * fp);
int vcdParseInits (SList * rvl, struct symtab *vals, struct vcd_hdr *h,
                   FILE * fp);

#endif
