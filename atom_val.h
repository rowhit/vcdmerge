#ifndef __atom_val_h
#define __atom_val_h

/*
when reading changesets, 
I need to find the full names+values from the ids
the full names are needed to assign the signal in the output,
because I can not expect the ids of a and b file(and c) 
to match.
This also means the the output file will have exactly 1 id
per variable name, as I can not easily determine which ones I 
could alias without reading everything twice...
Or perhaps I can, by generating (smaller) file c id groups which are completely 
contained by both as and bs ids
a:| 0 | 1  |
c:|0|1|2|3 |
b:|0| 1 | 2|
still a, and b may trigger writes 
to multiple values, but there will be fewer
outputs in file c than if its all separate...
how to find the atomic value sets?
grab all the names for id i in file a,
then get all the ids for those names in file b.
generate equiv-ids for file c, repeat for the other
ids in a.
there's one symtab for each input file.
*/
void avsGenerate(struct symtab * out, struct symtab * a, struct symtab * b);

#endif

