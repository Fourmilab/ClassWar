/*


                             A T L A D S

             Atlas primitives for accessing ADS services.

      Designed and implemented in March of 1990 by John Walker.

*/

#include "class.h"

#define printf  ads_printf

/*  ADS result group code definitions.  */

#define GX_CHUNK    1004              /* Binary chunk type */
#define XDT_LONG    1071              /* Long integer */

/*  Local variables  */

static struct resbuf *rbuf = NULL;    /* Result buffer pointer */

/*  RESITEM  --  Search a result buffer chain and return an item
                 with the specified group code.  Special gimmick:
                 if the group code is -(10000 + N), the Nth item
                 in the result buffer chain is returned (or NULL
                 if there aren't that many items). */

struct resbuf *resitem(rchain, gcode)
  struct resbuf *rchain;
  int gcode;
{
    if (gcode <= -10000) {
        gcode = -10000 - gcode;
        while ((rchain != NULL) && (gcode-- > 0))
           rchain = rchain->rbnext;
    } else {
        while ((rchain != NULL) && (rchain->restype != gcode))
           rchain = rchain->rbnext;
    }

    return rchain;
}

/*  DXF_PGROUP  --  Print a DXF group in a result buffer.  */

static void dxf_pgroup(rp)
  struct resbuf *rp;
{
    int rtype;

    ads_printf("%4d:  ", rp->restype);
    if (rp->restype < 0)
        rtype = rp->restype == -3 ? -3 : -1;
    else
        rtype = (rp->restype % 100) / 10;

    switch (rtype) {
        case -3:                      /* Application data delimiters */
            ads_printf("Application data:\n");
            break;

        case -1:                      /* Entity names */
            ads_printf("Entity name: %lX\n", rp->resval.rlname[0]);
            break;

        case 0:                       /* Strings */
            if (rp->restype == GX_CHUNK) {
                int i;

                ads_printf("%d byte binary chunk:\n",
                    rp->resval.rbinary.clen);
                ads_printf("       ");
                for (i = 0; i < rp->resval.rbinary.clen; i++) {
                    ads_printf("%02X", rp->resval.rbinary.buf[i] & 0xFF);
                }
                ads_printf("\n");
            } else {
                ads_printf("\"%s\"\n", rp->resval.rstring);
            }
            break;

        case 1:                       /* X co-ordinates */
        case 2:                       /* Y co-ordinates */
        case 3:                       /* Z co-ordinates */
        case 4:                       /* Real numbers */
        case 5:                       /* Angles */
            ads_printf("%g\n", rp->resval.rreal);
            break;

        case 6:                   /* Integers */
        case 7:                   /* More integers */
            if (rp->restype == XDT_LONG) {
                ads_printf("%ld\n", rp->resval.rlong);
            } else {
                ads_printf("%d\n", rp->resval.rint);
            }
            break;
    }
}

/*  DXF_PITEM  --  Print a result item chain.  */

/***** static ****/ void dxf_pitem(rp)
  struct resbuf *rp;
{
    while (rp != NULL) {
        int rclass = (rp->restype % 100) / 10;

        if (rp->restype > 0 && rclass >= 1 && rclass <= 3 &&
            (rp->restype != 38 && rp->restype != 39)) {
             ads_printf("%4d:  (%g, %g, %g)\n", rp->restype,
                 rp->resval.rpoint[X], rp->resval.rpoint[Y],
                 rp->resval.rpoint[Z]);
        } else {
            dxf_pgroup(rp);
        }
        rp = rp->rbnext;
    }
}

/*  ADS access primitives.  */

prim P_entnext()
{                                     /* name/0 resname -- status */
    Sl(2);
    Hpc(S0);
    if ((stackitem *) S1 != NULL) {
        Hpc(S1);
    }
    S1 = ads_entnext((long *) S1, (long *) S0);
    Pop;
}

prim P_entlast()
{                                     /* name -- status */
    Sl(1);
    Hpc(S0);
    S0 = ads_entlast((long *) S0);
}

prim P_entget()
{                                     /* name --  */
    Sl(1);
    Hpc(S0);
    if (rbuf != NULL)
        V ads_relrb(rbuf);
    rbuf = ads_entget((long *) S0);
    Pop;
}

prim P_entmake()
{                                     /*  -- status  */
    if (rbuf != NULL) {
        So(1);
        Push = ads_entmake(rbuf);
        ads_relrb(rbuf);
        rbuf = NULL;
    }
}

