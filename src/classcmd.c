/*

    Execute methods invoked as AutoCAD commands

*/

#include "class.h"

/*  CLS_VCOMMAND  --  Execute a virtual function command.  */

void cls_vcommand(cindex)
  int cindex;
{
    cmditem *cm = (cmditem *) &cmdef;
    classitem *ci = (classitem *) &classq;

#ifdef DBSCAN
    ads_printf("CLS_VCOMMAND(%d)\n", cindex);
#endif

    /* Search the external command definition table for a command
       with this function index. */

    while ((cm = (cmditem *) qnext(cm, &cmdef)) != NULL) {
        if (cindex == cm->cm_defun_index) {
            ads_name sset;

            /* Found it!  Now select the entity on which we're to
               operate and run the method associated with this
               command with that entity as its object. */

            if (ads_ssget(NULL, NULL, NULL, NULL, sset) == RTNORM) {
                classitem *ci;
                long n, sslen;
                Boolean primwarn = False, argsok;
                struct queue csq;
                int totalargs = 0;    /* Total arguments in all classes */

                ads_sslength(sset, &sslen);

                /* The first thing we do is make a table summarising
                   all the classes that occur in the selection set. */

                qinit(&csq);          /* Initialise class summary queue */
                for (n = 0; n < sslen; n++) {
                    ads_name ent;

                    if (ads_ssname(sset, n, ent) != RTNORM)
                        break;

                    if ((ci = entclass(ent, False)) != NULL) {
                        classum *cs = (classum *) &csq;
                        vitem *vi;

                        while ((cs = (classum *) qnext(cs, &csq)) != NULL) {
                            if (ci == cs->clasci)
                                break;
                        }

                        /* Look up the actual virtual item descriptor
                           for this method and class.  We need this to
                           examine the argument requests made by this
                           message/object pair. */

                        vi = vf_search(((stackitem *) cm->cm_method) +
                                Dictwordl, (classdecl *) atl_body(
                                    ci->classhdr));

                        if (cs == NULL && vi != NULL) {
                            cs = (classum *) alloc(sizeof(classum) +
                                    (sizeof(clarg) *
                                     ((vi != NULL) ? (vi->vi_nargs - 1) : 0)));

                            cs->clasci = ci;
                            cs->clasvi = vi;
                            qinsert(&csq, &cs->clasql);
                            if (vi != NULL) {
                                int i;

                                totalargs += vi->vi_nargs;
                                for (i = 0; i < vi->vi_nargs; i++) {
                                    cs->clasarg[i].ca_nitems = 0;
                                    cs->clasarg[i].ca_source = NULL;
                                }
                            }
                        }
                    }
                }

                /* Obtain arguments needed by all methods we're
                   about to apply to objects. */

                argsok = arghhh(&csq, totalargs);

                /* Apply the appropriate method to each member
                   of the selection set. */

                for (n = 0; argsok && (n < sslen); n++) {
                    ads_name ent;

                    if (ads_ssname(sset, n, ent) != RTNORM)
                        break;

                    if ((ci = entclass(ent, True)) != NULL) {
                        char *bname;
                        int i, changed;
                        atl_int sfocus = focus;
                        char *sself = self;
                        classum *cs = (classum *) &csq;

                        while ((cs = (classum *) qnext(cs, &csq)) != NULL) {
                            if (ci == cs->clasci)
                                break;
                        }

                        /* Push the arguments assembled for this method
                           onto the stack before calling the method. */

                        if (cs != NULL) {
                            for (i = 0; i < cs->clasvi->vi_nargs; i++) {
                                stackitem *sc = cs->clasarg[i].ca_source;
                                int npush = cs->clasarg[i].ca_nitems;

                                assert(sc != NULL);
                                if (cs->clasvi->vi_modes & M_external) {
                                    ext_argstack(
                                      cs->clasvi->vi_arglist[i].argtype,
                                      (char *) sc, npush,
                                      cs->clasvi->vi_arglist[i].argkw != NULL);
                                } else {
                                    So(npush);
                                    while (npush > 0) {
                                        Push = *sc++;
                                        npush--;
                                    }
                                }
                            }
                        }

                        /* Pass the instance address to the method at the
                           top of the stack. */

                        focus = 0;
                        So(1);
                        Push = (stackitem) ci->classinstv;
                        self = ci->classinstv;  /* For PROTECTED */
                        callword(cm->cm_method);
                        self = sself;

                        changed = whatchanged(ci);
                        if (changed & ChangedInstance) {
                            bname = create_geometry(ci, ci->classinstv);
                            /* Load entity for modification */
                            modify_entity(ent);
                            if (bname != NULL) {
                                /* Swap to new geometry block */
                                free(resitem(rb, 2)->resval.rstring);
                                resitem(rb, 2)->resval.rstring =
                                    strsave(bname);
                            }
                            /* Tack instance variables onto entity */
                            tack_instance(ci, ci->classinstv);
                            if (ads_entmod(rb) != RTNORM) {
                                ads_printf(
                                    "Error in ads_entmod after method.\n");
                            }
                            /* Release modification result chain */
                            ads_relrb(rb);
                        }
                        if (changed & ChangedClass) {
                            store_classvar(ci);
                        }
                        focus = sfocus;
                    } else {
                        if (!primwarn) {
                            primwarn = True;
                            ads_printf(
                                "%s is not defined for primitive objects.\n",
                                cm->cm_method->wname + 1);
                        }
                    }
                }
                ads_ssfree(sset);
                /* Clear the class summary queue */
                while (!qempty(&csq))
                    free(qremove(&csq));
            }
            return;
        }
    }

    /* Search for an instance create command belonging to the
       class. */

    while ((ci = (classitem *) qnext(ci, &classq)) != NULL) {
        if (cindex == ci->cl_create_index) {
            atl_int sfocus = focus;

            /* Run the constructor on the temporary instance to set the
               initial default values for newly-created objects. */

            construct((classdecl *) atl_body(ci->classhdr),
                      (cinstvar *) (ci->classinstv - Civl));

            So(1);
            Push = (stackitem) ci->classinstv;
            focus = 0;
            if (atl_exec(ci->cl_acquisition) == ATL_SNORM) {
                stackitem acqok;

                Sl(1);
                acqok = S0;
                Pop;
                if (acqok != 0) {
                    char *bname = create_geometry(ci, ci->classinstv);

                    if (bname != NULL) {
                        ads_name insname;
                        cinstvar *cv = (cinstvar *) (ci->classinstv - Civl);

                        blockdef(bname);

                        tackrb(-3);   /* Start of application data */
                        /* Application name */
                        tackstring(Xed(1), strsave(ourapp));
                        /* Class name */
                        tackstring(Xed(0), strsave(ci->classname));

                        /* Now tack on the extended entity data items that
                           represent the instance variables. */

                        tack_instance(ci, ci->classinstv);
                        makent();
                        ads_entlast(insname);
                        ads_entlast(cv->ciename);
#define ZZZ
#ifdef ZZZ
                        CommandB();
                        ads_command(RTSTR, "Move", RTENAME, insname,
                            RTSTR, "", RTSTR, "*0,0,0", PAUSE, RTNONE);
                        CommandE();
#endif
                    }
                    /* Acquisition may have changed class variables. */
                    if (whatchanged(ci) & ChangedClass) {
                        store_classvar(ci);
                    }
                }
            }
            focus = sfocus;
            break;
        }
    }
}
