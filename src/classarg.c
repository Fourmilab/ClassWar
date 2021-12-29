/*

    Command method argument handling functions

*/

#include "class.h"

/*  Forward functions  */

int arg_obtain();

/*  (ARGWIPE)  --  Discard any pending, unprocessed arguments on
                   the argument declaration list.  */

void P_argwipe()
{                                     /*  --  */
    while (!qempty(&argq)) {
        free(qremove(&argq));
    }
}

/*  (ARGDEF)  --  Append an argument to the pending argument list
                  being assembled for the current method.  */

void P_argdef()
{                                     /* type prompt default basepoint
                                         getmodes keywords --  */
    argqitem *ai;

    Sl(6);
#ifndef NOMEMCHECK
#define Opc(x) if (((stackitem *) (x)) != NULL) { Hpc(x); }
    Opc(S0);
    Opc(S2);
    Opc(S3);
    Opc(S4);
#undef Opc
#endif
    ai = geta(argqitem);
    ai->aqi.argtype = S5;             /* Argument type */
    /* Prompt string */
    ai->aqi.argprompt = S4 != 0 ? strsave((char *) S4) : NULL;
    ai->aqi.argdefault = (char *) S3;/* Default value pointer */
    ai->aqi.argbase = (char *) S2;    /* Base point */
    ai->aqi.argmodes = S1;            /* Acquisition modes */
    /* Keyword list */
    ai->aqi.argkw = S0 != 0 ? strsave((char *) S0) : NULL;
    Npop(6);
    qinsert(&argq, &ai->argql);       /* Enqueue the argument */
}

/*  (ARG_GET)  --  This is called when an argument request appears within
                   the body of a method (as opposed to within the static
                   argument list.  It immediately obtains the argument
                   described by the top of the argument queue, places
                   the value on the stack, and discards the argument
                   from the queue.  The argument is left on the stack,
                   with the acquisition status on the top of the stack.
                   This status is 0 for a normal argument, 1 if a keyword
                   was entered, and -1 if the user canceled the input.
                   Note that string, point, and keyword arguments are returned
                   in a temporary string buffer and must be copied before
                   the next argument acquisition. */

void P_arg_get()
{                                     /*  -- <arg> status */
    argqitem *ai = (argqitem *) qremove(&argq);

    if (ai != NULL) {
        char *argb = strbuf[atl_ntempstr - 1]; /* Use last temp string */
        int stat, arglen;
        stackitem *argptr;

        stat = arg_obtain(&ai->aqi, &argb, &argptr, &arglen);
        if (stat != -1) {
            assert(arglen != 0);
            So(arglen);
            while (arglen > 0) {
                Push = *argptr++;
                arglen--;
            }
        }
        /* If the status was normal and the argument specified a keyword,
           the status will already have been placed on the stack by
           arg_obtain().  Since we always want to leave a status, we
           only store the status in case of error or if there was no
           keyword declaration. */

        if ((stat == -1) || (ai->aqi.argkw == NULL)) {
            So(1);
            Push = stat;              /* Leave status on top of stack */
        }
        free(ai);
    }
}

/*  ARGSAME  --  Given two argument items, determine if they are
                 equivalent.  Identity among argument items is the
                 key factor in condensing argument requests across
                 disparate classes in a selection set. */

static Boolean astrsame(s1, s2)       /* Test strings the same */
  char *s1, *s2;
{
    return (s1 == NULL && s2 == NULL) ||
           ((s1 != NULL && s2 != NULL) && (strcmp(s1, s2) == 0));
}

static Boolean aptsame(p1, p2)        /* Test points the same */
  ads_point p1, p2;
{
    return (p1 == NULL && p2 == NULL) ||
            ((p1 != NULL && p2 != NULL) &&
                (memcmp(p1, p2, sizeof(ads_point)) == 0));
}

static int aptlength(t)               /* Get length of controlled variable */
  int t;
{
    int len = -1;

    switch (t % 100) {
        case E_string:
        case E_pointer:
            len = 0;
            break;

        case E_triple:
        case E_position:
        case E_displacement:
        case E_direction:
            len = 3 * sizeof(ads_real);
            break;

        case E_real:
        case E_scalefactor:
        case E_distance:
            len = sizeof(ads_real);
            break;

        case E_integer:
            len = sizeof(long);
            break;
    }
    assert(len >= 0);
    return len;
}

