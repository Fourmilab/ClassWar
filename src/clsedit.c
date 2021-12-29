/*

    Class definition and editing facilities

*/

#include "class.h"

/*  Names used to interface with AutoCAD.  */

#define ClassBlock  "$CD$"            /* Class definition block name */
#define ClassFile   "$CF$"            /* Class definition file block name */

#define ClassFileName   "CLASS.FILENAME" /* Class editor file name generator */
#define ClassEdit       "CLASS.EDIT"  /* Class editor */
#define TempFileSuffix  ".tmp"        /* Default temporary file suffix */
#define ClassLines  100               /* Class definition max lines */

/*  Local variables  */

static char classfblock[40];          /* Class file block name */

/*  MAKECLASS  --  Make the master class block definition in the drawing, if
                   required.  */

static void makeclass()
{

    /* Construct class definition block from prefix and our application
       name.  This allows class definitions from various applications
       to coexist in a drawing without name clashes or disputes
       over who implements what. */

    strcpy(classblock, ClassBlock);
    strcat(classblock, ourapp);

    if ((rb = ads_tblsearch("BLOCK", classblock, False)) != NULL) {
        ads_relrb(rb);
    } else {
        int i;

        defent("BLOCK");

        tackstring(2, classblock);    /* Block name */
        tackint(70, 2);               /* Block flags */
        tackpoint(10, 0.0, 0.0, 0.0); /* Block origin */
        makent();

        defent("ATTDEF");
        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
        tackreal(40, 0.2);            /* Text height */
        tackstring(1, "");            /* Default value */
        tackstring(2, "CLASSNAME");   /* Attribute tag */
        tackstring(3, "Class");       /* Attribute prompt */
        tackint(70, 9);               /* Attribute is invisible */
        makent();

        defent("ENDBLK");
        i = ads_entmake(rb);
        if (i != RTKWORD) {
            ads_printf("Error creating %s entity\n", rb->resval.rstring);
        }
        ads_relrb(rb);
    }

    /*  Similarly, create the class file definition block.  */

    strcpy(classfblock, ClassFile);
    strcat(classfblock, ourapp);

    if ((rb = ads_tblsearch("BLOCK", classfblock, False)) != NULL) {
        ads_relrb(rb);
    } else {
        int i;

        defent("BLOCK");

        tackstring(2, classfblock);   /* Block name */
        tackint(70, 2);               /* Block flags */
        tackpoint(10, 0.0, 0.0, 0.0); /* Block origin */
        makent();

        defent("ATTDEF");
        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
        tackreal(40, 0.2);            /* Text height */
        tackstring(1, "");            /* Default value */
        tackstring(2, "CLASSNAME");   /* Attribute tag */
        tackstring(3, "Class name");  /* Attribute prompt */
        tackint(70, 9);               /* Attribute is invisible */
        makent();

        defent("ATTDEF");
        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
        tackreal(40, 0.2);            /* Text height */
        tackstring(1, "");            /* Default value */
        tackstring(2, "CLASSFILE");   /* Attribute tag */
        tackstring(3, "File name");   /* Attribute prompt */
        tackint(70, 9);               /* Attribute is invisible */
        makent();

        defent("ENDBLK");
        i = ads_entmake(rb);
        if (i != RTKWORD) {
            ads_printf("Error creating %s entity\n", rb->resval.rstring);
        }
        ads_relrb(rb);
    }
}

/*  PURGE_CLASSES  --  Delete all in-memory class definition items.  */

static void purge_classes()
{
    while (!qempty(&classq)) {
        classitem *ci = (classitem *) qremove(&classq);
        if (ci->classfile != NULL)    /* Free file name buffer */
            free(ci->classfile);
        free(ci);
    }
    extlist = NULL;                   /* Clear external method list */
}

/*  PURGE_COMMANDS  --  Delete all AutoCAD command definitions.  */

static void purge_commands()
{
    while (!qempty(&cmdef)) {
        cmditem *cm = (cmditem *) qremove(&cmdef);
        char cmdname[40];

        strcpy(cmdname, "C:");
        strcat(cmdname, cm->cm_method->wname + 1);
        if (ads_undef(cmdname, cm->cm_defun_index) != RTNORM) {
            ads_printf("Error in ads_undef of %s\n", cmdname);
        }
    }
}

