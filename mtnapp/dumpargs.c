/*

      Dump a set of argument result buffers

*/

#include <stdio.h>
#include "adslib.h"

#define printf  ads_printf

/*  ADS result group code definitions.  */

#define GX_CHUNK    1004              /* Binary chunk type */
#define XDT_LONG    1071              /* Long integer */

/*  DXF_PGROUP  --  Print a DXF group in a result buffer.  */

static void dxf_pgroup(rp)
  struct resbuf *rp;
{
    ads_printf("%4d:  ", rp->restype);

    switch (rp->restype) {

        case RTENAME:                 /* Entity names */
        case RTPICKS:                 /* Selection sets */
            ads_printf("Entity name: %lX\n", rp->resval.rlname[0]);
            break;

        case RTSTR:                   /* Strings */
            ads_printf("\"%s\"\n", rp->resval.rstring);
            break;

        case RTPOINT:
        case RT3DPOINT:
            ads_printf("(%g, %g, %g)\n",
                rp->resval.rpoint[X], rp->resval.rpoint[Y],
                rp->resval.rpoint[Z]);
            break;

        case RTREAL:                  /* Real numbers */
        case RTANG:                   /* Angles */
        case RTORINT:
            ads_printf("%g\n", rp->resval.rreal);
            break;

        case RTSHORT:             /* Integers */
            ads_printf("%d\n", rp->resval.rint);
            break;

        case RTLONG:              /* Long integers */
            ads_printf("%ld\n", rp->resval.rlong);
            break;
    }
}

/*  DUMPARGS  --  Print a result item chain.  */

void dumpargs(rp)
  struct resbuf *rp;
{
    while (rp != NULL) {
        dxf_pgroup(rp);
        rp = rp->rbnext;
    }
}