static Boolean argsame(a1, a2)
  argitem *a1, *a2;
{
    int l;

    return ((a1->argtype == a2->argtype) &&
            (a1->argmodes == a2->argmodes) &&
            astrsame(a1->argprompt, a2->argprompt) &&
            astrsame(a1->argkw, a2->argkw) &&
            aptsame((ads_real *) a1->argbase, (ads_real *) a2->argbase) &&
            ((l = aptlength(a1->argtype)) == 0 ?
                (strcmp(a1->argdefault, a2->argdefault) == 0) :
                (memcmp(a1->argdefault, a2->argdefault, l) == 0)));
}

/*  ARGHHH  --  Obtain arguments for a command.  All arguments needed
                by the methods activated by sending this command message
                to all classes listed on the argument queue are collected.
                Returns True if the arguments were obtained OK and False if
                the user canceled the command during argument acquisition. */

Boolean arghhh(csq, totalargs)
  struct queue *csq;
  int totalargs;
{
    int classn = qlength(csq);        /* Classes in selection set */
    classum *cs = (classum *) csq;    /* Pointer to scan classum table */
    typedef struct aap {
        clarg *ar_argdesc;            /* Class-specific argument descriptor */
        argitem *ar_aitem;            /* Argument request item */
        struct aap *ar_master;        /* Master entity if this was
                                         subordinated to another identical
                                         argument */
        int ar_clsat;                 /* Number of classes satisfied by this
                                         argument */
        vitem *ar_vitem[1];           /* List of vitems for the classes
                                         satisfied by this argument. */
    } argapp;
    char *at;                         /* Argument assembly table */
    int argn = 0, argl;               /* Indices to scan assembly table */
    Boolean argsok = True;            /* Argument acquisition status */

    /* Size of ARGAPP table entries for this processing sequence. */
    int argappl = sizeof(argapp) + ((classn - 1) * sizeof(vitem *));

    if (classn == 0 || totalargs == 0) /* If no methods or no arguments */
        return True;                  /* Bail out immediately */

    assert(classn > 0);

#ifdef ARGDEBUG
    ads_printf("%d total args, %d classes in selection set:\n",
        totalargs, classn);
#endif

    at = alloc((totalargs + 1) * argappl);   /* Get temp item at end */
#define ati(n)  ((argapp *) (at + (argappl * (n))))

    /* Now that the preliminaries are out of the way, we scan the
       list of unique classes referenced in the selection set and
       fill the argument assembly table by scanning the argument
       request tables in the virtual method table selected by
       each class. */

    while ((cs = (classum *) qnext(cs, csq)) != NULL) {
        argl = argn;                  /* Save last argument table end */
#ifdef ARGDEBUG
        ads_printf("    %s%s\n", cs->clasci->classname,
            cs->clasvi == NULL ? " (No method)" : "");
#endif
        if (cs->clasvi != NULL) {
            int i;

            for (i = 0; i < cs->clasvi->vi_nargs; i++) {
                int j;
                argitem *ar = cs->clasvi->vi_arglist + i;
                argapp *aa = ati(argn);

#ifdef ARGDEBUG
                ads_printf("       %2d %s\n", ar->argtype, ar->argprompt);
#endif
                assert(argn < totalargs);
                aa->ar_argdesc = &(cs->clasarg[i]);
                aa->ar_aitem = ar;
                aa->ar_master = NULL;
                aa->ar_clsat = 1;
                aa->ar_vitem[0] = cs->clasvi;

                /* Now that we've defined this argument acquisition
                   request, check the arguments declared by earlier
                   classes to see if any is identical to this argument
                   request.  If so, subordinate this request to the
                   earlier one and add this class to its table of
                   classes satisfied. */

                for (j = 0; j < argl; j++) {
                    if (argsame(aa->ar_aitem, ati(j)->ar_aitem)) {
#ifdef ARGDEBUG
                        ads_printf("            Subordinated to item %d\n",
                                    j);
#endif
                        aa->ar_master = ati(j);  /* Set master link */
                        /* Mark the master prompt as satisfying this
                           class's requirement as well. */
                        ati(j)->ar_vitem[ati(j)->ar_clsat++] =
                            cs->clasvi;
                    }
                }
                argn++;
            }
        }
    }
    assert(argn == totalargs);        /* All arguments processed ? */

#ifdef ARGDEBUG
    {   int i;

        ads_printf("    Before Sort\n");
        for (i = 0; i < totalargs; i++) {
            argapp *ar = ati(i);

            ads_printf("%d:  %2d  %2d %s", i, ar->ar_aitem->argtype,
                ar->ar_clsat, ar->ar_aitem->argprompt);
            if (ar->ar_master != NULL) {
                ads_printf("   Sub[%d]\n", ar->ar_master - ati(0));
            } else {
                ads_printf("\n");
            }
        }
    }
#endif

    /* With the argument assembly table filled, next we sort it so
       that more general argument requests (those satisfying the needs
       of more classes in the selection set) are promoted to the start
       of the table and those of greater specificity move to the end.
       Except for arguments moved by the sorting process, original
       order is preserved. */

    {   int i, j;

        for (i = 0; i < totalargs - 1; i++) {
            for (j = i + 1; j < totalargs; j++) {
                argapp *a1 = ati(i), *a2 = ati(j);

                if (a1->ar_clsat < a2->ar_clsat) {
                    argapp *ax = ati(totalargs);

                    memcpy(ax, a1, argappl);
                    memcpy(a1, a2, argappl);
                    memcpy(a2, ax, argappl);
                }
            }
        }
    }

#ifdef ARGDEBUG
    {   int i;

        ads_printf("\n    After Sort\n");
        for (i = 0; i < totalargs; i++) {
            argapp *ar = ati(i);

            ads_printf("%d:  %2d  %2d %s", i, ar->ar_aitem->argtype,
                ar->ar_clsat, ar->ar_aitem->argprompt);
            if (ar->ar_master != NULL) {
                ads_printf("   Sub[%d]\n", ar->ar_master - ati(0));
            } else {
                ads_printf("\n");
            }
        }
    }
#endif

    /* The argument table has now been ordered from the most general
       to the most specific, with any argument requests common to all
       objects in the selection having percolated to the head of the
       table.  Now actually make the argument requests, informing
       the user of the types of objects each request applies to, if
       it's not common to all objects in the set. */

    {   char *argb = strbuf[0];       /* Use string buffer to assemble args */

        for (argn = 0; argn < totalargs; argn++) {
            argapp *ar = ati(argn);

            if (ar->ar_master == NULL) {
                int stat;

                if ((argn > 0) && (ar->ar_clsat < classn)) {
                    Boolean reprompt = True;

                    if (ar->ar_clsat == ati(argn - 1)->ar_clsat) {
                        int j;

                        for (j = 0; j < ar->ar_clsat; j++) {
                            if (ar->ar_vitem[j] !=
                                ati(argn - 1)->ar_vitem[j]) {
                                reprompt = False;
                                break;
                            }
                        }
                    }
                    if (reprompt) {
                        int j;

                        ads_printf("\nFor ");
                        for (j = 0; j < ar->ar_clsat; j++) {
                            ads_printf("%s%s ",
                                ar->ar_vitem[j]->vi_classitem->classname,
                                (j < (ar->ar_clsat - 1)) ? "," : "");
                        }
                        ads_printf("objects...\n");
                    }
                }
                ar->ar_argdesc->ca_source = (stackitem *) argb;
                stat = arg_obtain(ar->ar_aitem, &argb,
                    &(ar->ar_argdesc->ca_source),
                    &(ar->ar_argdesc->ca_nitems));

                /* If user canceled the argument acquisition, abort
                   the entire process of collecting arguments and
                   return a False status to our caller. */

                if (stat == -1) {
                    argsok = False;
                    break;
                }
            } else {
                /* Argument was subordinated to another acquisition.
                   Just copy the length and location of that acquisition
                   to the specific argument pointer for this class. */
                memcpy(ar->ar_argdesc, ar->ar_master->ar_argdesc,
                    sizeof(clarg));
            }
#ifdef ARGDEBUG
            ads_printf("Push %d words starting at byte %d\n",
                ar->ar_argdesc->ca_nitems,
                (char *) ar->ar_argdesc->ca_source - strbuf[0]);
#endif
        }
    }

    free(at);                         /* Release argument assembly buffer */
#undef ati
    return argsok;                    /* Indicate if arguments obtained OK. */
}