/*  CLASSCAN  --  Scan the drawing and prepare our in-memory list of
                  all class definitions within it.  */

static void classcan()
{
    char c;
    ads_name classset;

    defent("INSERT");
    c = classblock[2];                /* Build wild card for all our classes */
    classblock[2] = '?';
    tackstring(2, classblock);
    classblock[2] = c;
    if (ads_ssget("X", NULL, NULL, rb, classset) == RTNORM) {
        long l = 0;
        ads_name en;

        while (ads_ssname(classset, l, en) == RTNORM) {
            Boolean isfile;

            l++;
            ri = ads_entget(en);
            if (ri != NULL) {
                classitem *ci = geta(classitem);
                Boolean classok = True;

                Cname(ci->classent, en);
                isfile = resitem(ri, 2)->resval.rstring[2] == 'F';
                ci->classinstv = ci->classv = ci->classfile = NULL;
                ci->classinstvl = ci->classvl = 0;
                ads_relrb(ri);
                ads_entnext(en, en);
                if ((ri = ads_entget(en)) != NULL) {
                    strncpy(ci->classname,
                        resitem(ri, 1)->resval.rstring, 31);
                    ci->classname[32] = 0;
                    ucase(ci->classname);

                    /* If the class name is void or some idiot
                       left the original question mark prompt in
                       the class name, mark it invalid. */

                    if ((strlen(ci->classname) == 0) ||
                        (strcmp(ci->classname, "?") == 0)) {
                        classok = False;
                    }
                    ads_relrb(ri);
                    if (isfile && classok) {
                        ads_entnext(en, en);
                        if ((ri = ads_entget(en)) != NULL) {
                            char *cf = resitem(ri, 1)->resval.rstring;

                            if (strlen(cf) == 0) {
                                char cfname[40];

                                strcpy(cfname, ci->classname);
                                lcase(cfname);
                                strcat(cfname, ".cls");
                                ci->classfile = strsave(cfname);
                            } else {
                                ci->classfile = strsave(cf);
                            }
                            ads_relrb(ri);
                        }
                    }
                }
                if (classok) {
                    qpush(&classq, &ci->clq);
                } else {
                    /* Class definition is invalid.  Ditch it. */
                    ads_entdel(ci->classent);
                    free(ci);
                }
            }
        }
        ads_ssfree(classset);
    }
    ads_relrb(rb);
}

/*  CLASSLOAD  --  Load a single class definition.  */

static void classload(ci)
  classitem *ci;
{
    ci->classhdr = NULL;              /* Mark class initially not loaded */
    ci->cl_constructor = NULL;        /* No instance constructor */
    ci->cl_newclass = NULL;           /* No class constructor */
    ci->cl_acquisition = NULL;        /* No acquisition method */
    md_classitem = ci;                /* Point methods at classitem */

    if (ci->classfile != NULL) {
        char cfname[60];
        char filename[256];
        FILE *fp;

        strcpy(cfname, ci->classfile);
        if (strchr(cfname, '.') == NULL)
            strcat(cfname, ".cls");
        if (ads_findfile(cfname, filename) == RTNORM) {
           fp = fopen(filename,
#ifdef FBmode
                               "rb"
#else
                               "r"
#endif
           );
        } else {
            fp = NULL;
            ads_printf("Cannot locate class definition file %s.\n",
                cfname);
        }
        if (fp != NULL) {
            int s;
            char iline[80];

            sprintf(iline, "CLASS %s", ci->classname);
            if ((s = atl_eval(iline)) == ATL_SNORM) {
                s = atl_load(fp);
            }
            fclose(fp);
            if (s == ATL_SNORM) {
                V atl_eval("CLASS.END");
            } else {
                V atl_eval("CLASS.ERR");
                ads_printf(
                    "\nError loading class %s from %s file on line %ld.\n",
                    ci->classname, filename, atl_errline);
            }
        }
    } else {
        char iline[80];
        ads_name ename;
        int es = ATL_SNORM;
        int lineno = 0;
        atl_statemark mk;
        atl_int scomm = atl_comment;      /* Stack comment pending state */
        struct resbuf *rp;

        atl_mark(&mk);
        atl_comment = 0;
        Cname(ename, ci->classent);
        ads_entnext(ename, ename);    /* Skip first attribute (class name) */
        sprintf(iline, "CLASS %s", ci->classname);
        if ((es = atl_eval(iline)) == ATL_SNORM) {
            while (ads_entnext(ename, ename) == RTNORM) {
                Boolean alldone = False;
                char *istr;

                if ((rp = ads_entget(ename)) == NULL) {
                    break;                    /* Huh ? */
                }
                if ((alldone =
                    strcmp((istr = resitem(rp, 0)->resval.rstring),
                        "SEQEND") == 0) != 0) {
                    istr = "CLASS.END";
                } else {
                    istr = resitem(rp, 1)->resval.rstring;
                }

                lineno++;
                if ((es = atl_eval(istr)) != ATL_SNORM) {
                    atl_unwind(&mk);
                    ads_relrb(rp);
                    break;
                }
                ads_relrb(rp);
                if (alldone)
                    break;
            }
        }
        if ((es == ATL_SNORM) && (atl_comment != 0)) {
            V ads_printf("\nRunaway `(' comment.\n");
            atl_unwind(&mk);
            es = ATL_RUNCOMM;
        }
        atl_comment = scomm;              /* Unstack comment pending status */
        if (es != ATL_SNORM) {
            V atl_eval("CLASS.ERR");
            ads_printf("\nError compiling class %s on line %d.\n",
                ci->classname, lineno);
        }
    }
    md_classitem = NULL;
}

