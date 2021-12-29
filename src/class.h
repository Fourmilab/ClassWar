/*

        Definitions for Class system

*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#ifdef __TURBOC__
#define assert(x)
#else
#include "adssert.h"
#endif
#include <string.h>
#ifdef ALIGNMENT
#ifdef __TURBOC__
#include <mem.h>
#else
#include <memory.h>
#endif
#endif

#include "adslib.h"
#include "atlasdef.h"
#include "queues.h"

/* Co-ordinate indices within arrays. */

#define X     0
#define Y     1
#define Z     2

/* Implicit functions (work for all numeric types). */

#ifdef abs
#undef abs
#endif
#define abs(x)   ((x) < 0    ? -(x) : (x))
#ifdef max
#undef max
#endif
#define max(a,b) ((a) >  (b) ?  (a) : (b))
#ifdef min
#undef min
#endif
#define min(a,b) ((a) <= (b) ?  (a) : (b))

/* Utility definition to get an  array's  element  count  (at  compile
   time).   For  example:

       int  arr[] = {1,2,3,4,5};
       ...
       printf("%d", ELEMENTS(arr));

   would print a five.  ELEMENTS("abc") can also be used to  tell  how
   many  bytes are in a string constant INCLUDING THE TRAILING NULL. */

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

#define V     (void)

#define Fuzz    1E-6                  /* Real comparison fuzz factor */
#define EOS     '\0'                  /* End of string */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

/*  Allocate typedef and structure  */

#define geta(q)  (q *) alloc(sizeof(q))
#define gets(q)  (struct q *) alloc(sizeof(struct q))

/*  Data types  */

typedef enum {False = 0, True = 1} Boolean;

/* Set point variable from three co-ordinates */

#define Spoint(pt, x, y, z)  pt[X] = (x);  pt[Y] = (y);  pt[Z] = (z)

/* Copy point from another */

#define Cpoint(d, s)   d[X] = s[X];  d[Y] = s[Y];  d[Z] = s[Z]

/* Copy entity name from another */

#define Cname(d, s)    d[0] = s[0];  d[1] = s[1]

/* Definitions for building result buffer chains. */

#define tacky()     struct resbuf *rb, *rbtail, *ri
#define tackrb(t)   ri=ads_newrb(t);  rbtail->rbnext=ri;  rbtail=ri
#define defent(e)   rb=ri=rbtail=ads_newrb(0); ri->resval.rstring = strsave(e)
#define Xed(g)     (1000 + (g))       /* Generate extended entity data code */
#define makent() { int stat=ads_entmake(rb); if (stat!=RTNORM) { ads_printf("Error creating %s entity\n", rb->resval.rstring); } ads_relrb(rb); }
#define tackint(code, val) tackrb(code); ri->resval.rint = (val)
#define tackreal(code, val) tackrb(code); ri->resval.rreal = (val)
#define tackpoint(code, x, y, z) tackrb(code); Spoint(ri->resval.rpoint, (x), (y), (z))
#define tackvec(code, vec) tackrb(code); Cpoint(ri->resval.rpoint, vec)
#define tackstring(code, s) tackrb(code), ri->resval.rstring = strsave(s)
#define tackatt() tackstring(6, drawltype); tackint(62, drawcolour)

/* We redefine ads_entget() to call ads_entgetx() with our application
   filter for extended entity data. */

#define ads_entget(x)  ads_entgetx((x), &applist)

/* Definitions  to  wrap  around  submission  of  AutoCAD commands to
   prevent their being echoed.  */

#define Cmdecho  False             /* Make True for debug command output */

#define CommandB()  { struct resbuf rBc, rBb, rBu, rBh;             \
        ads_getvar("CMDECHO", &rBc); ads_getvar("BLIPMODE", &rBb);   \
        ads_getvar("HIGHLIGHT", &rBh);                                \
        rBu.restype = RTSHORT; rBu.resval.rint = (int) Cmdecho;        \
        ads_setvar("CMDECHO", &rBu); rBu.resval.rint = (int) False;     \
        ads_setvar("BLIPMODE", &rBu); ads_setvar("HIGHLIGHT", &rBu)