prim P_entmod()
{                                     /*  -- status */
    if (rbuf != NULL) {
        So(1);
        Push = ads_entmod(rbuf);
        ads_relrb(rbuf);
        rbuf = NULL;
    }
}

/*  Result buffer chain manipulation primitives.  */

/*  CLEARITEM  --  Release current item.  Since the item is normally
                   automatically cleared when written, this is normally
                   needed only when clearing out an item before
                   replacing it with a totally different item.  */

prim clearitem()
{                                     /*  --  */
    V ads_relrb(rbuf);
    rbuf = NULL;
}

/*  ADDGROUP  --  Tack an item of the specified type on to the end
                  of the result buffer chain.  The item is cleared
                  to zero, thus setting its pointer fields to NULL. */

prim addgroup()
{                                     /*  gtype  --  */
    struct resbuf *rb;

    Sl(1);
    rb = gets(resbuf);
    /* This NULLs rb->rbnext as well */
    V memset((char *) rb, 0, sizeof(struct resbuf));
    rb->restype = S0;
    Pop;
    if (rbuf == NULL)
        rbuf = rb;
    else {
        struct resbuf *rs = rbuf;

        while (rs->rbnext != NULL) {
            rs = rs->rbnext;
        }
        rs->rbnext = rb;
    }
}

/*  DELGROUP  --  Delete named group, if present in the result buffer
                  chain.  Does nothing if the specified group doesn't
                  appear in the chain (use GROUP to test, if you care).  */

prim delgroup()
{                                     /* group -- */
    struct resbuf *rbt;
    int rcode;

    Sl(1);
    rcode = S0;
    Pop;

    rbt = resitem(rbuf, rcode);
    if (rbt != NULL) {
        struct resbuf *rb = rbuf, *rbl = NULL;
        while ((rb != NULL) && (rb != rbt)) {
            rbl = rb;
            rb = rb->rbnext;
        }
        if (rb != NULL) {
            if (rbl == NULL) {
                rbuf = rb->rbnext;
            } else {
                rbl->rbnext = rb->rbnext;
            }
            rb->rbnext = NULL;
            V ads_relrb(rb);
        }
    }
}

/*  GROUP  --  Push group from current result buffer chain onto the
               stack.  The format of the data pushed onto the stack
               depends upon the class of the group code requested.
               The value of the group is left on the top of the stack
               in the format in which the requested data type is
               expressed.  Note that string and chunk data types are
               copied to temporary string buffers, not passed directly
               as pointers.  The group is assumed to exist and an error
               message issued if it isn't found.  Use GROUP? to check
               whether an optional group is present before calling
               GROUP.  */

prim group()
{
    int rcode;
    struct resbuf *rb;

    Sl(1);
    rcode = S0;
    Pop;

    if ((rb = resitem(rbuf, rcode)) == NULL) {
V printf("\nGROUP: no %d group in chain.\n", rcode);
    } else {
        int i;

        switch ((rb->restype % 100) / 10) {
            case 0:                   /* Strings */
                if (rb->restype == GX_CHUNK) {
                    So(2);
                    V memcpy(strbuf[cstrbuf], rb->resval.rbinary.buf,
                        rb->resval.rbinary.clen);
                    Push = (stackitem) strbuf[cstrbuf];
                    Push = rb->resval.rbinary.clen;
                } else {
                    So(1);
                    V strcpy(strbuf[cstrbuf], rb->resval.rstring);
                    Push = (stackitem) strbuf[cstrbuf];
                }
                cstrbuf = (cstrbuf + 1) % ((int) atl_ntempstr);
                break;

            case 1:                   /* Co-ordinates */
                So(Realsize * 3);
                for (i = X; i <= Z; i++) {
                    stk += Realsize;
                    SREAL0(rb->resval.rpoint[i]);
                }
                break;

            case 4:                   /* Real numbers */
            case 5:                   /* Angles */
                So(Realsize);
                stk += Realsize;
                SREAL0(rb->resval.rreal);
                break;

            case 6:                   /* Integer indices */
            case 7:                   /* More indices */
                So(1);
                Push = (rb->restype == XDT_LONG) ? rb->resval.rlong :
                            rb->resval.rint;
                break;
        }
    }
}

/*  GROUPQ  --  Quick Boolean test for presence of group.  */

prim groupq()
{                                     /* gcode -- present? */
    Sl(1);
    S0 = (resitem(rbuf, (int) S0) != NULL) ? -1 : 0;
}

/*  GROUPCOUNT  --  Return number of groups in current item.  */