/*  CLASSCLEAN  --  Delete all existing class and method definitions,
                    remove all AutoCAD commands we defined, and release
                    all storage we allocated, both on the Atlas heap
                    and otherwise.  This is called upon exit from the
                    drawing editor, and prior to reloading all class
                    definitions.  */

void classclean()
{
#define BookMark    "(CLASS_BOOKMARK)"

    purge_classes();                  /* Delete class definition items */
    purge_commands();                 /* Delete AutoCAD command definitions */
    P_argwipe();                      /* Ditch any unprocessed arguments */
    if (atl_lookup(BookMark) != NULL) {
        V atl_eval("forget (CLASS_BOOKMARK)");
    }
}

/*  CLASSRLOAD  --  Load (or reload) all classes.  Any existing class
                    definitions are purged from the Atlas system. */

static void classrload()
{
    classitem *ci;
    dictword *di;

    classclean();                     /* Delete all existing definitions */
    V atl_vardef(BookMark, 4);
    classcan();
    ci = (classitem *) &classq;

    while ((ci = (classitem *) qnext(ci, &classq)) != NULL) {
        classload(ci);
    }

    /* Look up the virtual draw method.  If none is defined, or
       the draw word is defined as something else, mark no
       drawing message present. */

    draw_message = NULL;
    if ((di = atl_lookup(DrawName)) != NULL) {
        if (di->wcode == P_vfexec) {
            draw_message = (struct queue *) atl_body(di);
        }
    }
}

/*  CLASSTOFILE  --  Export a class to a temporary file.  */

