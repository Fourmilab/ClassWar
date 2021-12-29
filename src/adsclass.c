/*
#define CLASSTOREDEBUG
#define PVDEBUG
#define INHDEBUG
#define ECVARDEBUG
#define CLASSVARDEBUG
#define CONSTRDEBUG
#define TACKINSTANCEDEBUG
#define VDEFDEBUG
*/
/* #define DBSCAN    */                     /* Debug output on endclass scan */
#define SPY
/*

    Classes and objects for ADS applications.

    Designed and implemented in February of 1990 by John Walker

*/

#include "class.h"

/*  Configuration  */

#define DEBRIS                        /* Check for debris left on the stack */

/*  Altas interface words.  */

#define ClassStartDraw  "CLASS.STARTDRAW" /* Start drawing prologue */
#define ClassEndDraw    "CLASS.ENDDRAW"  /* End drawing epilogue */
#define ClassInstanceDecl "CLASS.INSTANCE.DECL" /*Create instance declarator */
#define ThisPush        "THIS"        /* Push current object */
#define SelfName        "OBJECT.SELF" /* Current instance */
#ifdef DEBRIS
#define ClassDebris     "CLASS.DEBRIS-CHECK" /* Debris checking word */
#endif
#define CreationName    "ACQUIRE"     /* Name of entity create definition */
#define ConstructorName "NEW"         /* Name of instance constructor */
#define NewClassName    "NEWCLASS"    /* Name of class constructor */

#define ClassSentinel   0x13508F1D    /* Class header sentinel */

/*  Data structures  */

typedef dictword *afunc;              /* ATLAS word definition */

/*  Variables shared with the ATLAS program.  */

static shared v_classtrace;           /* Trace invocation of class functions */
shared v_focus;                       /* Current object focus or zero */
shared v_drawcolour;                  /* Current drawing colour */
shared v_wireframe;                   /* Wireframe generation mode */
double *v_stellation;                 /* Polygon stellation */
shared v_cnsegs;                      /* Circle number of segments */
char *v_drawltype;                    /* Current drawing line type */
static dictword *startdraw;           /* Start drawing definitions */
static dictword *enddraw;             /* End drawing definitions */
static dictword *instance_decl;       /* Create instance declaring word */
static dictword *thispush;            /* Push current instance address */
char **object_self;                   /* Current method's instance */
#ifdef DEBRIS
static dictword *debris;              /* Check for stack debris */
#endif

#define         classtrace  *v_classtrace

/*  Functions accessed in the ATLAS program.  NULL indicates that
    the program does not define this function.  */

/*  Local variables  */

static int defun_index = 10000;       /* AutoCAD function linkage index */

/*  Global variables exported  */

char *ourapp;                         /* Our application name */
char classblock[40];                  /* Class block name */
vitem *extlist = NULL;                /* External method list */
Boolean spyring;                      /* Allow INSPECT of private and
                                         protected variables. */
tacky();                              /* Result buffer chain cells */
struct queue classq = {&classq, &classq}; /* Class list */
struct queue cmdef = {&cmdef, &cmdef};    /* External command table */
struct queue argq = {&argq, &argq};       /* Command argument list */
struct queue *draw_message;           /* Virtual draw method */
classitem *md_classitem;              /* Classitem for compilation underway */
struct resbuf applist;                /* Application list for ads_entgetx() */

/*  Forward functions  */


/*  CALLWORD  --  Call an ATLAS word, tracing its invocation if
                  requested.  */

int callword(wd)
  dictword *wd;
{
    int stat;
dictword *scurword = curword, *screateword = createword;


createword = curword = NULL;
    if (classtrace)
        V ads_printf("Invoking %s\n", wd->wname + 1);
    if ((stat = atl_exec(wd)) != ATL_SNORM) {
        V ads_printf("\nError %d in %s function.\n", stat,
            wd->wname + 1);
    }
#ifdef DEBRIS
      else {
        if (debris != NULL) {
            atl_exec(debris);
        }
    }
#endif
curword = scurword;
createword = screateword;
    return stat;
}

/*  CALLVIRT  --  Take the appropriate action for a virtual method,
                  either running the internal word or invoking an
                  external method defined in another application.  */

int callvirt(vi)
  vitem *vi;
{
    int estat;

    if (vi->vi_modes & M_external) {
        estat = call_external(vi);
    } else {
        estat = callword(vi->vi_word_to_run);
    }
    return estat;
}

/*  WHATCHANGED  --  Examines a class definition and determines if
                     the class and/or instance variables have changed.
                     Returns the logical OR of ChangedClass and
                     ChangedInstance if the class and/or instance
                     variables have changed.  */

int whatchanged(ci)
  classitem *ci;
{
    return (((ci->classinstvl > 0) &&
             (memcmp(ci->classinstv, ci->classinstv + ci->classinstvl,
                     ci->classinstvl) != 0)) ? ChangedInstance : 0) |
           (((ci->classvl > 0) &&
             (memcmp(ci->classv, ci->classv + ci->classvl,
                     ci->classvl) != 0)) ? ChangedClass : 0);
}

/*  PROCVAR  --  Process variables within a class definition.  Given
                 the class definition, calls a user-defined function
                 to process each variable.  */

static void iprocvar(cc, vfunc, cip, ccp, nesting, nflags, recurse)
  classitem *cc;
  Boolean (*vfunc)();
  long cip, ccp;
  int nesting, nflags;
  Boolean recurse;
{
    dictword *cl = cc->classhdr;
    classdecl *cd = (classdecl *) atl_body(cl);

    if (cd->cd_sentinel != ClassSentinel) {
        atl_error("PROCVAR: Invalid sentinel in class header");
        return;
    } else {
        int inum = cd->cd_ivcount,    /* Class variable count */
            i;
        stackitem *si = &(cd->cd_vartab); /* Start of variable table */
        dictword *dw;
        dictword *dl = cl;            /* Last dictionary item */

        /* Loop through the variable descriptors.  For each
           call the user function, passing the properties of
           the variable. */

        for (i = 0; i < inum; i++) {
            int vlen, vtype, vflags, eflags;
            long clip = -1, clcp = -1, pcip, pccp;

            dw = (dictword *) si;     /* Next item is dictionary entry */
            if (dw->wnext != dl)      /* If word involves DOES> */
                dw = (dictword *) (si + 1); /* Skip past its DOES> pointer */
            dl = dw;
            si = atl_body(dw);        /* Advance to its body */
            vlen = (*si) >> 16;       /* Length of storage item */
            vtype = ((*si) >> 8) & 0xFF; /* Variable type */
            vflags = (*si) & 0xFF;    /* Variable flags */
#ifdef PVDEBUG
            {   int i;

                for (i = 0; i < nesting; i++)
                    ads_printf("  ");
                ads_printf(
                   "Process variable %s: Len %d, Type %d, Flags: %d\n",
                   dw->wname + 1, vlen, vtype, vflags);
            }
#endif

            /* Determine the "effective storage flags" of this variable.
               If the variable is a primitive member of the class, its
               effective flags are just those declared for it.  If the
               variable is a member of a nested instance of another class,
               then its effective flags are determined as follows:

                    The storage type (Instance, Class, or Temporary)
                    is that of the outermost object in the nest.

                    The storage accessibility modes (Public, Protected,
                    or Private) are the most restrictive of any
                    variable in the nested sequence of instances.

               Further, if this is a member of a nested instance, process
               it only if the member is an instance variable.  Class and
               temporaries are ignored for nested instances. */

            if (nesting == 0) {
                eflags = vflags;
            } else {
#define S_Privacy   (S_private | S_protected | S_public)
                if ((vflags & (F_class | S_temp)) != 0)
                    eflags = S_temp | S_private;
                else
                    eflags = (nflags & (F_class | S_temp)) |
                        min(nflags & S_Privacy, vflags & S_Privacy);
#undef S_Privacy
            }

            /* Determine offset of this variable (if a class or instance
               variable) and update the running offset counter for the
               type of variable we're processing. */

            pcip = cip;
            pccp = ccp;
            if (!(eflags & S_temp)) {
                if (eflags & F_class) {
                    clcp = ccp;
                    ccp += vlen;
                } else {
                    clip = cip;
                    cip += vlen;
                }
            }

            /* If this variable is an instance of another class,
               load the pointer to the class declaration that follows
               the variable type word (and offset if this is a class
               or instance variable). */

            if (vtype == E_instance) {
                classdecl *vclass = (classdecl *)
                                      si[(vflags & S_temp) ? 1 : 2];

#ifdef PVDEBUG
                {   int i;

                    for (i = 0; i < nesting; i++)
                        ads_printf("  ");
                    Hpc(vclass);
                    ads_printf("  Nested instance of class %s\n",
                        vclass->cd_sentinel != ClassSentinel ?
                        "<Bad sentinel>" :
                        ((classitem *) vclass->cd_classitem)->classname);
                }
#endif
                /* First, call the processing function to let it know
                   we're about to recurse into an nested instance. */

                if (!(*vfunc)(cc, dw, vtype, eflags, vlen,
                                  clip, clcp, vclass))
                    break;

                if (recurse) {

                    /* Invoke ourselves recursively to process variables
                       within this instance. */

                    iprocvar((classitem *) vclass->cd_classitem, vfunc,
                             pcip < 0 ? -1 : pcip + Civl,
                             pccp < 0 ? -1 : pccp + Civl,
                             nesting + 1, nesting == 0 ?
                             vflags : eflags, recurse);

                    /* Finally, call the processing function with a type
                       of -E_instance to inform it we've popped after
                       passing the variables of the nested instance. */
                    if (!(*vfunc)(cc, dw, -vtype, eflags, vlen,
                                      clip, clcp, vclass))
                        break;
                }
            } else {

                /* Process primitive variable.

                   Call the user variable processing function with arguments:

                        Class definition item
                        Current variable dictionary entry
                        Variable type (XDATA code - 1000)
                        Effective variable storage type flags
                        Variable length in bytes
                        Instance variable offset within instance storage,
                            or -1 if the variable is static or a class
                            variable.
                        Class variable offset within class storage, or -1
                            if the variable is static or an instance
                            variable.
                        Class declaration pointer if recursing into a
                            nested instance (NULL for primitive
                            variables).

                   Note that if we've been recursively called to walk
                   through the variables of a nested instance, we
                   totally ignore all class and temporary variables of
                   the nested instance.  When an instance is nested, only
                   its instance variables are relevant.  The instance
                   variables of the nested instance assume the storage
                   modes of the outermost variable.
                */

                if ((nesting == 0) || (!(vflags & (S_temp | F_class)))) {
                    if (!(*vfunc)(cc, dw, vtype, eflags,
                                  vlen, clip, clcp, NULL))
                        break;
                }
            }

            /* Skip past mode word, offset (if an instance or class
               variable), and static storage (if a temporary
               variable).  Note that if this is a nested instance we
               must also skip the class declaration pointer which
               follows the offset word.  */

            si += 1 + ((vflags & S_temp) ? (vlen / 4) :
                  (vtype == E_instance ? 2 : 1));
        }
    }
}