prim groupcount()
{                                     /*  -- count */
    struct resbuf *rb = rbuf;
    int n = 0;

    So(1);
    while (rb != NULL) {
        rb = rb->rbnext;
        n++;
    }
    Push = n;
}

/*  SETGROUP  --  Set group to values on stack.  */

prim setgroup()
{                                     /* gvalues gcode --  */
    int rcode;
    struct resbuf *rb;

    Sl(1);
    rcode = S0;
    Pop;

    if ((rb = resitem(rbuf, rcode)) == NULL) {
        V printf("\nSETGROUP: %d group not found.\n", rcode);
    } else {
        switch ((rb->restype % 100) / 10) {
            case 0:                   /* Strings */
                if (rb->restype == GX_CHUNK) {

                    Sl(2);
                    S0 &= 0x7FFFFFFF; /* Make sure negative lengths bounce */
                    Hpc(S1);          /* Check both the address... */
                    Hpc(S1 + S0);     /* ...and the length! */
                    if (rb->resval.rbinary.buf != NULL)
                        free(rb->resval.rbinary.buf);
                    rb->resval.rbinary.buf = alloc((unsigned int)
                        (rb->resval.rbinary.clen = S0));
                    V memcpy(rb->resval.rbinary.buf, (char *) S1,
                        rb->resval.rbinary.clen);
                    Pop2;
                } else {
                    Sl(1);
                    Hpc(S0);
                    if (rb->resval.rstring != NULL)
                        free(rb->resval.rstring);
                    rb->resval.rstring = strsave((char *) S0);
                    Pop;
                }
                break;

            case 1:                   /* Co-ordinates */
                Sl(Realsize * 3);
                rb->resval.rpoint[X] = REAL2;
                rb->resval.rpoint[Y] = REAL1;
                rb->resval.rpoint[Z] = REAL0;
                Npop(Realsize * 3);
                break;

            case 4:                   /* Real numbers */
            case 5:                   /* Angles */
                Sl(Realsize);
                rb->resval.rreal = REAL0;
                Realpop;
                break;

            case 6:                   /* Integer indices */
            case 7:                   /* More indices */
                Sl(1);
                if (rb->restype == XDT_LONG)
                    rb->resval.rlong = S0;
                else
                    rb->resval.rint = S0;
                Pop;
                break;

            default:
V printf("\nDuh....\n");
                break;
        }
    }
}

/*  PRINTGROUP  --  Print group.  */

prim printgroup()                     /* gcode -- */
{
    struct resbuf *rb;

    Sl(1);
    if ((rb = resitem(rbuf, (int) S0)) == NULL) {
        V ads_printf("No %ld group in chain.\n", S0);
    } else {
        struct resbuf *rnext = rb->rbnext;

        /* All this hocus-pocus with the RBNEXT field is because we
           want to print the group with dxf_pitem(), which edits the
           group as an ADS group, display all co-ordinates of a point
           if the group code is in the 10-19 range, rather than as a
           split-out DXF group, where a 10-19 code would just be
           edited as a real number as would be done were we to call
           dxf_pgroup().  Since dxf_pitem() normally prints the entire
           chain, we have to temporarily break the link so only the
           requested item is printed. */
        rb->rbnext = NULL;
        dxf_pitem(rb);
        rb->rbnext = rnext;
    }
    Pop;
}

/*  PRINTITEM  --  Print entire current item.  */

prim printitem()                      /*  --  */
{
    dxf_pitem(rbuf);
}

/*  ATLADS_INIT  --  Define primitives accessible from the ATLAS program. */

void atlads_init()
{
    static struct primfcn primt[] = {

        /* ADS interface primitives. */

        {"0ADS_ENTNEXT", P_entnext},
        {"0ADS_ENTLAST", P_entlast},
        {"0ADS_ENTGET",  P_entget},
        {"0ADS_ENTMAKE", P_entmake},
        {"0ADS_ENTMOD",  P_entmod},

        /* DXF and result buffer chain primitives. */

        {"0ADDGROUP",    addgroup},
        {"0CLEARITEM",   clearitem},
        {"0DELGROUP",    delgroup},
        {"0GROUP",       group},
        {"0GROUP?",      groupq},
        {"0GROUPCOUNT",  groupcount},
        {"0PRINTGROUP",  printgroup},
        {"0PRINTITEM",   printitem},
        {"0SETGROUP",    setgroup},

        {NULL,           (codeptr) 0}
    };
    atl_primdef(primt);
}
