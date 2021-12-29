/*

        Interfaces to external methods (implemented in other applications).

*/

#include "class.h"

/* Argument chain assembly list */

static struct resbuf *argchain = NULL, *argtail;

/*  DXFTOADS  --  This is a truly awful routine that converts all
                  the result buffer type fields in a result buffer
                  chain from DXF-type codes to the corresponding
                  ADS types.  This allows us to use common code for
                  assembling application data and transmitting
                  messages across an ads_invoke link.  */

static void dxftoads(rp)
  struct resbuf *rp;
{
    while (rp != NULL) {
        switch ((rp->restype % 100) / 10) {
            case 0:
                rp->restype = RTSTR;
                break;

            case 1:
                rp->restype = RTPOINT;
                break;

            case 4:
                rp->restype = RTREAL;
                break;

            case 7:
                /* Yer gonna be sick.  Since Atlas uses LONGs for all
                   its integers and extended entity data provides a
                   LONG type, we allow all integer class and instance
                   variables to be 32 bits.  Everything is fine until
                   we want to pass one of these variables to another
                   application with ads_invoke().  Since it uses
                   chains of result buffers, and these chains allow for
                   RTLOG values, you'd think that would work fine.
                   You'd be wrong.  So...we actually transmit the LONG
                   as a REAL, adding a little fuzz factor so it truncates
                   the right way on the other side. */

                rp->restype = RTREAL;
                rp->resval.rreal = rp->resval.rlong + 0.1;
                break;

            default:
                assert(False);
        }

        rp = rp->rbnext;
    }
}

/*  UPDVARS  --  Update class and instance variables from results
                 returned by the external method.  */

static char *updv_buf;                /* Current variable buffer */
static Boolean updv_class;            /* Doing class variables */

