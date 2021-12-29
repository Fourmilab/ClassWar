/*

    Interactive inspection and modification of class and instance variables

*/

#include "class.h"

/*  CLS_INSPECT  --  Inspect instance variables of an object and
                     update the object if they are changed.

                     The CLS_SPY variant, enabled only for debugging,
                     allows access to protected and private variables. */

static char insprefix[256];           /* Prefix for nested instances */
static int insphide;                  /* Hide private nested instances */
static char *insp_inst;               /* Top-level instance pointer */
static Boolean insp_classvars;        /* Inspect class variables ? */

static Boolean inspv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
    long offset = insp_classvars ? clcp : clip;

#ifdef INSPDEBUG
ads_printf(
"INSPV: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif

    if (abs(vtype) == E_instance && offset != -1) {
        if (!(vflags & (S_temp & F_class))) {
            if (vtype > 0) {
                if (!spyring && (!(vflags & S_public)))
                    insphide++;
                if (insphide == 0) {
                    strcat(insprefix, dw->wname + 1);
                    strcat(insprefix, ".");
#ifdef INSPDEBUG
                    ads_printf("Stack prefix: (%s)\n", insprefix);
#endif
                }
            } else {
                char *ipp;

                if (insphide == 0) {
                    if (strlen(insprefix) > 0) {
                        insprefix[strlen(insprefix) - 1] = EOS;
                        if ((ipp = strrchr(insprefix, '.')) == NULL)
                            insprefix[0] = EOS;
                        else
                            ipp[1] = EOS;
                    }
                }
                if (!spyring && (!(vflags & S_public)))
                    insphide--;
#ifdef INSPDEBUG
                    ads_printf("Unstack prefix: (%s)\n", insprefix);
#endif
            }
        }
        return True;
    }

    if ((offset != -1) && !insphide && (spyring || (vflags & S_public))) {
        int i;
        char attval[256];
        ads_real *rp;

        switch (vtype) {

            case E_string:
            case E_pointer:
                strncpy(attval, insp_inst + offset, 256);
                attval[255] = EOS;
                break;

            case E_triple:
            case E_position:
            case E_displacement:
            case E_direction:
                attval[0] = EOS;
                rp = (ads_real *) (insp_inst + offset);
                for (i = 0; i < 3; i++) {
                    ads_rtos(*rp++, 2, 6, attval + strlen(attval));
                    if (i < 2)
                        strcat(attval, ",");
                }
                break;

            case E_real:
            case E_distance:
            case E_scalefactor:
                rp = (ads_real *) (insp_inst + offset);
                ads_rtos(*rp, 2, 6, attval);
                break;

            case E_integer:
                sprintf(attval, "%ld", *((long *) (insp_inst + offset)));
                break;
        }
        defent("ATTRIB");
        tackpoint(10, 0.0, 0.0, 0.0); /* Attribute location */
        tackreal(40, 0.2);            /* Text height */
        tackstring(1, attval);        /* Initial value */
        strcpy(attval, insprefix);    /* Start with instance prefix */
        strcat(attval, dw->wname + 1);/* And append the variable name */
        tackstring(2, attval);        /* Variable name is attribute tag */
        tackint(70, 9);               /* Attribute is invisible */
        makent();
    }
    return True;
}

static ads_name inspmv_en;            /* Hidden argument to inspmv */

static Boolean inspmv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
    long offset = insp_classvars ? clcp : clip;
#ifdef INSPDEBUG
ads_printf(
  "INSPMV: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif
    if ((offset != -1) && (abs(vtype) != E_instance) &&
        (spyring || (vflags & S_public))) {
        int i;
        char *attval;
        ads_real *rp;
        ads_real rv;
        long *ip;
        long iv;
        ads_point p1;
        struct resbuf *urb = NULL, *av;

        if ((ads_entnext(inspmv_en, inspmv_en) != RTNORM) ||
            ((urb = ads_entget(inspmv_en)) == NULL) ||
            ((av = resitem(urb, 1)) == NULL)) {
            ads_printf("INSPMV: Error fetching attribute for %s\n",
                dw->wname + 1);
            return False;
        }
        attval = av->resval.rstring;

        switch (vtype) {
            case E_string:
            case E_pointer:
                strcpy(insp_inst + offset, attval);
                break;

            case E_instance:
            case -E_instance:
                /* Just ignore instance nesting events. */
                break;

            case E_triple:
            case E_position:
            case E_displacement:
            case E_direction:
                sscanf(attval, "%lg,%lg,%lg", &p1[X], &p1[Y], &p1[Z]);
                rp = (ads_real *) (insp_inst + offset);
                for (i = X; i <= Z; i++) {
                    *rp++ = p1[i];
                }
                break;

            case E_real:
            case E_distance:
            case E_scalefactor:
                rp = (ads_real *) (insp_inst + offset);
                sscanf(attval, "%lg", &rv);
                *rp = rv;
                break;

            case E_integer:
                ip = (long *) (insp_inst + offset);
                sscanf(attval, "%ld", &iv);
                *ip = iv;
                break;
        }
        ads_relrb(urb);               /* Release attribute item */
    }
    return True;
}