static Boolean classtofile(cname, ew)
  ads_name cname;
  dictword *ew;
{
    ads_name ename;
    char classname[32];
    char filename[60];
    struct resbuf *rp, *ri;
    dictword *df;
    FILE *fp;
    Boolean changed;
    char eline[256];
    int blanks = 0;

    if (ads_entnext(cname, ename) != RTNORM)
        return False;

    if (((rp = ads_entget(ename)) == NULL) ||
        ((ri = resitem(rp, 1)) == NULL)) {
        ads_relrb(rp);
        return False;
    }

    strncpy(classname, ri->resval.rstring, 31);
    classname[31] = EOS;
    if (classname[0] == '?') {
        strcpy(classname, "_NEW_");
    }
    ads_relrb(rp);

    /* If the user has defined ClassFileName, let him massage the file
       name before we open it.  */

    if ((df = atl_lookup(ClassFileName)) != NULL) {
        So(1);
        V strcpy(strbuf[cstrbuf], classname);
        Push = (stackitem) strbuf[cstrbuf];
        cstrbuf = (cstrbuf + 1) % ((int) atl_ntempstr);
        if (atl_exec(df) != ATL_SNORM)
            return False;
        Sl(1);
        strncpy(filename, (char *) S0, 59);
        filename[59] = EOS;
        Pop;
    } else {
        /* Default file name generation. */

        strcpy(filename, classname);
        strcat(filename, TempFileSuffix);
    }

    /* Now export the class definition from the attributes into
       a file the user can edit. */

    if ((fp = fopen(filename, "w")) == NULL)
        return False;

    fprintf(fp, "CLASS %s\n", classname);
    while (ads_entnext(ename, ename) == RTNORM) {
        if ((rp = ads_entget(ename)) == NULL) {
            break;                    /* Huh ? */
        }
        if (strcmp(resitem(rp, 0)->resval.rstring, "SEQEND") == 0) {
            ads_relrb(rp);
            break;
        }
        if (strlen(resitem(rp, 1)->resval.rstring) == 0) {
            blanks++;
        } else {
            while (blanks-- > 0)
                fprintf(fp, "\n");
            fprintf(fp, "%s\n", resitem(rp, 1)->resval.rstring);
        }
        ads_relrb(rp);
    }
    fclose(fp);

    /* Invoke the user's CLASS.EDIT method to edit the file. */

    So(1);
    V strcpy(strbuf[cstrbuf], filename);
    Push = (stackitem) strbuf[cstrbuf];
    cstrbuf = (cstrbuf + 1) % ((int) atl_ntempstr);
    if (atl_exec(ew) != ATL_SNORM)
        return False;
    Sl(1);
    changed = (Boolean) S0;
    Pop;

    /* Finally, read the file back and plug the values into the
       attributes for the class definition. */

    if (changed) {
        Boolean cline = True;

        if ((fp = fopen(filename, "r")) == NULL)
            return False;

        ads_entnext(cname, ename);

        while (atl_fgetsp(eline, 255, fp) != NULL) {
            Boolean lmod = False;

            rp = ads_entget(ename);
            if ((rp == NULL) || (strcmp(resitem(rp, 0)->resval.rstring,
                "ATTRIB") != 0)) {
                ads_printf("Warning!  Class %s is too long--truncated.\n",
                    classname);
                if (rp != NULL) {
                    ads_relrb(rp);
                    break;
                }
            }
            if (cline) {
                if (strncmp(eline, "CLASS ", 6) != 0) {
                    ads_printf("Class name missing in first line.\n");
                    changed = False;
                    ads_relrb(rp);
                    break;
                }
                ri = resitem(rp, 1);
                if (strcmp(ri->resval.rstring, eline + 6) != 0) {
                    lmod = True;
                    free(ri->resval.rstring);
                    ri->resval.rstring = strsave(eline + 6);
                }
                cline = False;
            } else {
                ri = resitem(rp, 1);
                if (strcmp(ri->resval.rstring, eline) != 0) {
                    lmod = True;
                    free(ri->resval.rstring);
                    ri->resval.rstring = strsave(eline);
                }
            }
            if (lmod)
                ads_entmod(rp);
            ads_relrb(rp);
            ads_entnext(ename, ename);
        }

        /* Finally, verify that all lines after the last line from
           the file are blank. */

        while (True) {
            rp = ads_entget(ename);
            if ((rp == NULL) || (strcmp(resitem(rp, 0)->resval.rstring,
                "ATTRIB") != 0)) {
                if (rp != NULL) {
                    ads_relrb(rp);
                    break;
                }
            }
            if (strlen(resitem(rp, 1)->resval.rstring) > 0) {
                resitem(rp, 1)->resval.rstring[0] = EOS;
                ads_entmod(rp);
            }
            ads_relrb(rp);
            ads_entnext(ename, ename);
        }
    }

    unlink(filename);

    return changed;
}

/*  CLS_EDINIT  --  Reinitialise the class system at the start of
                    a new drawing.  The objects required to support
                    the class system are created if not already present
                    in the drawing.  */