static Boolean updvars(cc, dw, vtype, vflags, vlen, clip, clcp, vclass)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
  classdecl *vclass;
{
    long offset = updv_class ? clcp : clip;
#ifdef UPDVDEBUG
ads_printf(
"Update variable: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld, clcp %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip, clcp);
#endif

    if ((ri != NULL) && (offset != -1) && (abs(vtype) != E_instance)) {
        if (vtype <= E_pointer) {
            strcpy(updv_buf + offset, ri->resval.rstring);
        } else if (vtype == E_integer) {
            *((long *) (updv_buf + offset)) = ri->resval.rreal;
        } else {
            memcpy(updv_buf + offset, (char *) &(ri->resval.rint), vlen);
        }
        ri = ri->rbnext;
    }
    return True;
}

/*  EXT_ARGSTACK  --  Place the next argument on the argument queue.  */

void ext_argstack(atype, value, vlength, kwflag)
  int atype;
  char *value;
  int vlength;
  Boolean kwflag;
{
    struct resbuf *ru = ads_newrb(atype);
    int bargtype = atype % 100;

    /* If this argument acquisition request allowed optional
       keywords, the data to be pushed is followed by a flag
       that indicates whether a keyword was supplied.  We
       examine this flag and, if a keyword was entered, emit a
       string result buffer containing the keyword.  Whether a
       keyword was entered or not, we remove the keyword item
       from the end of the data. */

    if (kwflag) {
        stackitem *sia = (stackitem *) value;

        if (sia[vlength - 1] != 0) {
            ru->restype = atype = E_string;
        }
        vlength--;
    }

    /* Copy the argument to the result buffer.  Note that since
       strings and points are passed as pointers, we have to
       dereference to get the actual data for the result buffer. */

    if (atype <= E_pointer) {
        ru->resval.rstring = strsave(*((char **) value));
    } else if (atype == E_integer) {
        ru->resval.rreal = *((long *) value);
    } else if (bargtype >= E_triple && bargtype < E_real) {
        memcpy((char *) (ru->resval.rpoint), *((char **) value),
            sizeof(ads_real));
    } else {
        memcpy((char *) &(ru->resval.rint), value,
            vlength * sizeof(stackitem));
    }

    if (argchain == NULL) {
        argchain = argtail = ru;
    } else {
        argtail->rbnext = ru;
        argtail = ru;
    }
}

/*  CALL_EXTERNAL  --  Run an externally-defined method.  */

int call_external(vi)
  vitem *vi;
{
    int stat;
    char extname[80];
    struct resbuf *rrb;

    /* Get instance off the top of the stack */

    Sl(1);

    strcpy(extname, vi->vi_classitem->classname);
    strcat(extname, ".");
    strcat(extname, vi->vi_vword->wname + 1);

/*  ads_printf("Invoking external method %s.\n", extname);   */

    defent(extname);

    tack_classvar(vi->vi_classitem);
    tack_instance(vi->vi_classitem, (char *) S0);

    /* Affix the argument chain to the end of the class and instance
       variable result items.  */

    ri->rbnext = argchain;
    argchain = NULL;

/* dxf_pitem(rb); */

    dxftoads(rb);                     /* Ker-whack! */
    stat = ads_invoke(rb, &rrb);

#ifdef UPDVDEBUG
    {   struct resbuf *or = rrb;

        ads_printf("Variables returned:\n");
        /* Jam types to plausible values so dxf_pitem() doesn't crash. */
        while (or != NULL) {
            switch (or->restype) {
                case RTSTR:
                    or->restype = E_string;
                    break;
                case RT3DPOINT:
                    or->restype = E_triple;
                    break;
                case RTREAL:
                    or->restype = E_real;
                    break;
                default:
                    or->restype = E_integer;
                    break;
            }
            or = or->rbnext;
        }
        dxf_pitem(rrb);
    }
#endif
    ads_relrb(rb);

    ri = rrb;
    updv_class = True;
    updv_buf = vi->vi_classitem->classv; /* Update class variables */
    procvar(vi->vi_classitem, updvars);

    updv_class = False;
    updv_buf = (char *) S0;           /* Update instance variables */
    procvar(vi->vi_classitem, updvars);
    ads_relrb(rrb);                   /* Release result list */

    Pop;                              /* Drop the instance address */
    return (stat == RTNORM) ? ATL_SNORM : ATL_APPLICATION;
}

/*  CLS_TO_C  --  Export a class definition as a C structure and the
                  instructions for sending and receiving messages
                  across the ADS_INVOKE link.  */

static char insprefix[256];           /* Prefix for nested instances */
static Boolean insp_classvars;        /* Inspect class variables ? */
static int clclevel;                  /* Indentation level */
static FILE *clcfp;                   /* C header output file */
static Boolean clcprot;               /* Outputting protocol table */

static void clci()                    /* Output present indentation */
{
    int i;

    for (i = 0; i < clclevel; i++)
        fprintf(clcfp, "    ");
}

static Boolean clstocv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
    long offset = insp_classvars ? clcp : clip;
    char vname[82];

#ifdef CLTOCDEBUG
ads_printf(
"INSPV: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif

    strcpy(vname, dw->wname+1);
    lcase(vname);
    if (abs(vtype) == E_instance && offset != -1) {
        if (!(vflags & S_temp)) {
            if (vtype > 0) {
                strcat(insprefix, vname);
                strcat(insprefix, ".");
                if (!clcprot) {
                    clci();
                    fprintf(clcfp, "struct {\n");
                }
                clclevel++;
            } else {
                char *ipp;

                if (strlen(insprefix) > 0) {
                    insprefix[strlen(insprefix) - 1] = EOS;
                    if ((ipp = strrchr(insprefix, '.')) == NULL)
                        insprefix[0] = EOS;
                    else
                        ipp[1] = EOS;
                }
                clclevel--;
                if (!clcprot) {
                    clci();
                    fprintf(clcfp, "} %s;\n", vname);
                }
            }
        }
        return True;
    }

    if (offset != -1) {
        if (clcprot) {
            fprintf(clcfp, "    { %2d, G%c(%s%s) },\n", vtype,
                vtype < E_real ? 'p' : 'v', insprefix, vname);
        } else {
            clci();
            switch (vtype) {

                case E_string:
                case E_pointer:
                    fprintf(clcfp, "char %s[%d];\n", vname, vlen + 2);
                    break;

                case E_triple:
                case E_position:
                case E_displacement:
                case E_direction:
                    fprintf(clcfp, "ads_point %s;\n", vname);
                    break;

                case E_real:
                case E_distance:
                case E_scalefactor:
                    fprintf(clcfp, "ads_real %s;\n", vname);
                    break;

                case E_integer:
                    fprintf(clcfp, "long %s;\n", vname);
                    break;
            }
        }
    }
    return True;
}

void cls_to_c()
{
    classitem *ci = chooseclass();
    vitem *vi = extlist;

    if (ci != NULL) {
        char lclassname[32];
        char prompt[80];
        char filename[134];
        FILE *fp;
        Boolean firstime;

        strcpy(lclassname, ci->classname);
        lcase(lclassname);
        sprintf(prompt, "C header file name <%s.h>: ", lclassname);
        if (ads_getstring(False, prompt, filename) != RTNORM)
            return;
        if (strlen(filename) == 0)
            strcpy(filename, lclassname);
        if (strchr(filename, '.') == NULL)
            strcat(filename, ".h");
        if ((fp = fopen(filename, "w")) == NULL) {
            ads_printf("\nCannot open file %s\n", filename);
            return;
        }

        /* O.K.  The class is selected, the file is open, and we're
           ready to crank out some C.  Get on with it. */

        fprintf(fp, "/*\n\n");
        fprintf(fp, "    External method interface definition for class %s\n",
            ci->classname);
        fprintf(fp, "\n*/\n\n");
        fprintf(fp, "typedef struct {\n");
        fprintf(fp, "\n    /* Class variables */\n\n");

        clclevel = 1;
        clcprot = False;
        clcfp = fp;
        insp_classvars = True;
        insprefix[0] = EOS;
        procvar(ci, clstocv);

        fprintf(fp, "\n    /* Instance variables */\n\n");
        clclevel = 1;
        insp_classvars = False;
        insprefix[0] = EOS;
        procvar(ci, clstocv);
        fprintf(fp, "} s_%s;\n", lclassname);

        fprintf(fp, "\nstatic s_%s %s;\n\n", lclassname, lclassname);

        fprintf(fp, "/* Message protocol description */\n\n");

        fprintf(fp, "#define Gv(x) ((char *) &(%s.x))\n", lclassname);
        fprintf(fp, "#define Gp(x) ((char *)  (%s.x))\n\n", lclassname);
        fprintf(fp, "#ifndef m_Protocol_defined\n");
        fprintf(fp, "#define m_Protocol_defined 1\n");
        fprintf(fp, "typedef struct {\n");
        fprintf(fp, "    int p_type;\n");
        fprintf(fp, "    char *p_field;\n");
        fprintf(fp, "} m_Protocol;\n");
        fprintf(fp, "typedef struct {\n");
        fprintf(fp, "    char *methname;\n");
        fprintf(fp, "    void (*methfunc)();\n");
        fprintf(fp, "} method_Item;\n");
        fprintf(fp, "extern int beg_method(), end_method();\n");
        fprintf(fp, "extern void define_class();\n");
        fprintf(fp, "#endif\n\n");
        fprintf(fp, "static m_Protocol mP_%s[] = {\n",
            lclassname);

        clcprot = True;
        insp_classvars = True;
        insprefix[0] = EOS;
        procvar(ci, clstocv);

        clclevel = 1;
        insp_classvars = False;
        insprefix[0] = EOS;
        procvar(ci, clstocv);

        fprintf(fp, "    {  0, NULL}\n");
        fprintf(fp, "};\n", lclassname);
        fprintf(fp, "#undef Gv\n");
        fprintf(fp, "#undef Gp\n");

        /* Now generate argument descriptors for the external commands
           exported by this application. */

        while (vi != NULL) {
            if (ci == vi->vi_classitem) {
                int i;
                char mname[80];

                strcpy(mname, vi->vi_vword->wname + 1);
                lcase(mname);
                if (vi->vi_nargs > 0) {
                    fprintf(fp, "\n/* Arguments for %s method */\n\n",
                        vi->vi_vword->wname + 1);
                    fprintf(fp, "typedef struct {\n");
                    for (i = 0; i < vi->vi_nargs; i++) {
                        argitem *ai = vi->vi_arglist + i;
                        char vname[32];

                        sprintf(vname, "arg%d", i + 1);
                        fprintf(fp, "    ");
                        if (ai->argkw != NULL) {
                            fprintf(fp, "struct {\n", vname);
                            fprintf(fp, "        int kwflag;\n");
                            fprintf(fp, "        union {\n");
                            fprintf(fp, "            char *kwtext;\n");
                            fprintf(fp, "            ");
                            strcpy(vname, "value");
                        }
                        switch (ai->argtype) {
                            case E_string:
                            case E_keyword:
                                fprintf(fp, "char %s[256];\n", vname);
                                break;

                            case E_pointer:
                                fprintf(fp, "char %s[18];\n", vname);
                                break;

                            case E_triple:
                            case E_position:
                            case E_displacement:
                            case E_direction:
                            case E_corner:
                                fprintf(fp, "ads_point %s;\n", vname);
                                break;

                            case E_real:
                            case E_distance:
                            case E_scalefactor:
                            case E_angle:
                            case E_orientation:
                                fprintf(fp, "ads_real %s;\n", vname);
                                break;

                            case E_integer:
                                fprintf(fp, "long %s;\n", vname);
                                break;
                        }
                        if (ai->argkw != NULL) {
                            fprintf(fp, "        } kw;\n");
                            fprintf(fp, "    } arg%d;\n", i + 1);
                        }
                    }
                    fprintf(fp, "} aS_%s;\n", mname);
                    fprintf(fp, "\nstatic aS_%s %s;\n", mname, mname);
                }

                fprintf(fp, "\n/* Protocol for %s method */\n\n",
                    vi->vi_vword->wname + 1);
                if (vi->vi_nargs > 0) {
                    fprintf(fp, "#define Gv(x) ((char *) &(%s.x))\n",
                        mname);
                    fprintf(fp, "#define Gp(x) ((char *)  (%s.x))\n\n",
                         mname);
                }
                fprintf(fp, "static m_Protocol aP_%s[] = {\n", mname);
                for (i = 0; i < vi->vi_nargs; i++) {
                    argitem *ai = vi->vi_arglist + i;
                    char vname[32];

                    sprintf(vname, "arg%d", i + 1);
                    fprintf(fp, "    { %4d, G", ai->argtype *
                        ((ai->argkw != NULL) ? -1 : 1));
                    switch (ai->argtype) {
                        case E_string:
                        case E_pointer:
                        case E_keyword:
                        case E_triple:
                        case E_position:
                        case E_displacement:
                        case E_direction:
                        case E_corner:
                            fprintf(fp, "p(%s", vname);
                            break;

                        default:
                            fprintf(fp, "v(%s", vname);
                            break;
                    }
                    fprintf(fp, ") },\n");
                }
                fprintf(fp, "    {   0, NULL}\n");
                fprintf(fp, "};\n", mname);
                if (vi->vi_nargs > 0) {
                    fprintf(fp, "#undef Gv\n");
                    fprintf(fp, "#undef Gp\n");
                }
                fprintf(fp, "\nextern void M_%s_%s();\n", mname, lclassname);
                fprintf(fp,
"#define %s_%s void M_%s_%s() { if (beg_method(mP_%s, aP_%s) != RTNORM) return; {\n",
                    mname, lclassname, mname, lclassname, lclassname, mname);
                fprintf(fp, "#define end_%s_%s } end_method(); }\n",
                    mname, lclassname);
            }
            vi = vi->vi_extnext;
        }

        /* Make another pass over the external command list and generate
           the method declaration table used to register the methods with
           ads_invoke(). */

        vi = extlist;
        firstime = True;
        while (vi != NULL) {
            if (ci == vi->vi_classitem) {
                char mname[80];

                strcpy(mname, vi->vi_vword->wname + 1);
                lcase(mname);
                if (firstime) {
                    firstime = False;
                    fprintf(fp, "\n/* Method definition table */\n\n");
                    fprintf(fp, "method_Item %s[] = {\n", ci->classname);
                }
                fprintf(fp, "    {\"%s.%s\", M_%s_%s},\n", ci->classname,
                    vi->vi_vword->wname + 1, mname, lclassname);
            }
            vi = vi->vi_extnext;
        }
        if (!firstime) {
            fprintf(fp, "    {NULL, 0}\n");
            fprintf(fp, "};\n");

            /* As a final nicety, crank out a macro that generates the
               entire main program needed to interface with ADS and
               CLASSAPP. */

            fprintf(fp, "#define %s_methods void main(c,v)int c;char *v[];{main_method(c,v,%s);}\n",
                lclassname, ci->classname);
        }
        fclose(fp);
    }
}