#define CommandE()  ads_setvar("CMDECHO", &rBc); \
                    ads_setvar("BLIPMODE", &rBb); \
                    ads_setvar("HIGHLIGHT", &rBh); }

/*  Definitions  that permit you to push and pop system variables with
    minimal complexity.  These don't work  (and  will  cause  horrible
    crashes  if  used)  with  string  variables,  but since all string
    variables are read-only, they cannot be saved and restored in  any
    case.  */

#define PushVar(var, cell, newval, newtype) { struct resbuf cell, cNeW; \
        ads_getvar(var, &cell); cNeW.restype = cell.restype;             \
        cNeW.resval.newtype = newval; ads_setvar(var, &cNeW)

#define PopVar(var, cell) ads_setvar(var, &cell); }

#define PAUSE   RTSTR, "\\"           /* Pause for user input in ads_command */

/*  Variable type codes.  */

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
#define E_instance      99

/*  Pseudo variable type codes for acquisition.  */

#define E_angle        140
#define E_orientation  240
#define E_corner       111
#define E_keyword      100

/*  Storage classes.  */

#define S_private        1
#define S_protected      2
#define S_public         4
#define S_temp           8

/*  Storage flags.  */

#define F_instance       0
#define F_class         16

/*  Class mode bits.  */

#define C_dpublic        1            /* Publicly derived class */

/*  Method type bits.  */

#define M_atlas          0            /* Regular Atlas-implemented method */
#define M_command       16            /* Method defined as AutoCAD command */
#define M_external      32            /* Method implemented in external app */

/*  Result codes from WHATCHANGED.  */

#define ChangedClass    1             /* Class variable changed */
#define ChangedInstance 2             /* Instance variable changed */

/*  Atlas interface names.  */

#define DrawName        "DRAW"        /* Name of geometry creation word */

/*  Calculate the stack cells needed to hold a given number of bytes.  */

#define StackCells(x) (((x) + (sizeof(stackitem) - 1)) / sizeof(stackitem))

/*  The following structure describes the class definition header
    compiled in for a class declaration.  It must agree with the
    definition of the CLASS word in OBJECT.CLS.  */

typedef struct {
    stackitem cd_sentinel;            /* ClassSentinel */
    stackitem cd_ivcount;             /* Instance variable count */
    stackitem cd_ivlength;            /* Instance variable size in bytes */
    stackitem cd_classitem;           /* Pointer to classitem */
    stackitem cd_cvlength;            /* Class variable size in bytes */
    stackitem cd_parent;              /* Parent class or NULL */
    stackitem cd_classmodes;          /* Class mode bits (public derived) */

    stackitem cd_vartab;              /* First variable in variable table */
} classdecl;

/* The following structure describes the instance variables of a class.
   The instance variable word pushes the CIVFIRST field address onto the
   stack.  The class pointer and AutoCAD entity name that precede it
   can be used as contect by words that require this information.  */

typedef struct {
    classdecl *ciclass;               /* Class object belongs to */
    ads_name ciename;                 /* AutoCAD entity database name */
/*  stackitem civfirst;                  First variable storage field */
} cinstvar;

#define Civl    (sizeof(cinstvar))    /* Size of class instance prologue */

/*  Item for list of classes in drawing.  */

typedef struct {
    struct queue clq;                 /* Queue links */
    char classname[32];               /* Class name */
    char *classfile;                  /* A file class?  If so, its name. */
    ads_name classent;                /* Entity defining class */
    int classinstvl;                  /* Instance variable length */
    char *classinstv;                 /* Instance variable buffer */
    int classvl;                      /* Class variable length */
    char *classv;                     /* Class variable buffer */
    dictword *classhdr;               /* Class header on Atlas heap */
    int cl_create_index;              /* AutoCAD instance create command idx */
    dictword *cl_constructor;         /* Constructor for instances */
    dictword *cl_newclass;            /* Constructor for class variables */
    dictword *cl_acquisition;         /* Acquisition word */
} classitem;

/*  Virtual command argument table item.  */

typedef struct {
    int argtype;                      /* Argument type code (such as E_real) */
    char *argprompt;                  /* Prompt string or NULL */
    int argmodes;                     /* Acquisition (ads_initget) modes */
    char *argdefault;                 /* Default value pointer or NULL */
    char *argbase;                    /* Base point or NULL */
    char *argkw;                      /* Keyword string or NULL */
} argitem;

