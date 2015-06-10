#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "help.h"
/*
usage:
vcdmerge a b c

merge file a and b into output file c
a and b must exist. c must not exist. 
all three arguments must be present. 

*/

extern int vcdmerge (char const *fa, char const *fb, char const *fc,
                     int do_diff);
int
main (int argc, char *argv[])
{
  char const *fa = NULL, *fb = NULL, *fc = NULL;
  int do_diff = 0;
  int rv = 0;
  int c;
  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"diff", no_argument, 0, 'd'},
      {0, 0, 0, 0}
    };

    c = getopt_long (argc, argv, "dh", long_options, &option_index);
    if (c == -1)
      break;
    switch (c)
    {
    case 'h':
      fprintf (stderr, help_txt);
      return 0;
      break;
    case 'd':
      do_diff = 1;
      break;
    default:
      fprintf (stderr, "Unrecognized option: %c\n", c);
      fprintf (stderr, "Enter `vcdmerge -h' for help\n");
      rv = 1;
      break;
    }
    if (rv)
      break;
  }
  if (rv == 0)
  {
    if ((argc - optind) != 3)
    {
      fprintf (stderr, "error: Wrong nmber of files. Need exactly three.\n");
      fprintf (stderr, "Enter `vcdmerge -h' for help\n");
      rv = 1;
    }
    else
    {
      fa = argv[optind + 0];
      fb = argv[optind + 1];
      fc = argv[optind + 2];
      rv = vcdmerge (fa, fb, fc, do_diff);
    }
  }
  return rv;
}