void cls_edinit(appname)
  char *appname;
{
    struct resbuf *rp;

    ourapp = appname;                 /* Remember our application name */

    /* First of all, see if our application is registered
       and register it if not. */

    if ((rp = ads_tblsearch("APPID", ourapp, False)) == NULL) {
        if (ads_regapp(ourapp) != RTNORM) {
            ads_printf("Cannot register application %s.\n", ourapp);
            return;
        }
    } else {
        ads_relrb(rp);
    }
    applist.restype = RTSTR;          /* Build extended data filter list */
    applist.resval.rstring = strsave(ourapp);
    applist.rbnext = NULL;
    makeclass();                      /* Define blocks if not present. */
    classrload();                     /* Reload all class definitions */
}

/*  CLS_UPDATE  --  Reload all class definitions.  This allows
                    changes made in external file class definitions
                    to be applied within a drawing session.  Note
                    that this operation does not cause instance
                    updating, however, as CLS_EDIT does.  It is mainly
                    a convenience for development.  */

void cls_update()
{
    classrload();
}

/*  CLS_EDIT  --  Edit a class definition.  Called with the ADS_NAME
                  of the block containing the class definition.  It
                  returns a Boolean indicating whether the class was
                  modified or not.  */

Boolean cls_edit(cname, inspector)
  ads_name cname;
  Boolean inspector;
{
    struct resbuf *rb = ads_entget(cname);
    Boolean changed = False;
    char classname[32];

    if (rb == NULL ||
        (strcmp(resitem(rb, 0)->resval.rstring, "INSERT") != 0)) {
        ads_printf("CLS_EDIT: Invalid entity name.\n");
        return False;
    }

    /* If this is an embedded class definition, edit it either with
       DDATTE or with the user-defined CLASS.EDIT mechanism, if one
       is supplied. */

    if (strcmp(resitem(rb, 2)->resval.rstring, classblock) == 0) {
        dictword *ce = atl_lookup(ClassEdit);
        ads_name ename;

        ads_relrb(rb);
        if (ce == NULL) {
            struct resbuf diaok;

            CommandB();
            ads_command(RTSTR, "DDAttE", RTENAME, cname, RTNONE);
            CommandE();
            changed = (ads_getvar("DIASTAT", &diaok) == RTNORM) ?
                diaok.resval.rint : True;
        } else {
            changed = classtofile(cname, ce);
        }

        /* Now see if the user has specified inclusion of the class
           from a file.  If so, replace the class definition in
           the attributes with the contents of the named file. */

        classname[0] = EOS;
        if (!inspector && ads_entnext(cname, ename) == RTNORM &&
            ((rb = ads_entget(ename)) != NULL)) {
            if (strcmp(resitem(rb, 0)->resval.rstring, "ATTRIB") == 0) {
                strncpy(classname, resitem(rb, 1)->resval.rstring, 32);
                classname[31] = EOS;
            }
            ads_relrb(rb);
        }

        if (strlen(classname) > 0 &&
            ads_entnext(ename, ename) == RTNORM &&
            ((rb = ads_entget(ename)) != NULL)) {

            if (strcmp(resitem(rb, 0)->resval.rstring, "ATTRIB") == 0) {
                char *fline = resitem(rb, 1)->resval.rstring;
/* ads_printf("First line: %s\n", fline); */
                if (fline[0] == '<') {
                    char fname[256];
                    FILE *fp;

                    strcpy(fname, fline + 1);
                    if (strchr(fname, '.') == NULL)
                        strcat(fname, ".cls");
/* ads_printf("Replace with file %s\n", fname); */
                    if ((fp = fopen(fname,
#ifdef FBmode
                               "rb"
#else
                               "r"
#endif
                        )) == NULL)  {
                        ads_printf("Cannot open class file %s\n", fname);
                    } else {
                        int i, j;

                        ads_entdel(cname);  /* Delete old definition block */

                        defent("INSERT");
                        tackint(66, 1);               /* Attributes present */
                        tackstring(2, classblock);    /* Block name */
                        tackpoint(10, 0.0, 0.0, 0.0); /* Block location */
                        makent();

                        defent("ATTRIB");
                        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
                        tackreal(40, 0.2);            /* Text height */
                        tackstring(1, classname);     /* Initial value */
                        tackstring(2, "CLASSNAME");   /* Attribute tag */
                        tackint(70, 9);               /* Attribute is invisible */
                        makent();

                        i = 0;
                        while (True) {
                            char lineno[40];
                            char filedata[256];

                            if (atl_fgetsp(filedata, sizeof filedata, fp) ==
                                NULL) {
                                break;
                            }
                            defent("ATTRIB");
                            tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
                            tackreal(40, 0.2);        /* Text height */
                            tackstring(1, filedata);  /* Default value */
                            sprintf(lineno, "%3d", ++i);
                            tackstring(2, lineno);    /* Attribute tag */
                            tackint(70, 9);           /* Attribute is invisible */
                            makent();
                        }

                        j = max(ClassLines, i) + i / 10;
                        while (i < j) {
                            char lineno[40];

                            defent("ATTRIB");
                            tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
                            tackreal(40, 0.2);        /* Text height */
                            tackstring(1, "");        /* Default value */
                            sprintf(lineno, "%3d", ++i);
                            tackstring(2, lineno);    /* Attribute tag */
                            tackint(70, 9);           /* Attribute is invisible */
                            makent();
                        }

                        defent("SEQEND");
                        makent();

                        ads_entlast(cname);

                        fclose(fp);
                        rb = NULL;    /* Indicate result chain released */
                    }
                }
            }
            if (rb != NULL)
                ads_relrb(rb);
        }
    }

    /* If this is a class defined in a file, allow the user to change
       the file name using DDATTE. */

    else if (strcmp(resitem(rb, 2)->resval.rstring, classfblock) == 0) {
        struct resbuf diaok;

        ads_relrb(rb);
        CommandB();
        ads_command(RTSTR, "DDAttE", RTENAME, cname, RTNONE);
        CommandE();
        changed = (ads_getvar("DIASTAT", &diaok) == RTNORM) ?
            diaok.resval.rint : True;
    }
    return changed;
}