void procvar(cc, vfunc)
  classitem *cc;
  Boolean (*vfunc)();
{
    iprocvar(cc, vfunc, 0L, 0L, 0, 0, True);
}

/*  CONSTRUCT  --  Construct an instance of an object.  We initially
                   clear the instance to zero, then place its class
                   pointer at the head of the instance.  Then we
                   walk down the instance variables, initialising
                   each.  Primitives are simply set to zero, while
                   nested class instances generate a recursive
                   call on CONSTRUCT to initialise them.  Finally,
                   if a constructor definition exists for this
                   class, run it with the instance as its argument.  */

static char *constr_inst;             /* Instance being constructed */
static int constr_parent;             /* Parent class's instance length */

static Boolean constrv(cc, dw, vtype, vflags, vlen, clip, clcp, vclass)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
  classdecl *vclass;
{
#ifdef CONSTRDEBUG
ads_printf(
"Construct instance: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif
    /* Process only nested instances with offsets in the class variable
       buffer greater than the parent class's instance variable length.
       If there is no parent class, constr_parent is 0 and all nested
       instance variables are processed. */
    if ((clip >= constr_parent) && (vtype == E_instance)) {
        char *scinst = constr_inst;   /* Stack instance being constructed */
        int sparent = constr_parent;

        construct(vclass, constr_inst + clip);
        constr_inst = scinst;         /* Unstack instance being constructed */
        constr_parent = sparent;
    }

    return True;
}

void construct(cd, cv)
  classdecl *cd;
  cinstvar *cv;
{
    classitem *ci = (classitem *) cd->cd_classitem;

    /* If this class was derived from another, recurse to call the
       constructor for the parent class before we perform any
       special processing at this class level. */

    if (cd->cd_parent != 0) {
        construct((classdecl *) cd->cd_parent, cv);
    }

    /* One more little trick if the class was derived.  Since at
       this point we've already performed the initialisation
       of all the derived variables by recursive calls to ourself,
       and some parent class may have had a constructor that's
       been run to initialise a variable, we don't want to re-do
       the default construction for these inherited variables.
       We prevent this by comparing the offset in the instance
       variables with the length of the parent class's instance
       variables.  Only if the offset is greater do we construct
       the variable at this level, as that condition denotes a
       variable declared explicitly in this class rather than
       having been inherited. */

    constr_parent = (cd->cd_parent == 0) ? 0 :
        ((classdecl *) cd->cd_parent)->cd_ivlength;
    constr_inst = ((char *) cv) + Civl; /* Instance variable base */

    /* Clear instance buffer */
    memset(constr_inst + constr_parent, 0, ci->classinstvl - constr_parent);
    cv->ciclass = cd;                 /* Point instance to class */
    memset((char *) cv->ciename, 0, sizeof(ads_name));
    iprocvar(ci, constrv, 0L, 0L, 0, 0, False);  /* Construct variables */

    /* Now, if a constructor is defined for this class, execute it,
       passing the instance address on the stack. */

    if (ci->cl_constructor != NULL) {
        atl_int sfocus = focus;

        So(1);
        focus = 0;
        Push = (stackitem) constr_inst;
        /* We use atl_exec() rather than callword() to invoke the
           constructor because callword() may perform the DEBRIS
           check.  Since we can get here from the (construct)
           primitive at any stack context, DEBRIS would report an
           error. */
        V atl_exec(ci->cl_constructor);
        focus = sfocus;
    }
}

/*  ENTCLASS  --  Given an entity name, determine which class it
                  belongs to and return the classitem describing
                  its parent class.  The instance variables are
                  loaded from the entity's Xdata fields.  If the
                  entity is not a class-defined entity, or an
                  error occurs attempting to load it, NULL is
                  returned.  */

static char *entclv_inst;             /* Top-level class instance pointer */

static Boolean entclv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
#ifdef ENTCLASSDEBUG
ads_printf(
"Load instance: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif

    if (clip >= 0) {                  /* Only process instance variables */

        if ((ri != NULL) && (abs(vtype) != E_instance)) {

            /* Walk along the Xdata chain to the next variable.  Verify
               that the Xdata item type matches the type of the variable
               we expect to load. */

            if (Xed(vtype) != ri->restype) {
                ads_printf("Instance variable %s mismatch:\n", dw->wname + 1);
                ads_printf("  Expected type %d, received type %d.\n",
                    Xed(vtype), ri->restype);
                return False;
            }
            if (vtype <= E_pointer) {
                strcpy(entclv_inst + clip, ri->resval.rstring);
            } else {
                memcpy(entclv_inst + clip,
                       (char *) &(ri->resval.rint), vlen);
            }
            ri = ri->rbnext;
        }
    }
    return True;
}

classitem *entclass(ent, loadvars)
  ads_name ent;
  Boolean loadvars;
{
    struct resbuf *urb = ads_entget(ent);
    struct resbuf *uri = urb;
    classitem *ci = NULL;

    while ((uri = resitem(uri, -3)) != NULL) {
        uri = uri->rbnext;
        if ((uri != NULL) && (uri->restype == Xed(1)) &&
            (strcmp(uri->resval.rstring, ourapp) == 0))
            break;
    }
    if (uri != NULL) {
        char *cname = NULL;
        cinstvar *cv;

        if ((uri = resitem(uri, Xed(0))) != NULL)
            cname = uri->resval.rstring;
        if (cname == NULL) {
            /* Must be somebody's else anonymous block. */
#ifdef DBSCAN
            ads_printf("ENTCLASS: Class name missing in entity.\n");
#endif
            return NULL;
        }
        ci = cls_lookup(cname);
        if (ci == NULL) {
            ads_printf("Unknown class, %s.\n", cname);
            return NULL;
        }

        if (ci->classhdr == NULL) {
            ads_printf("Error in %s class definition.\n", cname);
            ads_printf(
          "Cannot edit entities of that type until the error is corrected.\n");
            return NULL;
        }

        /* Store the AutoCAD entity name in the header of the instance
           variable item. */

        cv = (cinstvar *) (((char *) (ci->classinstv)) - Civl);
        construct((classdecl *) atl_body(ci->classhdr), cv);
        Cname(cv->ciename, ent);

        /* Now load the instance variables.  */

        if (loadvars) {
            if (ci->classinstvl > 0) {
                ri = uri->rbnext;
                /* Pass top-level instance addr */
                entclv_inst = ci->classinstv;
                procvar(ci, entclv);

                /* Now copy the values loaded for the instance variables to the
                   shadow copy of the instance variables.  This is used to
                   determine if a method changed any instance variable which
                   might require updating the entity database.  */

                memcpy(ci->classinstv + ci->classinstvl, ci->classinstv,
                    ci->classinstvl);
            }

        }
    }

    if (urb != NULL) {
        ads_relrb(urb);
    }
    return ci;
}

/*  TACK_INSTANCE  --  Tack instance variables onto the end of the
                       current static result buffer chain.  */

static char *tackib;                  /* Hidden argument to tackiv(). */

static Boolean tackiv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
#ifdef TACKINSTANCEDEBUG
    ads_printf(
"Tack instance: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld, clcp %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip, clcp);
#endif

    if ((clip >= 0) && (abs(vtype) != E_instance)) {

        /* Build the XDATA item.  Note that we assume that the
           internal Atlas data type is the same as the result buffer
           type, allowing us to just copy the bytes rather than have a
           special case for each data type.  */

        tackrb(Xed(vtype));
        if (vtype <= E_pointer) {
            ri->resval.rstring = strsave(tackib + clip);
        } else {
            memcpy((char *) &(ri->resval.rint), tackib + clip, vlen);
        }
    }

    return True;
}

void tack_instance(ci, instbuf)
  classitem *ci;
  char *instbuf;
{
    tackib = instbuf;                 /* Pass tackiv() instance buffer addr */
    procvar(ci, tackiv);
}

/*  MODIFY_ENTITY  --  Load a class-defined entity into memory
                       to allow modification of its instance variable
                       Xdata items.  */

void modify_entity(ent)
  ads_name ent;
{
    rb = ads_entget(ent);
    ri = rb;
    rbtail = NULL;
    while ((ri = resitem(ri, -3)) != NULL) {
        rbtail = ri;
        ri = ri->rbnext;
        if ((ri != NULL) && (ri->restype == Xed(1)) &&
            (strcmp(ri->resval.rstring, ourapp) == 0)) {
            rbtail = ri;
            ri = ri->rbnext;
            break;
        }
    }
    if (rbtail == NULL) {
        ri = rb;
        while (ri->rbnext != NULL)
            ri = ri->rbnext;
        rbtail = ri;
        ri = NULL;
    }
    if (ri != NULL) {
        rbtail = ri;
        ads_relrb(rbtail->rbnext);    /* Release tail subchain */
        rbtail->rbnext = NULL;        /* Mark this as end of chain */
    }
    ri = rb;
}

/*  CREATE_GEOMETRY  --  Create the AutoCAD entities that define the
                         geometry associated with an instance.  The
                         geometry is collected into an anonymous block,
                         the name of which is returned to the caller. */

char *create_geometry(ci, instance)
  classitem *ci;
  char *instance;
{
    static char bname[32];
    int i;

    V ads_entmake(NULL);              /* Wipe out any entity underway */
    defent("BLOCK");                  /* Define block entity */
    tackstring(2, "*U");              /* Make an anonymous block */
    tackint(70, 1);                   /* Set anonymous bit */
    tackpoint(10, 0.0, 0.0, 0.0);     /* Block origin 0,0,0 */
    makent();                         /* Begin block definition */

    /* If a geometry creation method exists for this class,
       invoke it. */

    if (draw_message != NULL) {
        atl_int sfocus = focus;
        vitem *vi = vf_search(draw_message,
                        (classdecl *) atl_body(ci->classhdr));

        if (vi != NULL && vi->vi_word_to_run != NULL) {
            drawcolour = 0;           /* Set default colour to BYBLOCK */
            strcpy(drawltype, "BYBLOCK"); /* Linetype also governed by block */
            if (startdraw != NULL) {  /* If drawing prologue is defined */
                atl_exec(startdraw);  /* Run it first. */
            }
            So(1);
            Push = (stackitem) instance;  /* Pass instance to DRAW method. */
            focus = 0;
            if (callvirt(vi) != ATL_SNORM) {
                if (enddraw != NULL)
                    atl_exec(enddraw);/* Run draw epilogue */
                V ads_entmake(NULL);  /* Abort entity being generated */
                return NULL;          /* Return error status */
            }
            if (enddraw != NULL) {    /* If drawing epilogue is defined... */
                atl_exec(enddraw);    /* ...run it. */
            }
            focus = sfocus;
        }
    }

    defent("ENDBLK");
    i = ads_entmake(rb);
    if (i == RTKWORD) {
        ads_getinput(bname);
        return bname;
    } else {
        ads_printf("Error in %s class entity creation.\n",
            ci->classname);
    }
    return NULL;
}

/*  CONSTRUCT_CLASSVAR  --  Initialise the class variables by calling
                            the class variable constructor, if present.
                            Any variables not set by the constructor
                            are initialised to zero.  */

static void construct_classvar(bci, ci, cvars)
  classitem *bci, *ci;
  char *cvars;
{
    classdecl *pd = (classdecl *)
                        ((classdecl *) atl_body(ci->classhdr))->cd_parent;
    int parcvlen = (pd == NULL) ? 0 : pd->cd_cvlength;

    if (ci->classvl > 0) {

        /* If this class was derived from another, call the parent class's
           constructor to initialise the class variables it defined. */

        if (pd != NULL) {
            construct_classvar(bci, (classitem *) pd->cd_classitem, cvars);
        }

        /* Initially clear the class variable buffer to zero.  This
           guarantees that any new class variables not defined by the
           existing class variables stored in the drawing and not
           initialised by a NEWCLASS method are cleared to zero. */

        memset(cvars + parcvlen, 0, ci->classvl - parcvlen);

        /* If this class has a NEWCLASS method, run it.  The NEWCLASS word
           is normally used to initialise class variables.  Since the
           class variable buffer is not allocated until (endclass) time,
           they cannot be assigned at compile time, hence NEWCLASS. */

        if (ci->cl_newclass != NULL) {
            atl_int sfocus = focus;

            So(1);
            /* The next line looks wrong, but it's really doing the right
               thing.  Recall that all methods expect an INSTANCE pointer
               on the top of the stack when they run.  If a class variable
               is referenced, it is found via the instance variable.  The
               NEWCLASS method (which presumably sets only class variables)
               is no exception.  So, we pass the temporary instance of the
               outermost class being constructed, since the method prologue
               will find the correct class variable buffer from it. */
            Push = (stackitem) bci->classinstv;
            focus = 0;
            atl_exec(ci->cl_newclass);
            focus = sfocus;
        }
    }
}

/*  LOAD_CLASSVAR  --  Load class variable values for this class.
                       Returns True if the class variables were
                       loaded and False if no class variable definitions
                       exist (as for an entity just created for a
                       newly-defined class). */

static char *classv_buf;              /* Top level class variable buffer */
static int classv_nest;               /* Instance nesting level */

static Boolean lclasv(cc, dw, vtype, vflags, vlen, clip, clcp, vclass)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
  classdecl *vclass;
{
#ifdef CLASSVARDEBUG
ads_printf(
"Load classvar: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clcp %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clcp);
#endif

    if (clcp >= 0) {                  /* Only process class variables */

        /* If this is the beginning of a nested instance, call the
           constructor for the class of that instance.  This plugs
           the class type and initialises the fields of the instance
           to their defaults.  We need to do this to guarantee that
           newly-declared classes or variables added to classes are
           initialised to the instance constructor-defined values. */

        if (vtype == E_instance) {
            /* Since construct() initialises arbitrarily deeply nested
               structures of instances, we need to call it only for the
               outermost level of instances that occur within the class
               variables.  Its own recursion will take care of the deeper
               levels. */
            if (classv_nest == 0)
                construct(vclass, (cinstvar *) (classv_buf + clcp));
            classv_nest++;
        } else if (vtype == -E_instance) {
            classv_nest--;
        }

        if ((ri != NULL) && (abs(vtype) != E_instance)) {

            /* Walk along the Xdata chain to the next variable.  Verify
               that the Xdata item type matches the type of the variable
               we expect to load. */

            if (Xed(vtype) != ri->restype) {
                ads_printf("Class variable %s mismatch:\n", dw->wname + 1);
                ads_printf("  Expected type %d, received type %d.\n",
                    Xed(vtype), ri->restype);
                return False;
            }
            if (vtype == E_pointer) {
                strcpy(classv_buf + clcp, ri->resval.rstring);
            } else {
                memcpy(classv_buf + clcp,
                       (char *) &(ri->resval.rint), vlen);
            }
            ri = ri->rbnext;
        }
    }
    return True;
}

static void load_classvar(ci)
  classitem *ci;
{
    struct resbuf *urb, *uri;

    if (ci->classvl == 0)
        return;

    /* Call the class variable constructor to initialise the class
       variable before we attempt to load them from the database.
       This guarantees that any newly-added class variables are
       initialised to their defaults as well as setting the class
       variables for a newly-defined class. */

    construct_classvar(ci, ci, ci->classv);

    /* Now walk through the entity data attached to the class definition
       block and initialise the class variables from the entity data. */

    uri = urb = ads_entget(ci->classent);
    while ((uri = resitem(uri, -3)) != NULL) {
        uri = uri->rbnext;
        if ((uri != NULL) && (uri->restype == Xed(1)) &&
            (strcmp(uri->resval.rstring, ourapp) == 0))
            break;
    }
    ri = (uri != NULL) ? uri->rbnext : NULL;
    classv_buf = ci->classv;          /* Set pointer to outermost class vars */
    classv_nest = 0;                  /* Clear instance nesting level */
    procvar(ci, lclasv);
    if (urb != NULL) {
        ads_relrb(urb);
    }

    /* Initialise the shadow copy of the class variables.  This copy
       is used by whatchanged() to determine whether any class
       variables have changed, requiring the class variables to be
       updated in the database. */

    memcpy(ci->classv + ci->classvl, ci->classv, ci->classvl);
}

/*  TACK_CLASSVAR  --  Attach extended entity data items to the current
                       result buffer chain that encode the class variables
                       for a given class.  */

static Boolean tclassv(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
    if ((clcp >= 0) && (abs(vtype) != E_instance)) {
#ifdef STORECLASSVDEBUG
    ads_printf(
"Store classvar: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld clcp %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip, clcp);
#endif

        /* Build the XDATA item.  Note that we assume that the
           internal Atlas data type is the same as the result buffer
           type, allowing us to just copy the bytes rather than have a
           special case for each data type.  */

        tackrb(Xed(vtype));
        if (vtype <= E_pointer) {
            ri->resval.rstring = strsave(classv_buf + clcp);
        } else {
            memcpy((char *) &(ri->resval.rint), classv_buf + clcp, vlen);
        }
    }
    return True;
}

void tack_classvar(ci)
  classitem *ci;
{
    classv_buf = ci->classv;          /* Set pointer to outermost class vars */
    procvar(ci, tclassv);
}

/*  STORE_CLASSVAR  --  Store class variables as entity attributes
                        on the class definition block insertion.  */

void store_classvar(ci)
  classitem *ci;
{

    /* Load the class definition entity into memory. */

    rb = ads_entget(ci->classent);
    ri = rb;
    rbtail = NULL;

    /* Scan the class definition entity for the start of the
       extended entity data that holds stores the class
       variables.  */

    while ((ri = resitem(ri, -3)) != NULL) {
        rbtail = ri;
        ri = ri->rbnext;
        if ((ri != NULL) && (ri->restype == Xed(1)) &&
            (strcmp(ri->resval.rstring, ourapp) == 0)) {
            rbtail = ri;
            ads_relrb(rbtail->rbnext);/* Release tail subchain */
            rbtail->rbnext = NULL;    /* Mark this as end of chain */
            break;
        }
    }

    /* If the class definition contains no extended entity data (as
       is the case for a newly-defined class), append the application
       data header and prepare to tack the class variables after
       it. */

    if (rbtail == NULL) {
        ri = rb;
        while (ri->rbnext != NULL)
            ri = ri->rbnext;
        rbtail = ri;
        tackrb(-3);                   /* Attach extended data sentinel */
        tackstring(Xed(1), strsave(ourapp)); /* Application name */
    }
    tack_classvar(ci);                /* Attach extended data for class vars */
    if (ads_entmod(rb) != RTNORM) {
        ads_printf("STORE_CLASSVAR: Error in ads_entmod.\n");
    }
    ads_relrb(rb);

    /* Refresh the shadow copy of the class variables to reflect the
       new values stored in the database. */

    memcpy(ci->classv + ci->classvl, ci->classv, ci->classvl);
#ifdef CLASSTOREDEBUG
    ads_printf("Stored class variables for %s.\n", ci->classname);
#endif
}

/*  SAVE_CLASSVARS  --  Store all class variables of all classes that
                        have class variables back in the database.  */

void save_classvars()
{
    classitem *ci = (classitem *) &classq;

    while ((ci = (classitem *) qnext(ci, &classq)) != NULL) {
        if (whatchanged(ci) & ChangedClass) {
            store_classvar(ci);
        }
    }
}

/*  VF_LOOKUP  --  Given a virtual method definition word and a class
                   declaration, return the virtual method definition
                   item describing the action for this method sent to
                   this class.  If no method of this type is defined
                   for this class, return NULL.  */

static vitem *vf_lookup(vfq, cd)
  struct queue *vfq;
  classdecl *cd;
{
    vitem *vi = (vitem *) vfq;
    while ((vi = (vitem *) qnext(vi, vfq)) != NULL) {
        if (cd == vi->vi_class)
            break;
    }
    return vi;
}

/*  VF_SEARCH  --  Search for a method of a given type within a
                   class specified by its declaration or in any
                   parent class whose methods were inherited through
                   public derivation.  Returns the VITEM for the
                   method or NULL if no method exists for objects of
                   this class.  */

vitem *vf_search(vfq, cd)
  struct queue *vfq;
  classdecl *cd;
{
    vitem *vi = NULL;

    while (cd != NULL) {
        vi = vf_lookup(vfq, cd);
        if (vi != NULL) {

            /* Verify that the method we're about to return did
               not appear inside a class that failed to compile.  If so,
               say it doesn't exist.  */

            if (vi->vi_classitem == NULL ||
                vi->vi_classitem->classhdr == NULL)
                vi = NULL;
            break;
        }

        /* We failed to find a method in the current class.  If the
           class publicly inherited the methods of its parent class,
           walk up the inheritance chain to the parent and look
           there. */

        if ((cd->cd_classmodes & C_dpublic) == 0) {
            break;
        }
        cd = (classdecl *) cd->cd_parent;
    }
    return vi;
}

/*  (VFEXEC)  --  Execute virtual method.  When this primitive is called,
                  we establish, from the top of the stack or by the
                  focus, which class the recipient of the message
                  belongs to, and invoke the method defined for this
                  message within that class.

                  If the class we're trying to send the message to was
                  derived from another class with the PUBLIC attribute
                  (specifying that the parent's methods operate on the
                  derived class as well), if we fail to find the requested
                  method in the current class we walk up the chain of
                  inheritance until we either find the message or fail
                  by hitting the top of the chain or encountering a
                  derivation without the PUBLIC attribute.

IMPORTANT!! ===>  Resolving inheritance of public method inheritance at
                  runtime as described in the paragraph above is simple
                  and minimises memory consumption, but it is INCREDIBLY
                  INEFFICIENT OF CPU TIME, particularly when there are
                  many levels of inheritance and many methods with the
                  same name.  To speed this up, we should really invert
                  the VITEM table and generate, for each class, a private
                  table attached to its CLASSITEM that directly maps
                  the unique method index into a word_to_run or
                  field descriptor.  This would reduce P_vfexec()
                  into a table look-up.  THIS SHOULD BE DONE AFTER THE SYSTEM
                  IS UP AND RUNNING RELIABLY.  I'm not going to do it at
                  this time because the structure of the VITEM is still
                  changing rapidly and I don't want to have to rewrite
                  the table inverting code every time I change my mind about
                  how inheritance should work.  */

void P_vfexec()
{
    cinstvar *cv;
    classdecl *cd;
    vitem *vi = NULL;

#ifdef VFEXEC_DEBUG
ads_printf("Execute virtual method %s", curword->wname + 1);
#endif
    if (focus != 0) {
        cv = (cinstvar *) (focus - Civl);
    } else {
        Sl(1);
        cv = (cinstvar *) (S0 - Civl);
    }
    Hpc(cv);
    cd = cv->ciclass;
    Hpc(cd);
#ifdef VFEXEC_DEBUG
ads_printf(" for class %s.\n",
   ((classitem *) cd->cd_classitem)->classname);
#endif
    vi = vf_search((struct queue *) (((stackitem *) curword) + Dictwordl), cd);
    if (vi != NULL) {

        /* If this method was declared with the PROTECTED attribute
           in its home class, verify that we are executing a method
           derived from the class containing the method.  Otherwise,
           reject the attempt to use a PROTECTED method in other
           class code. */

        if (vi->vi_modes & S_protected) {
            classdecl *mc;

            mc = (self == NULL) ? NULL :
                ((cinstvar *) (self - Civl))->ciclass;
            while (mc != NULL) {
                if (mc == vi->vi_class)
                    break;
                mc = (classdecl *) mc->cd_parent;
            }
            if (mc == NULL) {
                char goof[80];

                sprintf(goof, "Method %s not defined for class %s (protected)",
                    curword->wname + 1,
                    ((classitem *) cd->cd_classitem)->classname);
                if (ip == NULL) {
                    ads_printf("%s.\n", goof);
                    Pop;
                } else {
                    atl_error(goof);
                }
                return;
            }
        }

        /* Access to the method is granted.  Perform the action for
           the message. */

        if (vi->vi_word_to_run != NULL) {
            if (vi->vi_modes & M_external)
                V callvirt(vi);
            else
                V atl_exec(vi->vi_word_to_run);
        } else if (vi->vi_offset != 0) {
            classitem *ci = (classitem *) cd->cd_classitem;

            /* This is a field access method.  Rather than running
               code, we just compute the field address of the
               instance (if the offset is positive) or class (if
               negative) variable denoted by this message. */

            if (focus != 0) {
                So(1);
                Push = focus;
            }
            if (vi->vi_offset > 0)
                S0 += vi->vi_offset - 1; /* Instance variable reference */
            else                      /* Class variable reference */
                S0 = ((stackitem) ci->classv) - (vi->vi_offset + 1);
        }
    } else {
        char goof[80];

        sprintf(goof, "Method %s not defined for class %s",
            curword->wname + 1, ((classitem *) cd->cd_classitem)->classname);
        if (ip == NULL) {
            ads_printf("%s.\n", goof);
            Pop;
        } else {
            atl_error(goof);
        }
    }
}

/*  VDEFINE  --  Make a new entry in the virtual function table. */

static vitem *vdefine(vname)
  char *vname;
{
    dictword *dw = atl_lookup(vname);
    struct queue *vfq;
    vitem *vi;

    /* First we determine whether a virtual method has been defined
       for this message.  */

#ifdef VDEFDEBUG
 ads_printf("VDEFINE: %s\n", vname);
#endif
    if ((dw == NULL) || (dw->wcode != P_vfexec)) {
#ifdef VDEFDEBUG
 ads_printf("VDEFINE:  Made new VF entry.\n");
#endif
        /* Define the Atlas word that will invoke the virtual
           function. */
        Ho(Dictwordl + StackCells(sizeof(struct queue)));
        dw = (dictword *) hptr;
        dw->wnext = dict;             /* Enchain word on dictionary */
        dict = dw;
        dw->wname = alloc(strlen(vname) + 2);
        dw->wname[0] = 0;
        strcpy(dw->wname + 1, vname); /* Name word as generic method */
        dw->wcode = P_vfexec;         /* Set virtual function code action */
        hptr += Dictwordl;            /* Allocate heap for this word */
        qinit(hptr);                  /* Create virtual function queue */
        hptr += StackCells(sizeof(struct queue));
    }
    vfq = (struct queue *) atl_body(dw); /* Get body address (queue links) */

    /* Now create a new virtual function item and attach it to the
       queue in the generic function word definition. */

    Ho(StackCells(sizeof(vitem)));
    vi = (vitem *) hptr;              /* Allocate word on heap */
    hptr += StackCells(sizeof(vitem));
    qinsert(vfq, &vi->viql);          /* Link new vitem onto queue */
    vi->vi_vword = dw;                /* Set backpointer to generic word */

    return vi;                        /* Return the new vitem */
}

/*  CLS_LOOKUP  --  Look up a word in the class table.  */

classitem *cls_lookup(cname)
  char *cname;
{
    classitem *ci;

    ucase(cname);
    ci = (classitem *) &classq;

    while ((ci = (classitem *) qnext(ci, &classq)) != NULL) {
        if (strcmp(cname, ci->classname) == 0)
            break;
    }
    return ci;
}

/*  BLOCKDEF  --  Create the start of an instance block definition
                  in the current result buffer chain.  */

void blockdef(bname)
  char *bname;
{
    ads_point normal;
    struct resbuf from, to;

    defent("INSERT");
    tackstring(2, bname);
    tackpoint(10, 0.0, 0.0, 0.0);
    Spoint(normal, 0.0, 0.0, 1.0);    /* Set local normal vector */
    from.restype = to.restype = RTSHORT;
    from.resval.rint = 1;             /* From UCS... */
    to.resval.rint = 0;               /* ...to world. */
    ads_trans(normal, &from, &to, True, normal); /* Transform to world */
    tackvec(210, normal);             /* Append world normal vector */
                                      /* Transform UCS X axis to... */
    to.restype = RT3DPOINT;           /* block's ECS... */
    Cpoint(to.resval.rpoint, normal); /* ...specified by normal */
    Spoint(normal, 1.0, 0.0, 0.0);    /* Get X axis direction */
    ads_trans(normal, &from, &to, True, normal); /* Find UCS X axis in ECS */
    tackreal(50, atan2(normal[Y], normal[X])); /* Convert to ECS rotation */
}

/*  Primitive words made available to ATLAS.  */

prim P_nothing()
{                                     /*  --  */
    /************ WILD KELVIN *************/
    struct resbuf *rb;
    V ads_invoke(ads_buildlist(RTSTR, "C:MOUNTAIN", RTSHORT, 16,
        RTREAL, 1.50, RTREAL, 1.0, RTSHORT, 0, RTSHORT, 0x5625, RTNONE), &rb);
}

/*  OBJECT.INSPECT  --  Inspect and perhaps change instance variables
                        of an object.  */

static void inspy(spying, doclass)
  Boolean spying, doclass;
{                                     /* instance --  */
    char *inst;
    stackitem iresult;

    Sl(1);
    inst = (char *) S0;
    Pop;
    Hpc(inst);
#ifdef SPY
    spyring = spying;
#endif
    iresult = inspect(inst, doclass) ? -1 : 0;
    So(1);
    Push = iresult;
}

prim P_inspect()
{
    inspy(False, False);
}

#ifdef SPY
prim P_spy()
{
    inspy(True, False);
}
#endif

/*  (COMMIT)  --  Low-level primitive to update the database copies of
                  the class variables.  Ideally, this should be called
                  automatically just before a drawing database is
                  written to an external file.  */

prim P_commit()
{                                     /* classhdr --  */
    classdecl *ci;

    Sl(1);
    Hpc(S0);
    ci = (classdecl *) S0;
    Pop;
    if (ci->cd_sentinel != ClassSentinel)
        atl_error("Invalid sentinel in class header");
    else
        store_classvar((classitem *) ci->cd_classitem);
}

/*  (CONSTRUCT)  --  Run the constructor to initialise an instance
                     given the class declaration pointer and the
                     address of the instance to be initialised. */

prim P_construct()
{                                     /* :classname instanceptr --  */
    classdecl *cd;

    Sl(2);
    Hpc(S0);
    Hpc(S1);
    cd = (classdecl *) S1;
    if (cd->cd_sentinel != ClassSentinel) {
        atl_error("(construct): Invalid sentinel in class header");
        return;
    } else {
        cinstvar *cv = (cinstvar *) S0;

        construct(cd, cv);
    }
    Pop2;
}

/*  (ENTUPDATE)  --  Low-level primitive to update an entity when its
                     instance variables have changed.  If the entity
                     does not currently exist in the database, a new
                     entity is created and its name is saved in the
                     instance item. */

prim P_entupdate()
{                                     /* instance -- */
    char *bname = NULL, *ibuf;
    classitem *ci;
    cinstvar *cv;

    Sl(1);
    Hpc(S0);
    ibuf = (char *) S0;
    cv = (cinstvar *) (ibuf - Civl);
    ci = (classitem *) cv->ciclass->cd_classitem;
    Pop;

    if ((cv->ciename[0] == 0) ||
        ((draw_message != NULL) &&
         (vf_search(draw_message, cv->ciclass) != NULL))) {
        bname = create_geometry(ci, ibuf);
    }

    if ((cv->ciename[0] == 0) && (bname != NULL)) {
        ads_relrb(rb);
        blockdef(bname);

        tackrb(-3);                   /* Start of application data */
        /* Application name */
        tackstring(Xed(1), strsave(ourapp));
        /* Class name */
        tackstring(Xed(0), strsave(ci->classname));
        tack_instance(ci, ibuf);
        makent();
        ads_entlast(cv->ciename);
    } else {
        modify_entity(cv->ciename);   /* Load entity for modification */
        if (bname != NULL) {
            /* Swap to new geometry block */
            free(resitem(rb, 2)->resval.rstring);
            resitem(rb, 2)->resval.rstring = strsave(bname);
        }
        /* Tack instance variables onto entity */
        tack_instance(ci, ibuf);
        if (ads_entmod(rb) != RTNORM) {
            atl_error("Error in ads_entmod in (entupdate)");
        }
    }
    ads_relrb(rb);                    /* Release result chain */
}

/*  (CLASSVARS)  --  Given a class header, obtain the address of the class
                     variable buffer for the class.  */

prim P_classvars()
{                                     /* classhdr --  */
    classdecl *ci;

    Sl(1);
    Hpc(S0);
    ci = (classdecl *) S0;
    S0 = (stackitem) (((classitem *) ci->cd_classitem)->classv);
}

/*  (INSTVARS)  --  Given a class header, obtain the address of the scratch
                    instance variable buffer for the class.  */

prim P_instvars()
{
    classdecl *ci;

    Sl(1);
    Hpc(S0);
    ci = (classdecl *) S0;
    S0 = (stackitem) (((classitem *) ci->cd_classitem)->classinstv);
}

/*  (WORDBITS@)  --  Push bits in newest word mode flag field.  */

prim P_rwordbits()
{                                     /*  -- bits  */
    So(1);
    Push = dict->wname[0];
}

/*  (WORDBITS!)  --  Set bits in newest word mode flag field.  */

prim P_wwordbits()
{                                     /* bits --  */
    Sl(1);
    dict->wname[0] = S0;
    Pop;
/* ads_printf("(wordbits) %d\n", dict->wname[0]); */
}

/*  (REDEFINE)  --  Control whether redefinition of primitives causes
                    a compile-time warning.  */

prim P_redefine()
{
    Sl(1);
    atl_redef = S0 ? 0 : -1;
    Pop;
}

/*  (METHODEFINE)  --  Begin method declaration.  We have to interpose
                       ourselves here (at the time the "{" that delimits
                       the start of the method's text) in order to hide
                       the method definition name and replace it, if
                       appropriate, with a virtual method declaration
                       entry with the same name.  */

prim P_undefm()
{
    atl_error("Incompletely defined method executed");
}

prim P_methodefine()
{                                     /* classhead modebits --  */
    int wflags;
    dictword *dw = dict;              /* Operate on current word */
    classdecl *cd;
    char *oname = dw->wname + 1;

    /* If this is a public or protected method, define a virtual
       method name associated with it. */

    Sl(2);
    wflags = S0;
    if ((stackitem *) S1 == NULL) {
        atl_error("(METHODEFINE): No class definition active");
        return;
    }
    cd = (classdecl *) atl_body(S1);
    Pop2;

/* KLUDGE */
        if (strcmp(oname, CreationName) == 0 ||
            strcmp(oname, NewClassName) == 0 ||
            strcmp(oname, ConstructorName) == 0) {
            wflags = 0;               /* Don't virtualise canned name */
        }
/* END KLUDGE */

    if (strcmp(oname, DrawName) == 0) {
        wflags |= S_public;
    }

    if ((wflags & (S_protected | S_public)) != 0) {
        vitem *vi;
        char *namestr = dw->wname,
             *oname = namestr + 1;

        dw->wname[0] |= WORDHIDDEN;   /* Hide definition with method name */

        /* Delete the definition just made and release its heap.  We'll
           put it back in a moment, but first we need to let VDEFINE
           create his virtual method declaration if none has been
           defined previously. */

        dict = dw->wnext;             /* Delete pending definition */
        hptr = (stackitem *) dw;      /* Remove it from the heap */

        vi = vdefine(oname);          /* Define virtual method */
        vi->vi_modes = wflags;        /* Set modes of method */
        vi->vi_class = cd;            /* Class method belongs to */
        vi->vi_classitem = md_classitem; /* Set classitem pointer */
        vi->vi_offset = 0;            /* This is not a field access word */
        vi->vi_nargs = 0;             /* No arguments yet */
        vi->vi_arglist = NULL;        /* Hence, no argument list */
        vi->vi_extnext = NULL;        /* Clear external chain link */

        /* If this method is implemented in an external application,
           enchain it on the list of external methods. */

        if (wflags & M_external) {
            vi->vi_extnext = extlist; /* Attach list to this item */
            extlist = vi;             /* Place this as first on ext list */
        }

        /* If this is a command method, define an AutoCAD command
           to access the method.  */

        if (wflags & M_command) {
            int i;
            cmditem *cm = (cmditem *) &cmdef;

            /* Store the accumulated arguments for this command in
               an on-heap argument table, then set the pointer to
               the argument list in the virtual function item. */

            vi->vi_arglist = (argitem *) hptr;  /* Arglist goes on heap */
            vi->vi_nargs = qlength(&argq);      /* Number of arguments */
            Ho(StackCells(sizeof(argitem)) * vi->vi_nargs);
            hptr += StackCells(sizeof(argitem)) * vi->vi_nargs;
            for (i = 0; i < vi->vi_nargs; i++) {
                argqitem *qi = (argqitem *) qremove(&argq);

                memcpy((char *) &(vi->vi_arglist[i]),
                   (char *) &qi->aqi, sizeof(argitem));

                /* Since the prompt and keyword strings are initially
                   allocated in temporary string buffers, (argdef)
                   makes copies of them with strsave() so they don't
                   get stepped on.  Transcribe those copies to the heap
                   as we go. */

                if (vi->vi_arglist[i].argprompt != NULL) {
                    int sl = strlen(vi->vi_arglist[i].argprompt) + 1;

                    Ho(StackCells(sl));
                    strcpy((char *) hptr, vi->vi_arglist[i].argprompt);
                    free(vi->vi_arglist[i].argprompt);
                    vi->vi_arglist[i].argprompt = (char *) hptr;
                    hptr += sl;
                }
                if (vi->vi_arglist[i].argkw != NULL) {
                    int sl = strlen(vi->vi_arglist[i].argprompt) + 1;

                    Ho(StackCells(sl));
                    strcpy((char *) hptr, vi->vi_arglist[i].argkw);
                    free(vi->vi_arglist[i].argkw);
                    vi->vi_arglist[i].argkw = (char *) hptr;
                    hptr += sl;
                }
            }

            /* See if a command is already defined for this message.
               If so, there's no need to define a new one.  */

            while ((cm = (cmditem *) qnext(cm, &cmdef)) != NULL) {
                if (strcmp(oname, cm->cm_method->wname + 1) == 0)
                    break;
            }
            if (cm == NULL) {
                char cmdname[40];

                cm = geta(cmditem);
                strcpy(cmdname, "C:");
                strcat(cmdname, oname);
                cm->cm_vitem = vi;    /* Set virtual method to run */
                cm->cm_defun_index = defun_index++;
                ads_defun(cmdname, cm->cm_defun_index);
                qinsert(&cmdef, &cm->cmdql); /* Place command on queue */
            }
        }

        /* Finally, put back the pending definition so we can resume
           compilation of code into it. */

        dw = (dictword *) hptr;       /* Point to word we're about to define */
        vi->vi_word_to_run = dw;      /* Actual word to run for this class */
        Ho(Dictwordl);
        dw->wnext = dict;             /* Enchain word on dictionary */
        dict = dw;                    /* Link word to dictionary */
        dw->wname = namestr;          /* Reuse original name buffer */
        dw->wcode = P_undefm;         /* Set undefined method action for now */
        hptr += Dictwordl;            /* Allocate heap for this word */
        createword = dw;              /* Mark this as word being declared */
    }
}

/*  (<-)   --  Explicitly compile virtual field access word.

            This is just about the deepest, darkest, pure black
            magic in the entire program.  Within a class definition,
            public class and instance variable names are used to
            directly access the fields of the current object
            without explicit target object direction or any need to
            go through the virtual function table.  This makes methods
            easy to read and fast to execute, but one problem remains.
            How to we send a message to another object when the message
            name happens to be the same as a field of the current class?
            Rather than saying ``don't do that,'' we provide a way
            around the problem.  Consider a class with a field called
            SIZE.  This field would normally be referenced inside a
            method as follows:

                method grow
                {
                    size 2@ 2.0 f* size 2!
                }

            If I had an object within the class with a SIZE message, I
            could send that message to the member object as follows:

                method grow
                {
                    poly1 size <- 2@ 2.0 f* poly1 size <- 2!
                }

            How does it work?  Well, "<-" is an immediate word that
            looks at the last word we compiled, then rips back through
            the dictionary until it finds a definition that compiles
            into this word.  Then, it continues searching the dictionary
            until it finds a virtual function entry with the same name.

            The same definition also provides a way for a method that
            overloads a method of a parent class to explicitly invoke
            the parent class method on the current instance.  Consider:

                method grow
                {
                    mysize 2@ 2.0 f* mysize 2! grow <-
                }

            Here we're explicitly scaling the field MYSIZE, which is
            defined in the current class, then invoking the GROW method
            of our parent class (if more than one parent class defines
            GROW, the most recent definition in the inheritance chain
            is used).  Had we not specified the "<-" after GROW, this
            would have been compiled as a recursive call on the virtual
            GROW method.  Note that we do not have to explicitly push
            the current instance before invoking the method; since
            parent calls always operate on the current instance, its
            address is pushed automatically in compiling a call on THIS.
*/

prim P_virtualise()
{                                     /* classhdr --  */
    classdecl *cd;
    dictword *mword = (dictword *) hptr[-1]; /* Original word compiled */
    dictword *dw = dict;

    Sl(1);
    Hpc(S0);
    cd = (classdecl *) atl_body(S0);
    Pop;

    /* This next loop looks really idiotic, but it's there for a purpose.
       We want to be absolutely sure the last compiled word is a
       dictionary item, not a literal or something else, before we
       start modifying it and referencing through its pointers. */

    while ((dw != NULL) && (dw != mword)) {
        dw = dw->wnext;
    }

    if (dw == NULL) {
        atl_error("<- used in improper context");
    } else {
        if (dw->wcode == P_vfexec) {
            classdecl *pd = (classdecl *) cd->cd_parent;
            dictword *pword = NULL;
            struct queue *vfq = (struct queue *) atl_body(dw);

            /* The reference was to an already virtual word.  This must
               be a request to send a message to a parent class.  Search
               the virtual function table for a method belonging to a
               parent class.  Then replace the compiled-in reference
               to the virtual method with a hard reference to the
               parent class method, preceded by a push of the current
               instance onto the stack.  (We can't use vf_search() to
               scan the inheritance chain here because it stops upon
               the first non-public derivation.  Fields are always
               inherited.)  */

            while (pd != NULL) {
                vitem *vi = vf_lookup(vfq, pd);

                if (vi != NULL) {
                    pword = vi->vi_word_to_run;
                    break;
                }
                pd = (classdecl *) pd->cd_parent;
            }
            if (pword != NULL) {
                Ho(1);
/* THIS IS WRONG IF FOCUS IS NONZERO. */
                hptr[-1] = (stackitem) thispush; /* Compile in THIS */
                Hstore = (stackitem) pword;      /* Followed by parent word */
            } else {
                atl_error("Message not defined in parent class");
            }
        } else {
            dictword *dsave = dict, *dvert;

            dict = dw->wnext;
            if ((dvert = atl_lookup(dw->wname + 1)) != NULL) {
                hptr[-1] = (stackitem) dvert; /* Swap to use virtual message */
            }
            dict = dsave;
        }
    }
}

/*  (INHERIT)  --  Inherit variables from a parent class.  */

static int inh_vcount;                /* Count of variables inherited */
static int inh_public;                /* Nonzero if public inheritance */

static Boolean inheritv(cc, dw, vtype, vflags, vlen, clip, clcp, vclass)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
  classdecl *vclass;
{
#ifdef INHDEBUG
ads_printf(
"Inherit variable: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld, clcp %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip, clcp);
#endif

    if ((vflags & S_temp) == 0) {     /* Ignore temporary variables */
        stackitem *iargs = atl_body(dw);
        Boolean duzdef = dw->wcode == P_dodoes;
        dictword *iw = (dictword *) (hptr + (duzdef ? 1 : 0));

        /* Create variable definition */

        inh_vcount++;
        Ho(Dictwordl + ((vtype == E_instance) ? 3 : 2) + (duzdef ? 1 : 0));
        if (duzdef) {
            /* This variable was defined with DOES>.  Copy the magical
               code pointer the precedes the dictionary entry to the
               inherited copy of the word. */
            Hstore = *(((stackitem *) dw) - 1);
        }
        iw->wcode = dw->wcode;        /* Same code as parent word */
        iw->wname = alloc(strlen(dw->wname + 1) + 2);
        strcpy(iw->wname + 1, dw->wname + 1); /* Copy name from parent */
        hptr += Dictwordl;
        iw->wnext = dict;
        dict = iw;                    /* Enter word in dictionary */

        /* Hide the variable immediately if it is not declared PUBLIC
           or PROTECTED in the parent class.  Setting the WORDHIDDEN
           mode at the moment of declaration blocks references to the
           field from within this class definition. */

        iw->wname[0] = (vflags & (S_protected | S_public)) ? 0 : WORDHIDDEN;
        Hstore = iargs[0];            /* Transcribe variable modes word */
        Hstore = iargs[1];            /* Transcribe offset field */
        if (vtype == E_instance) {
            Hstore = (stackitem) vclass; /* Copy class pointer of instance */
        }

        /* If this word is visible to the derived class, we still must
           determine its effective mode as inherited and visibility
           in further inheritance.  If the inheritance was of the
           PUBLIC type, public and protected variables retain their
           attributes in the parent class.  If, however, the inheritance
           did not specify the PUBLIC attribute, all the variables we
           inherited will be deemed PRIVATE in the inherited class.
           This blocks any further inheritance or visibility of them. */

        if (!inh_public && (vflags & (S_protected | S_public))) {
            stackitem *flags = ((stackitem *) iw) + Dictwordl;

            *flags = (*flags & ~((S_protected | S_public))) | S_private;
#ifdef INHDEBUG
            ads_printf("Privatising %s in inheritance.\n", iw->wname + 1);
#endif
        }
    }

    return True;
}

prim P_inherit()
{                                     /* newclass parentclass classmodes
                                                      --
                                         classlength instlength varcount */
    dictword *cl;
    classdecl *cd, *pd;
    classitem *pi;

    Sl(3);
    Hpc(S1);
    pd = (classdecl *) S1;
    if (S2 == 0) {
        atl_error("(INHERIT): No class definition active");
        return;
    }
    Hpc(S2);
    cl = (dictword *) S2;
    cd = (classdecl *) atl_body(cl);
    cd->cd_parent = (stackitem) pd;
    cd->cd_classmodes = S0;
    Npop(3);

#ifdef INHDEBUG
    {   dictword *pw = (dictword *) (((char *) cd->cd_parent) - Civl),
                 *dw = (dictword *) (((char *) cd) - Civl);

        ads_printf("Class %s %sinherits class %s\n", dw->wname + 1,
            cd->cd_classmodes & C_dpublic ? "publicly " : "", pw->wname + 1);
    }
#endif

    /* The first step in inheritance is to copy all of the class
       and instance variable declarations from the parent class to
       the derived class.  These variables retain the modes declared
       originally in the parent class.  If a variable is declared PRIVATE
       in the parent class, it is hidden from reference in the
       derived class.  We use iprocvar() to scan the variable
       declarations in the parent class because we don't want to
       recursively unwind nested instances. */

    pi = (classitem *) pd->cd_classitem;
    inh_vcount = 0;                   /* Clear variables inherited */
    inh_public = cd->cd_classmodes & C_dpublic; /* Pass public flag */
    iprocvar(pi, inheritv, 0L, 0L, 0, 0, False);

    /* Finally, leave the class and instance variable lengths and
       the number of variables inherited on the stack.  These
       are used in OBJECT.CLS to set the counters used when
       declaring additional variables in the derived class. */

    So(3);
    Push = pd->cd_cvlength;
    Push = pd->cd_ivlength;
    Push = inh_vcount;
}

/*  (ENDCLASS)  --  Perform class declaration postprocessing.  */

static int ecvardnest;

static Boolean ecvard(cc, dw, vtype, vflags, vlen, clip, clcp)
  classitem *cc;
  dictword *dw;
  int vtype, vflags, vlen;
  long clip, clcp;
{
#ifdef ECVARDEBUG
ads_printf(
"Process variable: CC = %s, Var = %s, vtype %d, vflags %d, vlen %d, clip %ld\n",
        cc->classname, dw->wname + 1, vtype, vflags, vlen, clip);
#endif
    if (vtype == -E_instance) {
        ecvardnest--;
    } else {
        if ((ecvardnest == 0) && (vflags & (S_public | S_protected)) &&
            ((vflags & S_temp) == 0)) {
            vitem *vi = vdefine(dw->wname + 1);

            vi->vi_modes = vflags;    /* Set modes of method */
            vi->vi_word_to_run = NULL;/* No conventional word to run */
            vi->vi_classitem = cc;
            vi->vi_class = (classdecl *) atl_body(cc->classhdr);
            vi->vi_offset = ((vflags & F_class) ? -1 : 1) *
                 (1 + (clip >= 0 ? clip : clcp) +
                     ((vtype == E_instance) ? Civl : 0));
        }
        if (vtype == E_instance)
            ecvardnest++;
    }
    return True;
}

prim P_endclass()
{                                     /* startclass --  */
    dictword *cl, *dw = dict;
    classitem *ci;
    classdecl *cd;

    /* Obtain start of class from stack. */

    Sl(1);
    if (S0 == 0) {
        atl_error("ENDCLASS: No class definition active");
        return;
    }
    Hpc(S0);
    cl = (dictword *) S0;
    Pop;
    cd = (classdecl *) atl_body(cl);

    /* *** HERE WE OUGHT TO MAKE SURE THIS CLASS DEFINITION DOESN'T
           DUPLICATE ONE MADE PREVIOUSLY.  *** */

    ci = cls_lookup(cl->wname + 1);
    if (ci == NULL) {
        char error[80];

        sprintf(error, "Class %s is not defined in an eponymous object",
            cl->wname + 1);
        atl_error(error);
        return;
    }
    ci->classhdr = cl;
    ci->cl_constructor = NULL;        /* No instance constructor */
    ci->cl_newclass = NULL;           /* No class constructor */
    ci->cl_acquisition = NULL;        /* No acquisition method */

    /* Set the backlink from the class definition item on the Atlas
       stack to the classitem in the class definition chain.  This
       lets us get to our private information about the class from the
       Atlas declaration of it.  */

    cd->cd_classitem = (stackitem) ci;

    /* The first step in defining the class is to rip back through
       the dictionary definitions made by the class and rename them
       so they are unique to the class.  As we go, we build the
       virtual function command table for functions declared as
       part of this class.  All the +1's in the following code are
       due to Atlas' cute trick of using the first byte of the word
       name as a mode bit cell. */

#ifdef DBSCAN
   ads_printf("Endclass: %s\n", cl->wname + 1);
#endif
    while (dw != cl) {
        char *oname = dw->wname + 1;

        if (dw->wcode != P_vfexec) {
            dw->wname[0] |= WORDHIDDEN; /* Hide non-virtual definitions */
        }

        /* Look for method definitions with special meanings and save
           them for later use. */

        if (strcmp(oname, CreationName) == 0) {
            ci->cl_acquisition = dw;
        } else if (strcmp(oname, NewClassName) == 0) {
            ci->cl_newclass = dw;
        } else if (strcmp(oname, ConstructorName) == 0) {
            ci->cl_constructor = dw;
        }

        dw = dw->wnext;
    }

#ifdef INHDEBUG
if (cd->cd_parent != NULL) {
 dictword *pw = (dictword *) (((char *) cd->cd_parent) - Civl);
 ads_printf("ENDCLASS: Class %s inherits class %s\n",
   dw->wname + 1, pw->wname + 1);
}
#endif

    /* If the class was derived from another and no acquisition method
       was declared in this class, inherit the acqusition method of the
       parent class. */

    if ((ci->cl_acquisition == NULL) && (cd->cd_parent != 0)) {
        classitem *pi = (classitem *)
            (((classdecl *) (cd->cd_parent))->cd_classitem);

        ci->cl_acquisition = pi->cl_acquisition;
    }

    /* Now define field access virtual methods for all public and
       protected fields of the class. */

    ecvardnest = 0;                   /* Clear nesting of instances */
    procvar(ci, ecvard);

    /* Rename the class name <classname> to :<classname>.  This allows us
       to define a declaring word for the class while preserving access
       to the class name itself for those contexts in which it is
       useful (such as constructing a derived class). */

    {   char *newname = alloc(strlen(cl->wname + 1) + 3),
             *oldname = cl->wname;
        dictword *dw;

        newname[0] = cl->wname[0];
        strcpy(newname + 1, ":");
        strcat(newname + 1, cl->wname + 1);
        cl->wname = newname;

        /* Create the defining word used to declare class instances. */

        Ho(Dictwordl);
        dw = (dictword *) hptr;
        dw->wnext = dict;             /* Enchain word on dictionary */
        dict = dw;
        dw->wname = oldname;          /* Declaring word has name of class */
        dw->wcode = instance_decl->wcode; /* And is a regular : definition */
        hptr += Dictwordl;            /* Allocate heap for this word */
        So(1);
        Push = (stackitem) atl_body(cl);/* Pass the class definition address */
        V atl_exec(instance_decl);    /* Create the instance declaring word */
    }

    /* Allocate the variables declared by this class.  Separate them
       into class and instance, public and private, and allocate
       accordingly. */

    if (cd->cd_sentinel != ClassSentinel) {
        atl_error("Invalid sentinel in class header");
        return;
    } else {
        int off = cd->cd_ivlength;    /* Total instance variable offset */

#ifdef DBSCAN
ads_printf("%d class variables, %d total instance storage.\n", inum, off);
#endif

        /* Allocate instance variable buffer.  We allocate the instance
           variable buffer on the Atlas stack so that Atlas pointer
           references can be done to it without encountering pointer
           checking errors.  Note that we must allocate the instance
           variable buffer even if there are no user-declared instance
           variables; any instance has, at the very least, the class
           definition pointer the belongs to the class CLASS. */

        ci->classinstvl = off;
        {   cinstvar *cv = (cinstvar *) hptr;
            int instlen = (Civl / sizeof(stackitem)) +
                (((2 * ci->classinstvl) + (sizeof(stackitem) - 1)) /
                    sizeof(stackitem));

            Ho(instlen);
            ci->classinstv = ((char *) hptr) + Civl;
            hptr += instlen;
            construct(cd, cv);        /* Construct new instance */
        }

        /* Allocate class variable buffer.  It is also allocated on the
           Atlas stack.  All class variables are initialised to zero,
           by default. */

        if ((ci->classvl = cd->cd_cvlength) != 0) {
            ci->classv = (char *) hptr;
            hptr += ((2 * cd->cd_cvlength) + (sizeof(stackitem) - 1)) /
                sizeof(stackitem);
            load_classvar(ci);        /* Load current class variables */
        }
    }

    /* Store the initial values of the class variables. */

    store_classvar(ci);

    /* Finally, define an AutoCAD command which, when executed,
       creates an instance of the class.  The AutoCAD command has
       the same name as the class. */

    if (ci->cl_acquisition != NULL) {
        char clcreate[40];

        ci->cl_create_index = defun_index++;
        strcpy(clcreate, "C:");
        strcat(clcreate, ci->classname);
        ads_defun(clcreate, ci->cl_create_index);
    }
}

/*  FETCH  --  Load an entity designated by its handle and return the
               temporary object describing the entity.  */

prim P_fetch()
{                                     /* pointer -- object/0 */
    ads_name ename;

    Sl(1);
    Hpc(S0);
    if (ads_handent((char *) S0, ename) == RTNORM) {
        classitem *ci = entclass(ename, True);

        if (ci != NULL) {
            S0 = (stackitem) ci->classinstv;
        } else {
            S0 = 0;
        }
    } else {
        S0 = 0;
    }
}

/*  WORDZ  --  List words defined by current set of classes.  */

prim P_wordz()
{
    dictword *dw = dict;
    int lpos = 0;

    while (dw != NULL && dw->wname[1] != '(') {
        char wb[40];
        char *wp;

        strncpy(wb, dw->wname + 1, sizeof wb);
        if (strlen(wb) == 0) {
            strcpy(wb, "=VOID!=");
        }
        wb[39] = EOS;
        for (wp = wb; *wp != EOS; *wp++) {
            if (*wp == ' ')           /* Make blanks visible */
                *wp = '~';
        }
        if (dw->wname[0] & WORDHIDDEN) {
            lcase(wb);
        }
        if ((lpos + strlen(wb) + (lpos == 0 ? 0 : 1)) > 76) {
            ads_printf("\n");
            lpos = 0;
        }
        V ads_printf("%s%s", lpos == 0 ? "" : " ", wb);
        lpos += strlen(wb) + (lpos == 0 ? 0 : 1);
        dw = dw->wnext;
    }
    V ads_printf("\n");
}

/*  CLS_INIT  --  Initialise the class system.  */

void cls_init()
{
    FILE *fp;
    char filename[256];
    dictword *dw;

    static struct primfcn primt[] = {

        {"0NOTHING",        P_nothing},  /* Dummy placeholder */
        {"0(<-)",           P_virtualise},
        {"0(ARGDEF)",       P_argdef},
        {"0(ARG_GET)",      P_arg_get},
        {"0(ARGWIPE)",      P_argwipe},
        {"0(CLASSVARS)",    P_classvars},
        {"0(COMMIT)",       P_commit},
        {"0(CONSTRUCT)",    P_construct},
        {"0(ENDCLASS)",     P_endclass},
        {"0(ENTUPDATE)",    P_entupdate},
        {"0FETCH",          P_fetch},
        {"0(INHERIT)",      P_inherit},
        {"0(INSTVARS)",     P_instvars},
        {"0(METHODEFINE)",  P_methodefine},
        {"0OBJECT.INSPECT", P_inspect},
#ifdef SPY
        {"0OBJECT.SPY",     P_spy},
#endif
        {"0(REDEFINE)",     P_redefine},
        {"0(WORDBITS@)",    P_rwordbits},
        {"0(WORDBITS!)",    P_wwordbits},

        {"0WORDZ",          P_wordz},

        {NULL,              (codeptr) 0}
    };
    atl_primdef(primt);

    /* Define class system shared variables. */

#define Createvar(x,y) x=(atl_int *) atl_body(atl_vardef((y),sizeof(atl_int)))

    Createvar(v_classtrace,  "OBJECT.TRACE");
    Createvar(v_focus,       "OBJECT.FOCUS");
    Createvar(v_drawcolour,  "OBJECT.DRAWCOL");
    Createvar(v_wireframe,   "OBJECT.WIREFRAME");
    v_drawltype = (char *) atl_body(atl_vardef("OBJECT.DRAWLTYPE", 32));
    v_stellation = (atl_real *)
        atl_body(atl_vardef("OBJECT.STELLATION", sizeof(atl_real)));
    Createvar(v_cnsegs,      "OBJECT.CNSEGS");

#undef Createvar

    cnsegs = 8;                       /* Set segments in the circle */
    drawcolour = 0;                   /* Default colour by block */
    strcpy(drawltype, "BYBLOCK");     /* And line type as well */

    /* Ditch any items left on the external command definition table.
       Since this code is executed at initialisation time, we don't
       need to undefine the commands; they've already been discarded
       by AutoCAD when AutoLisp's reinitialised. */

    while (!qempty(&cmdef)) {
        free(qremove(&cmdef));
    }

    /* Load the optional class system configuration file. */

    if (ads_findfile("config.cls", filename) == RTNORM) {
       fp = fopen(filename,
#ifdef FBmode
                           "rb"
#else
                           "r"
#endif
       );
    } else {
        fp = NULL;
    }
    if (fp != NULL) {
        int s = atl_load(fp);
        fclose(fp);
        if (s != ATL_SNORM) {
            ads_printf(
                "Error in CONFIG.CLS file.  Class system overthrown.\n");
        }
    }

    /* Load the class system master implementation program. */

    if (ads_findfile("object.cls", filename) == RTNORM) {
       fp = fopen(filename,
#ifdef FBmode
                           "rb"
#else
                           "r"
#endif
       );
    } else {
        fp = NULL;
    }
    if (fp != NULL) {
        int s = atl_load(fp);
        fclose(fp);
        if (s != ATL_SNORM) {
            ads_printf(
                "Error in OBJECT.CLS file.  Class system overthrown.\n");
        }
    } else {
        ads_printf("OBJECT.CLS file missing.\n");
    }

    /* Look up interface definition "hook" words. */

    startdraw = atl_lookup(ClassStartDraw);
    enddraw = atl_lookup(ClassEndDraw);
    thispush = atl_lookup(ThisPush);
    instance_decl = atl_lookup(ClassInstanceDecl);
    dw = atl_lookup(SelfName);
    assert(dw != NULL);
    object_self = (char **) atl_body(dw);
#ifdef DEBRIS
    debris = atl_lookup(ClassDebris);
#endif

    /* If AutoCAD is in the middle of defining a complex entity, wipe
       it out. */

    V ads_entmake(NULL);
}