Boolean inspect(inst, doclass)
  char *inst;
  Boolean doclass;
{
    ads_name ent, en;
    ads_point pt;
    classitem *ci = NULL;
    Boolean pickedok = False;

    insp_classvars = doclass;

    if (inst == NULL) {
        /* Pick class from AutoCAD. */
        if (ads_entsel(NULL, ent, pt) == RTNORM) {
            ci = entclass(ent, True);
        }
    } else {
        cinstvar *cv;

        Hpc(inst);
        cv = (cinstvar *) (inst - Civl);
        ci = (classitem *) (cv->ciclass->cd_classitem);
        Cname(ent, cv->ciename);
        memcpy(ci->classinstv - Civl, cv, ci->classinstvl + Civl);
        memcpy(ci->classinstv + ci->classinstvl, ci->classinstv,
            ci->classinstvl);
    }
    if (ci == NULL)
        return;

    /* O.K., we now know what class this object belongs to.
       Construct a block insertion we can edit with DDATTE
       and let the user modify the class variables. */

    defent("INSERT");
    tackint(66, 1);                   /* Attributes present */
    tackstring(2, classblock);        /* Block name */
    tackpoint(10, 0.0, 0.0, 0.0);     /* Block location */
    makent();

    defent("ATTRIB");
    tackpoint(10, 0.0, 0.0, 0.0);     /* Attribute location */
    tackreal(40, 0.2);                /* Text height */
    tackstring(1, ci->classname);     /* Initial value */
    tackstring(2, "CLASSNAME");       /* Attribute tag */
    tackint(70, 9);                   /* Attribute is invisible */
    makent();

    insprefix[0] = EOS;               /* Clear instance variable prefix */
    insphide = 0;                     /* Clear hidden object nesting */
    insp_inst = doclass ? ci->classv : ci->classinstv; /* Buffer address */
    procvar(ci, inspv);               /* Process its variables */

    defent("SEQEND");
    makent();

    if (ads_entlast(en) == RTNORM) {
        if (cls_edit(en, True)) {
            int whatch;
            ads_name es;
            struct resbuf *arb;

            /* User changed something.  Walk through the
               attributes and update the class instance
               variables in Xdata. */

            pickedok = True;
            Cname(es, en);
            while (True) {
                ads_entnext(es, es);
                arb = ads_entget(es);
                if (arb == NULL ||
                    ((strcmp(resitem(arb, 0)->resval.rstring,
                        "ATTRIB") == 0) &&
                     (strcmp(resitem(arb, 2)->resval.rstring,
                        "CLASSNAME") == 0)))
                    break;
                ads_relrb(arb);
            }
            ads_relrb(arb);
            Cname(inspmv_en, es);     /* Pass entity name */
            insphide = 0;             /* Clear hidden object nesting */
            /* Pass instance or class variable buffer. */
            insp_inst = doclass ? ci->classv : ci->classinstv;
            procvar(ci, inspmv);      /* Modify the variables */

            /* If we are working on an instance passed by a caller,
               unconditionally copy the temporary instance back to
               the caller's. */

            if (inst != NULL) {
                memcpy(inst, ci->classinstv, ci->classinstvl);
            }

            if (((whatch = whatchanged(ci)) & ChangedInstance) &&
                (ent[0] != 0)) {
                char *bname;

                /* The user modified one or more of the instance
                   variables so we need to update the entity in
                   the database.  What happens here is a little
                   subtle, so it's worth describing it in some
                   detail.  In order to preserve the handle of
                   the entity, we define a new anonymous block
                   for the geometry, then swap out the block name
                   in the block insertion entity that represents
                   the instance to use the new geometry block.
                   The old anonymous block will be deleted when
                   the drawing is next loaded. */

                bname = create_geometry(ci, ci->classinstv);
                modify_entity(ent);   /* Load entity for modification */
                if (bname != NULL) {
                    /* Swap to new geometry block */
                    free(resitem(rb, 2)->resval.rstring);
                    resitem(rb, 2)->resval.rstring = strsave(bname);
                }
                /* Tack instance variables onto entity */
                tack_instance(ci, ci->classinstv);
                if (ads_entmod(rb) != RTNORM) {
                    ads_printf("Error in ads_entmod after inspect.\n");
                }
                ads_relrb(rb);        /* Release modification result chain */
            }

            /* If class variables changed, update them in the database. */

            if (whatch & ChangedClass) {
                store_classvar(ci);
            }
        }
    }
    ads_entdel(en);                   /* Delete the inspect/edit block */
    return pickedok;
}

/*  Command-level interfaces to inspect.  The variants are for INSPECT
    and SPY operations on instance and class variables.  */

Boolean cls_inspect()
{
    spyring = False;
    return inspect(NULL, False);
}

Boolean cls_spy()
{
    spyring = True;
    return inspect(NULL, False);
}

Boolean cls_inspclass()
{
    spyring = False;
    return inspect(NULL, True);
}

Boolean cls_spyclass()
{
    spyring = True;
    return inspect(NULL, True);
}
