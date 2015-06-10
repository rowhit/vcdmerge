#include "hdr_cmp.h"
#include <string.h>

static int compare_scopes (struct vcd_scope *scp_a, struct vcd_scope *scp_b);
#define strcmp_null(sa_,sb_) ((sa_==NULL)&&(sb_==NULL))?0:strcmp(sa_,sb_)
#define scmp_s(nm_,memb_) if(0==rv)rv=strcmp_null(nm_##_a->memb_,nm_##_b->memb_)
#define scmp_i(nm_,memb_) if(0==rv)rv=(nm_##_a->memb_ < nm_##_b->memb_)?-1:(nm_##_a->memb_ != nm_##_b->memb_)

static int
compare_vars (struct vcd_var *v_a, struct vcd_var *v_b)
{
  int rv = 0;
  scmp_s (v, tp);
//      scmp_s(v,id);
  scmp_s (v, name);
  scmp_s (v, arr);
  scmp_i (v, width);
  return rv;
}

static struct vcd_var *
find_same_var (struct vcd_scope *scp, struct vcd_var *v_a)
{
  size_t i;
  for (i = 0; i < scp->n_vars; i++)
  {
    struct vcd_var *v_b;
    v_b = scp->vars + i;
    if (0 == compare_vars (v_a, v_b))
      return v_b;
  }
  return NULL;
}

static int
compare_nvars (struct vcd_scope *scp_a, struct vcd_scope *scp_b)
{
  int rv = 0;
  size_t i;

  scmp_i (scp, n_vars);
  if (rv)
    return rv;
  for (i = 0; i < scp_a->n_vars; i++)
  {
    struct vcd_var *va = scp_a->vars + i;
    struct vcd_var *vb = find_same_var (scp_b, va);
    if (vb == NULL)
      return 1;
  }
  return 0;
}

static struct vcd_scope *
find_same_scope (struct vcd_scope *scp, struct vcd_scope *cscp_a)
{
  size_t i;
  int rv = 0;
  for (i = 0; i < scp->n_scopes; i++)
  {
    struct vcd_scope *cscp_b;
    cscp_b = scp->scopes + i;
    rv = 0;
    scmp_s (cscp, s_type);
    scmp_s (cscp, s_name);
    if (!rv)
      return cscp_b;
  }
  return NULL;

}

static int
compare_nscopes (struct vcd_scope *scp_a, struct vcd_scope *scp_b)
{
  int rv = 0;
  size_t i;

  scmp_i (scp, n_scopes);
  for (i = 0; i < scp_a->n_scopes; i++)
  {
    struct vcd_scope *cscp_a = scp_a->scopes + i;
    struct vcd_scope *cscp_b = find_same_scope (scp_b, cscp_a);
    if (cscp_b == NULL)
      return 1;
    rv = compare_scopes (cscp_a, cscp_b);
    if (rv)
      return rv;
  }
  return 0;
}

static int
compare_scopes (struct vcd_scope *scp_a, struct vcd_scope *scp_b)
{
  int rv = 0;

  scmp_s (scp, s_type);
  scmp_s (scp, s_name);

  if (0 == rv)
  {
    rv = compare_nvars (scp_a, scp_b);
  }
  if (0 == rv)
  {
    rv = compare_nscopes (scp_a, scp_b);
  }
  return rv;
}

int
vcdCompareHeaders (struct vcd_hdr *hdr_a, struct vcd_hdr *hdr_b)
{
  int rv;
  rv = strcmp (hdr_a->tscale, hdr_b->tscale);
  if (0 == rv)
  {
    rv = compare_scopes (&hdr_a->base, &hdr_b->base);
  }
  return rv;
}

#ifdef TEST
int
main (int argc, char *argv[])
{
  FILE *fp = NULL;
  int rv;
  struct vcd_hdr h_a = { 0 }, h_b =
  {
  0};
  fp = fopen ("./bldc_min.vcd", "rt");
  vcdReadHeader (&h_a, fp);
  fclose (fp);
  fp = fopen ("./bldc_max.vcd", "rt");
  vcdReadHeader (&h_b, fp);
  fclose (fp);

  rv = vcdCompareHeaders (&h_a, &h_b);
  printf ("compare returned %d\n", rv);
  vcdClearHeader (&h_a);
  vcdClearHeader (&h_b);
  return 0;
}
#endif
