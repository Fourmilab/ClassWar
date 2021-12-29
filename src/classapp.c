/*

        Class application interface

        This  module  is  included in ADS applications which implement
        methods declared  as  "external"  in  their  .cls  files.   It
        handles communication across the ads_invoke() link, converting
        the individual data in result buffers into the fields  in  the
        variable and argument structures.

        Designed and implemented in April of 1990 by John Walker.

*/

#include <stdio.h>
#include "adslib.h"

#define E_string         0
#define E_pointer        5
#define E_triple        10
#define E_position      11
#define E_displacement  12
#define E_direction     13
#define E_real          40
#define E_distance      41
#define E_scalefactor   42
#define E_integer       71
#define E_angle        140
#define E_orientation  240
#define E_corner       111
#define E_keyword      100

typedef struct {
    int p_type;                       /* Field type */
    char *p_field;                    /* Field offset in structure */
} m_Protocol;

typedef struct {
    char *methname;                   /* Method name */
    void (*methfunc)();               /* Method-implementing function */
} method_Item;

typedef struct {
    int kwflag;                       /* Nonzero if keyword entered */
    union {
        char *kwtext;                 /* Keyword text if keyword entered */
        ads_real kwreal;              /* Otherwise, the value. */
        ads_point kwpoint;
        long kwlong;
    } value;
} kwarg;

#define True        1
#define False       0

static struct resbuf *rbchain, *rbscan; /* Current result buffer chain */
static m_Protocol *mvars;             /* Current variable protocol */

/*  STRSAVE  --  Allocate a duplicate of a string.  */

static char *strsave(s)
  char *s;
{
        char *c = malloc((unsigned) (strlen(s) + 1));

        if (c == NULL)
           ads_abort("Out of memory");
        strcpy(c, s);
        return c;
}

/*  STOREVAR  --  Store the value in the next result buffer into the
                  corresponding field in the structure.  */

static int storevar(rb, fdesc)
  struct resbuf *rb;
  m_Protocol *fdesc;
{
    ads_real *pt;
    int ftype = fdesc->p_type;
    char *fvalue = fdesc->p_field;

    if (rb == NULL)
        return False;

    /* If the field type is negative, this is an argument for which
       keywords may be optionally specified (note that string arguments
       do not permit keyword specification).  For a keyword-optional
       argument, we examine the type of the result buffer.  If it's
       a string, a keyword was entered and we set the keyword present
       field in the KWARG structure and point the KWTEXT field in the
       union to the keyword in the result item.  Otherwise, we clear
       the keyword entered flag and store the value in the appropriate
       value field of the union. */

    if (ftype < 0) {
        kwarg *k = (kwarg *) fdesc->p_field;
        int basetype;

        if ((k->kwflag = rb->restype == RTSTR) != 0) {
            k->value.kwtext = rb->resval.rstring;
            return True;
        }
        ftype = -ftype;
        basetype = ftype % 100;
        if (basetype >= E_triple && basetype < E_real) {
            fvalue = (char *) k->value.kwpoint;
        } else if (basetype == E_integer) {
            fvalue = (char *) &(k->value.kwlong);
        } else {
            fvalue = (char *) &(k->value.kwreal);
        }
    }

    switch (ftype) {
        case E_string:
        case E_keyword:
        case E_pointer:
            if (rb->restype != RTSTR)
                return False;
            strcpy(fvalue, rb->resval.rstring);
            break;

        case E_triple:
        case E_position:
        case E_displacement:
        case E_direction:
        case E_corner:
            if (rb->restype != RT3DPOINT)
                return False;
            pt = (ads_real *) fvalue;
            pt[X] = rb->resval.rpoint[X];
            pt[Y] = rb->resval.rpoint[Y];
            pt[Z] = rb->resval.rpoint[Z];
            break;

        case E_real:
        case E_distance:
        case E_scalefactor:
        case E_angle:
        case E_orientation:
            if (rb->restype != RTREAL)
                return False;
            *((ads_real *) fvalue) = rb->resval.rreal;
            break;

        case E_integer:
            /* No, this isn't a typo.  To allow full 32 bit integers
               to be passed, we pass the integer variable as a real. */
            if (rb->restype != RTREAL)
                return False;
            *((long *) fvalue) = rb->resval.rreal;
            break;

        default:
            return False;
    }
    return True;
}

/*  FETCH_PROTOCOL  --  Obtain all the variables described in a
                        protocol structure.  */

static int fetch_protocol(pd)
  m_Protocol *pd;
{
    while (pd->p_type != 0) {
        if (!storevar(rbscan, pd))
            return False;
        pd++;
        rbscan = rbscan->rbnext;
    }
    return True;
}