/*  ARG_OBTAIN  --  Execute an argument acquisition request.  Returns
                    a status:

                            0:  Normal input received
                            1:  Keyword entered; returned as string result
                           -1:  Input canceled

                    The value is stored starting at the address in the
                    pointer whose address is passed as ARG_AREA.  That
                    pointer is advanced to the start of the next argument,
                    guaranteeing byte alignment to real number boundaries. */

static int arg_obtain(ai, arg_area, pushwhat, pushnum)
  argitem *ai;
  char **arg_area;
  stackitem **pushwhat;               /* Return what to push */
  int *pushnum;                       /* Return how many stackitems to push */
{
#define ARG_cronly  0x1000            /* Carriage return delimited input */
    int gstat, glen,
        cronly = (ai->argmodes & ARG_cronly) ? 1 : 0,
/*      igmodes = (ai->argmodes & ~(ARG_cronly | 1)) |
                  ((ai->argdefault != NULL) ? 0 : 1),   */
        bargtype = ai->argtype % 100; /* Primitive argument type */
    char *arg_buffer = *arg_area;     /* Argument start address */
    char prompt[100], *ps = NULL;
#undef ARG_cronly

    if (ai->argprompt != NULL) {
        ps = prompt;
        strcpy(prompt, ai->argprompt);
        if (ai->argdefault && (ai->argtype != E_pointer)) {
            char defstr[50];
            ads_real *rp;

            switch (ai->argtype) {
                case E_string:
                case E_keyword:
                    sprintf(defstr, "<%s>", ai->argdefault);
                    break;

                case E_triple:
                case E_position:
                case E_displacement:
                case E_direction:
                case E_corner:
                    rp = (ads_real *) ai->argdefault;
                    strcpy(defstr, "<");
                    ads_rtos(*rp++, -1, -1, defstr + 1);
                    strcat(defstr, ",");
                    ads_rtos(*rp++, -1, -1, defstr + strlen(defstr));
                    strcat(defstr, ",");
                    ads_rtos(*rp, -1, -1, defstr + strlen(defstr));
                    strcat(defstr, ">");
                    break;

                case E_real:
                case E_scalefactor:
                case E_distance:
                    rp = (ads_real *) ai->argdefault;
                    strcpy(defstr, "<");
                    ads_rtos(*rp, -1, -1, defstr + 1);
                    strcat(defstr, ">");
                    break;

                case E_angle:
                case E_orientation:
                    rp = (ads_real *) ai->argdefault;
                    strcpy(defstr, "<");
                    ads_angtos(*rp, -1, -1, defstr + 1);
                    strcat(defstr, ">");
                    break;

                case E_integer:
                    sprintf(defstr, "<%ld>", *((int *) ai->argdefault));
                    break;
            }
            strcat(prompt, " ");
            strcat(prompt, defstr);
        }
        strcat(prompt, ": ");
    }
    switch (ai->argtype) {
        ads_name ename;
        ads_point pt;

        case E_string:
            *arg_buffer = EOS;
            gstat = ads_getstring(cronly, ps, arg_buffer);
            glen = (gstat == RTNORM) ? (strlen(arg_buffer) + 1) : 2;
            if (ai->argdefault != NULL && strlen(arg_buffer) == 0)
                gstat = RTNONE;
            break;

        case E_pointer:
            gstat = ads_entsel(ps, ename, pt);
            *arg_buffer = EOS;
            glen = 2;
            if (gstat == RTNORM) {
                struct resbuf *rb;

                rb = ads_entget(ename);
                if (rb != NULL) {
                    struct resbuf *ri = resitem(rb, 5);

                    if (ri != NULL) {
                        strcpy(arg_buffer, ri->resval.rstring);
                        glen = strlen(arg_buffer) + 1;
                    } else {
                        gstat = RTNONE;
                    }
                    ads_relrb(rb);
                } else {
                    gstat = RTNONE;
                }
            }
            break;

        case E_keyword:
            *arg_buffer = EOS;
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getkword(ps, arg_buffer);
            glen = (gstat == RTNORM) ? (strlen(arg_buffer) + 1) : 2;
            if (ai->argdefault != NULL && strlen(arg_buffer) == 0)
                gstat = RTNONE;
            break;

        case E_triple:
        case E_position:
        case E_displacement:
        case E_direction:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getpoint((ads_real *) ai->argbase, ps,
                                 (ads_real *) arg_buffer);
            glen = sizeof(ads_point);
            break;

        case E_corner:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getcorner((ads_real *) ai->argbase, ps,
                                  (ads_real *) arg_buffer);
            glen = sizeof(ads_point);
            break;

        case E_real:
        case E_scalefactor:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getreal(ps, (ads_real *) arg_buffer);
            glen = sizeof(ads_real);
            break;

        case E_distance:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getdist((ads_real *) ai->argbase, ps,
                                (ads_real *) arg_buffer);
            glen = sizeof(ads_real);
            break;

        case E_integer:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getint(ps, (int *) arg_buffer);
            glen = sizeof(int);
            break;

        case E_angle:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getangle((ads_real *) ai->argbase, ps,
                                 (ads_real *) arg_buffer);
            glen = sizeof(ads_real);
            break;

        case E_orientation:
            ads_initget(ai->argmodes, ai->argkw);
            gstat = ads_getorient((ads_real *) ai->argbase, ps,
                                  (ads_real *) arg_buffer);
            glen = sizeof(ads_real);
            break;
    }

    switch (gstat) {

        case RTNORM:                  /* Normal result received */
            gstat = 0;
            break;

        case RTNONE:                  /* Null input: use default value */
            if ((ai->argdefault != NULL) && (bargtype != E_pointer)) {
                if (bargtype == E_string)
                    glen = strlen(ai->argdefault) + 1;
                memcpy(arg_buffer, ai->argdefault, glen);
            } else {
                if (bargtype <= E_pointer) {
                    arg_buffer = EOS;
                    glen = 1;
                } else {
                    memset(arg_buffer, 0, glen);
                }
            }
            gstat = 0;
            break;

        case RTKWORD:                 /* Keyword selected: return as string */
            ads_getinput(arg_buffer);
            glen = strlen(arg_buffer) + 1;
            gstat = 1;
            break;

        case RTCAN:                   /* Input canceled */
            gstat = -1;
            glen = 0;
            break;
    }

    /* Round up to next stackitem boundary */

    glen = ((glen + (sizeof(stackitem) - 1)) / sizeof(stackitem)) *
                sizeof(stackitem);

    /* Pass the start of items to push and the number to push back
       to the caller.  If the datum was a string or point type, we
       return it as a pointer, which we must now allocate and initialise. */

    if ((gstat >= 0) && ((gstat == 1) ||
            (bargtype >= E_string && bargtype < E_real))) {
        stackitem *argp = (stackitem *) (arg_buffer + glen);

        *arg_area += glen;            /* Advance argument pointer */
        *argp = (stackitem) arg_buffer; /* Store pointer to item */
        arg_buffer = (char *) argp;   /* Make result the pointer */
        glen = sizeof(stackitem);     /* And adjust size of result */
    }

    /* If a keyword was specified, return an additional stackitem that
       indicates whether a keyword was entered. */

    if (ai->argkw != NULL) {
        *((stackitem *) (arg_buffer + glen)) = gstat;
        glen += sizeof(stackitem);
    }

    *pushwhat = (stackitem *) arg_buffer; /* What to push */
    *pushnum = StackCells(glen);      /* How many stackitems to push */

    *arg_area += glen;                /* Advance argument pointer */

    return gstat;                     /* Return simplified status */
}
