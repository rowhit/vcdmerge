#ifndef __mergetab_h
#define __mergetab_h

/*
there's one of those for the output file
on every change populated with values that need dumping..
(full names, not ids)
need separate ids for output. 
*/
struct mergetab
{
	unsigned now;///<current simulation timestamp
	DHash changeset;///<cleared on every change and filled with values that changed, and may need dumping..
	DHash idmap[2];///<map an id from a sourcefile  to multiple ids in merged file.
};

void get_merged_ids(struct mergetab *m, struct symtab *s, char *id);

#endif