/*  CLS_NEW  --  Interactively define a new class.  */

void cls_new()
{
    int i;
    ads_name en;

    defent("INSERT");
    tackint(66, 1);                   /* Attributes present */
    tackstring(2, classblock);        /* Block name */
    tackpoint(10, 0.0, 0.0, 0.0);     /* Block location */
    makent();

    defent("ATTRIB");
    tackpoint(10, 0.0, 0.0, 0.0);     /* Attribute location */
    tackreal(40, 0.2);                /* Text height */
    tackstring(1, "?");               /* Initial value */
    tackstring(2, "CLASSNAME");       /* Attribute tag */
    tackint(70, 9);                   /* Attribute is invisible */
    makent();

    for (i = 0; i < ClassLines; i++) {
        char lineno[40];

        defent("ATTRIB");
        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
        tackreal(40, 0.2);        /* Text height */
        tackstring(1, "");        /* Default value */
        sprintf(lineno, "%3d", i + 1);
        tackstring(2, lineno);    /* Attribute tag */
        tackint(70, 9);           /* Attribute is invisible */
        makent();
    }

    defent("SEQEND");
    makent();

    if (ads_entlast(en) == RTNORM) {
        if (cls_edit(en, False)) {
/* *** IT WOULD BE REALLY NEAT IF THIS COULD JUST LOAD THE NEW CLASS
       WITHOUT HAVING TO RELOAD EVERYTHING.  TO FIX THIS I WILL HAVE
       TO SPLIT UP CLASSCAN TO SCAN A SINGLE CLASSDEF AND BUILD THE
       NEW CI ITEM INDIVIDUALLY.  *** */
            classrload();
        } else {
            ads_entdel(en);           /* Delete the new class definition */
        }
    }
}

/*  CLS_NEWF  --  Interactively define a new file class.  */