/*  BEG_METHOD  --  Receive the arguments from a method and store
                    them into the instance and class variable buffers
                    and the argument structure (if any).  */

int beg_method(vars, args)
  m_Protocol *vars, *args;
{
    rbscan = rbchain = ads_getargs();
    mvars = vars;                     /* Remember variable protocol */

    if (!fetch_protocol(vars))
        return RTERROR;

    if (args != NULL) {
        if (!fetch_protocol(args)) {
            return RTERROR;
        }
    }

    return RTNORM;
}

/*  UPDATEVAR  --  Update the value in the next result buffer into the
                   corresponding field in the structure.  */

static int updatevar(rb, fdesc)
  struct resbuf *rb;
  m_Protocol *fdesc;
{
    ads_real *pt;

    if (rb == NULL)
        return False;

    switch (fdesc->p_type) {
        case E_string:
        case E_keyword:
        case E_pointer:
            if (rb->restype != RTSTR)
                return False;
            free(rb->resval.rstring);
            rb->resval.rstring = strsave(fdesc->p_field);
            break;

        case E_triple:
        case E_position:
        case E_displacement:
        case E_direction:
        case E_corner:
            if (rb->restype != RT3DPOINT)
                return False;
            pt = (ads_real *) fdesc->p_field;
            rb->resval.rpoint[X] = pt[X];
            rb->resval.rpoint[Y] = pt[Y];
            rb->resval.rpoint[Z] = pt[Z];
            break;

        case E_real:
        case E_distance:
        case E_scalefactor:
        case E_angle:
        case E_orientation:
            if (rb->restype != RTREAL)
                return False;
            rb->resval.rreal = *((ads_real *) fdesc->p_field);
            break;

        case E_integer:
            /* No, this isn't a typo.  To allow full 32 bit integers
               to be passed, we pass the integer variable as a real. */
            if (rb->restype != RTREAL)
                return False;
            rb->resval.rreal = *((long *) fdesc->p_field) + 0.1;
            break;

        default:
            return False;
    }
    return True;
}

/*  UPDATE_PROTOCOL  --  Update all the variables described in a
                         protocol structure.  */

static struct resbuf *rbprev;         /* End of variable result buffers */

static int update_protocol(pd)
  m_Protocol *pd;
{
    while (pd->p_type != 0) {
        if (!updatevar(rbscan, pd))
            return False;
        pd++;
        rbprev = rbscan;
        rbscan = rbscan->rbnext;
    }
    return True;
}

/*  END_METHOD  --  Store the updated instance and class variables into
                    the argument list and return it across the link.  */

int end_method()
{
    rbscan = rbchain;
    rbprev = NULL;

    if (!update_protocol(mvars)) {
        ads_retnil();
        return RTERROR;
    }

    if (rbprev == NULL) {
        return ads_retnil();
    }

    ads_relrb(rbscan);                /* Release tail of result chain */
    rbprev->rbnext = NULL;            /* Lop off end of chain */
    return ads_retlist(rbchain);      /* Return arguments we received */
}

/*  DEFINE_CLASS  --  Registers the methods in a class method table
                      with ADS, rendering them accessible as external
                      methods.  */

void define_class(mt)
  method_Item *mt;
{
    static int defun_index = 1;

    while (mt->methname != NULL) {
        ads_defun(mt->methname, defun_index);
        ads_regfunc((int (*)()) mt->methfunc, defun_index);
        defun_index++;
        mt++;
    }
}

/*  MAIN_METHOD  --  Main program to manage the entire ADS interface
                     for a method-implementing program.  This is simply
                     called with the argc and argv passed to the user's
                     main program, plus the method definition table
                     for the class.  Programs which support more than
                     one class or wish a different interface with ADS
                     needn't use this function; it's just provided as
                     a convenience for the simplest and most common
                     form of external methods.  */

void main_method(argc, argv, mt)
  int argc;
  char *argv[];
  method_Item *mt;
{
    int stat, scode = RSRSLT;

    ads_init(argc, argv);
    while (True) {
       if ((stat = ads_link(scode)) < 0) {
           printf("\nBad status from ads_link() = %d\n", stat);
           exit(2);
       }

       scode = -RSRSLT;        /* Default return code */

       switch (stat) {

          case RQTERM:         /* Terminate.  Clean up and exit. */
             exit(0);

          case RQXLOAD:        /* Load functions.  */
             define_class(mt);
             break;

          default:
             break;
       }
    }
}
