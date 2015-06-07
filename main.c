#include <stdio.h>
#include <string.h>

/*
usage:
vcdmerge a b c

merge file a and b into output file c
a and b must exist. c must not exist. 
all three arguments must be present. 

*/
static char const * usage="usage:\n\
vcdmerge a b c\n\
\n\
merge file a and b into output file c\n\
a and b must exist. c must not exist. \n\
all three arguments must be present. \n\
";

extern int vcdmerge(char const * fa,char const * fb,char const * fc);
int main(int argc,char * argv[])
{
	char const *fa=NULL,*fb=NULL,*fc=NULL;
	int rv=0;
	if(argc!=4)
	{
		fprintf(stderr,"error: Not enough parameters on commandline. Need exactly three.\n");
		fprintf(stderr,usage);
		rv=1;
	}
	else
	{
		fa=argv[1];
		fb=argv[2];
		fc=argv[3];
		rv=vcdmerge(fa,fb,fc);
	}
	return rv;
}