void cls_newf()
{
    ads_name en;

    defent("INSERT");
    tackint(66, 1);                   /* Attributes present */
    tackstring(2, classfblock);       /* Block name */
    tackpoint(10, 0.0, 0.0, 0.0);     /* Block location */
    makent();

    defent("ATTRIB");
    tackpoint(10, 0.0, 0.0, 0.0);     /* Attribute location */
    tackreal(40, 0.2);                /* Text height */
    tackstring(1, "?");               /* Initial value */
    tackstring(2, "CLASSNAME");       /* Attribute tag */
    tackint(70, 9);                   /* Attribute is invisible */
    makent();

    defent("ATTRIB");
    tackpoint(10, 0.0, 0.0, 0.0);     /* Attribute location */
    tackreal(40, 0.2);                /* Text height */
    tackstring(1, "");                /* Initial value */
    tackstring(2, "CLASSFILE");       /* Attribute tag */
    tackint(70, 9);                   /* Attribute is invisible */
    makent();

    defent("SEQEND");
    makent();

    if (ads_entlast(en) == RTNORM) {
        if (cls_edit(en, False)) {
/* *** IT WOULD BE REALLY NEAT IF THIS COULD JUST LOAD THE NEW CLASS
       WITHOUT HAVING TO RELOAD EVERYTHING.  TO FIX THIS I WILL HAVE
       TO SPLIT UP CLASSCAN TO SCAN A SINGLE CLASSDEF AND BUILD THE
       NEW CI ITEM INDIVIDUALLY.  *** */
            classrload();
        } else {
            ads_entdel(en);           /* Delete the new class definition */
        }
    }
}

/*  CHOOSECLASS  --  Allow the user to select a class either by pointing
                     to an instance of the class or by entering the
                     class name.  */

classitem *chooseclass()
{
    char clname[132];
    classitem *ci;
    ads_name ent;
    ads_point pt;

    if ((ads_entsel("Select class instance, or CR to name class: ",
        ent, pt) == RTNORM) && ((ci = entclass(ent, False)) != NULL)) {
        return ci;
    } else while (ads_getstring(False,
                        "Class name or ?: ", clname) == RTNORM) {
        if (clname[0] == '?') {
            if (ads_getstring(False, "Classes to list <*>: ",
                clname) == RTNORM) {
                if (strlen(clname) == 0)
                    strcpy(clname, "*");
                ucase(clname);
                ci = (classitem *) &classq;
                ads_textpage();
                ads_printf("Defined classes.\n");
                while ((ci = (classitem *) qnext(ci, &classq)) != NULL) {
                    if (ads_wcmatch(ci->classname, clname) == RTNORM) {
                        ads_printf("  %s\n", ci->classname);
                    }
                }
                continue;
            }
        } else if (strlen(clname) == 0) {
            break;
        } else if ((ci = cls_lookup(clname)) != NULL) {
            return ci;
        } else {
            ads_printf("No such class.\n");
        }
        break;
    }
    return NULL;
}

/*  CLS_ENAME  --  Edit class by name.  */

void cls_ename()
{
    Boolean changed = False;
    classitem *ci = chooseclass();
    char clname[32];

    if (ci != NULL) {
        strcpy(clname, ci->classname);
        if (cls_edit(ci->classent, False)) {
            classrload();
            changed = True;
        }
    }

    /* If we changed a class definition, update all instances of
       that class that exist in the drawing. */

    if (changed) {

        /* Look up new definition of class and verify that it compiled
           correctly.  If there was an error compiling the class, leave
           existing instances alone. */

        if (((ci = cls_lookup(clname)) != NULL) &&
            (ci->classhdr != NULL)) {
            struct resbuf filt1, filt2;
            ads_name ss, en;
            long idx = 0L;

            filt1.restype = 0;
            filt1.rbnext = &filt2;
            filt1.resval.rstring = "INSERT";
#ifndef SSGET_WC_BUSTED
            filt2.restype = 2;
            filt2.rbnext = NULL;
            filt2.resval.rstring = "`*U*";
#else
            filt1.rbnext = NULL;
#endif
            ads_ssget("X", NULL, NULL, &filt1, ss);
            while (ads_ssname(ss, idx++, en) == RTNORM) {
                if (entclass(en, True) == ci) {
                    char *bname;
/* ads_printf("Updating %s instance.\n", ci->classname); */
                    bname = create_geometry(ci, ci->classinstv);
                    if (bname != NULL) {
                        rb = ads_entget(en);
                        free(resitem(rb, 2)->resval.rstring);
                        resitem(rb, 2)->resval.rstring = strsave(bname);
                        if (ads_entmod(rb) != RTNORM) {
                            ads_printf("Error in ads_entmod on update.\n");
                        }
                        ads_relrb(rb);
                    }
                }
            }
            ads_ssfree(ss);
        } else {
            ads_printf("Class %s destroyed by edit: no instance update!\n",
                clname);
        }
    }
}