typedef struct {                      /* Form with queue links */
    struct queue argql;               /* Links on argument queue */
    argitem aqi;                      /* Body of argument item */
} argqitem;

/*  Virtual function table entry item.  */

typedef struct vitems {
    struct queue viql;                /* Class virtual function table links */
    dictword *vi_vword;               /* Generic method word pointer */
    classdecl *vi_class;              /* Class this method applies to */
    classitem *vi_classitem;          /* Classitem for this method */
    dictword *vi_word_to_run;         /* Word to run for this message to this
                                         class. */
    int vi_modes;                     /* Modes from method to run */
    int vi_offset;                    /* If vi_word_to_run is nonzero,
                                         vi_offset specifies the offset of
                                         a data access field within the
                                         instance (if positive) or class
                                         (if negative).  One is added to
                                         the offset so the field is always
                                         nonzero.  When this entry is
                                         activated, the address of the field
                                         is pushed onto the stack. */
    struct vitems *vi_extnext;        /* Next item on external method list */
    int vi_nargs;                     /* Argument count for command function */
    argitem *vi_arglist;              /* Argument table for command function */
} vitem;

/*  External command definition table item.  */

typedef struct {
    struct queue cmdql;               /* Links on command definition queue */
    int cm_defun_index;               /* Command definition index */
#define       cm_method     cm_vitem->vi_vword
    vitem *cm_vitem;                  /* Virtual function table item */
} cmditem;

/*  Class argument descriptor.  This table, which contains
    vi_nargs entries, describes, for each, how many words
    to push onto the stack for that argument and the source of the
    data to be pushed. */

typedef struct {
    int ca_nitems;                    /* Stackitems to be pushed */
    stackitem *ca_source;             /* First stackitem for this argument */
} clarg;

/*  Class summary table item.  Use to prepare the argument lists submitted
    for methods invoked as AutoCAD commands.  */

typedef struct {
    struct queue clasql;              /* Class summary queue links */
    classitem *clasci;                /* Class item pointer */
    vitem *clasvi;                    /* Vitem for object specific method */
    clarg clasarg[1];                 /* Argument summary */
} classum;

/*  Global functions  */

extern void lcase(), ucase(), procvar(), tack_instance(),
            modify_entity(), store_classvar(), P_vfexec(),
            P_argwipe(), P_argdef(), P_arg_get(), save_classvars(),
            classclean(), blockdef(), construct();
extern Boolean arghhh(), inspect(), cls_edit();
extern int callword(), whatchanged();
extern char *alloc(), *strsave(), *create_geometry();
extern struct resbuf *resitem();
extern classitem *chooseclass(), *entclass(), *cls_lookup();
extern vitem *vf_search();

/*  Global variables  */

extern char *ourapp;                  /* Our application name */
extern char classblock[];             /* Class block name */
extern vitem *extlist;                /* External method list */
extern Boolean spyring;               /* INSPECT/SPY flag */
extern tacky();                       /* Result buffer assembly pointers */
extern struct queue classq;           /* Class list */
extern struct queue cmdef;            /* External command table */
extern struct queue argq;             /* Method argument table */
extern struct queue *draw_message;    /* Virtual draw method */
extern classitem *md_classitem;       /* Classitem for compilation underway */
extern struct resbuf applist;         /* Application list for ads_entgetx() */
typedef atl_int *shared;
extern shared v_focus;                /* Current object focus or zero */
#define         focus       *v_focus
extern shared v_drawcolour;           /* Current drawing colour */
#define         drawcolour  *v_drawcolour
extern shared v_wireframe;            /* SGLIB wire frame mode */
#define         wireframe   *v_wireframe
extern shared v_cnsegs;               /* Circle number of segments */
#define         cnsegs      *v_cnsegs
extern double *v_stellation;          /* Polygon stellation */
#define         stellation  *v_stellation
extern char *v_drawltype;             /* Current drawing line type */
#define         drawltype    v_drawltype
extern char **object_self;            /* Current method's instance */
#define         self        *object_self
