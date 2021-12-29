/*
#define PANICPAUSE
#define INITPAUSE
*/
#define SPY
#define DEBUG
/*

    General purpose object and class service ADS application

    Designed and implemented by John Walker in February 1990.

*/

#include "class.h"
#include "sglib.h"

/*  AutoCAD interface names  */

#define AppName     "AUTODESK_OBJECT_DATABASE"  /* Application name */

/*  Data structures  */

#define TurtleSent  0x2F5A7FC2        /* Sentinel for turtle objects */
#define TurtleScent icky              /* Odor of turtles */

/*  Turtle drawing modes for the DRAWLINES field in the turtle structure.  */

#define D_nothing   0                 /* No entities: GRDRAW only */
#define D_polylines 1                 /* 3D Polyline */
#define D_faces     2                 /* 3D Faces */
#define D_lines     3                 /* Line entities */

/*  Turtle structure.  Turtles exist on the Atlas heap.  Each is stored
    in a sequence of stackitems containing the following data.  */

typedef struct {
    atl_int tsent;                    /* Sentinel */
    ads_point p;                      /* Position */
    ads_point h;                      /* Normalised heading vector */
    ads_point l;                      /* Normalised rotation plane vector */
    ads_point u;                      /* Normalised up vector */
    Boolean pendown;                  /* Is pen down ? */
    Boolean visible;                  /* Is turtle visible ? */
    int drawlines;                    /* Make entities ? */
    Boolean polyactive;               /* Polyline active ? */
    int polycount;                    /* Vertices in polyline */
    ads_point firstvert;              /* First vertex of polyline */
} turtle;

/*  Globals imported  */

#ifdef lint
extern char *sprintf();
#endif
extern char *getenv();
extern void cls_ename(), cls_inspect(), cls_inspclass(),
            cls_new(), cls_newf(), cls_update(), cls_to_c();
#ifdef SPY
extern void cls_spy(), cls_spyclass();
#endif

/*  Forward functions  */

void atlas(), atlas1();
#ifdef DEBUG
void snag(), ubi();
#endif
Boolean funcload(), initacad();       /* ADS initialisation functions */

/*  Local variables  */

static Boolean functional;            /* C:command is returning result */

static turtle *cturt = NULL;          /* Pointer to current turtle */
static Boolean turtleshown = False;   /* Is turtle icon on screen ? */

typedef atl_real *shareal;            /* Shared real variable data type */

static shareal v_turtleheight;        /* Turtle height */
static shareal v_turtlewidth;         /* Turtle width */
static shareal v_turtledepth;         /* Turtle depth */

/*  Access tags used to indirectly access shared variables.  */

#define turtleheight    *v_turtleheight
#define turtlewidth     *v_turtlewidth
#define turtledepth     *v_turtledepth

/*  ADS command definition and dispatch table.  */

struct {
    char *cmdname;
    void (*cmdfunc)();
} cmdtab[] = {
/*    Name         Function  */
#ifdef DEBUG
    {"SNAG",       snag},
    {"UBI",        ubi},              /* Dump user break instrumentation */
#endif
    {"/",          atlas1},           /* Execute an Atlas command */
    {"ATLAS",      atlas},            /* Interactive Atlas execution */

    {"CLASSDEF",   cls_new},          /* Define a new class */
    {"CLASSEDIT",  cls_ename},        /* Edit class by name or list */
    {"CLASSFILE",  cls_newf},         /* Import a class from a file */
    {"CLASSTOC",   cls_to_c},         /* Export a class as a C header */
    {"CLASSUPD",   cls_update},       /* Reload all external classes */
    {"INSPECT",    cls_inspect},      /* Inspect an instance */
    {"INSPCLASS",  cls_inspclass},    /* Inspect class variables */
#ifdef SPY
    {"SPY",        cls_spy},          /* Spy on an instance */
    {"SPYCLASS",   cls_spyclass},     /* Spy on class variables */
#endif
};

/*  ALLOC  --  Allocate memory and error upon exhaustion.  */

char *alloc(size)
  unsigned int size;
{
    char *cp = malloc(size);

    if (cp == NULL) {
        V printf("\n\nOut of memory!  %u bytes requested.\n");
        abort();
    }
    return cp;
}

/*  STRSAVE  --  Allocate a duplicate of a string.  */

char *strsave(s)
  char *s;
{
    char *c = alloc((unsigned int) (strlen(s) + 1));

    V strcpy(c, s);
    return c;
}

/*  UCASE  --  Force letters in string to upper case.  */

void ucase(c)
  char *c;
{
    char ch;

    while ((ch = *c) != EOS) {
        if (islower(ch))
            *c = toupper(ch);
        c++;
    }
}

/*  LCASE  --  Force letters in string to lower case.  */

void lcase(c)
  char *c;
{
    char ch;

    while ((ch = *c) != EOS) {
        if (isupper(ch))
            *c = tolower(ch);
        c++;
    }
}

/*  ITURTLE  --  Initialise a turtle.  */

static void iturtle(t)
  turtle *t;
{
    t->tsent = TurtleSent;
    pointget(t->p, 0.0, 0.0, 0.0);
    pointget(t->h, 0.0, 1.0, 0.0);
    pointget(t->l, -1.0, 0.0, 0.0);
    pointget(t->u, 0.0, 0.0, 1.0);
    t->pendown = True;
    t->visible = True;
    t->drawlines = D_nothing;
    t->polyactive = False;
    t->polycount = 0;
}

/*  FLUSHTURTLE  --  Output any pending entity being created by the
                     turtle.  Thank heaven we're using turtles and
                     not alligators!  */

static void flushturtle(t)
  turtle *t;
{
    if (cturt->polyactive) {
        tacky();

        cturt->polyactive = False;

        /* If we're generating a face, output the vertex codes
           to complete the rat nest mesh. */

        if (cturt->drawlines == D_faces) {
            defent("VERTEX");
            tackatt();
            tackpoint(10, 0.0, 0.0, 0.0);
            tackint(70, 128);
            tackint(71, 1);
            if (cturt->polycount > 1) {
                tackint(72, 2);
                if (cturt->polycount > 2) {
                    tackint(73, 3);
                    if (cturt->polycount > 3) {
                        tackint(74, cturt->polycount > 4 ? -4 : 4);
                    }
                }
            }
            makent();
            if (cturt->polycount > 4) {
                int i, lvert = 4;

                for (i = 4; i < cturt->polycount; i++) {
                    defent("VERTEX");
                    tackatt();
                    tackpoint(10, 0.0, 0.0, 0.0);
                    tackint(70, 128);
                    tackint(71, -1);
                    tackint(72, lvert);
                    lvert = i + 1;
                    tackint(73, (i == (cturt->polycount - 1)) ?
                        lvert : -lvert);
                    makent();
                }
            }
        }

        cturt->polycount = 0;
        defent("SEQEND");
        tackatt();
        makent();
    }
}

/*  DRAW  --  Draw in the current mode.  The raw turtle co-ordinates
              are transformed by the currently effective SGLIB
              transformation matrix.  */

static void draw(ifrom, ito)
  point ifrom, ito;
{
    if (cturt->pendown) {
        tacky();
        point from, to;

        transpt(from, ifrom);         /* Transform from point */
        transpt(to, ito);             /* Transform to point */
        switch (cturt->drawlines) {
            case D_nothing:
                ads_grdraw(from, to, 7, 0);
                break;

            case D_polylines:
            case D_faces:
                if (!cturt->polyactive) {
                    cturt->polyactive = True;
                    defent("POLYLINE");
                    tackatt();
                    tackint(66, 1);
                    tackint(70, cturt->drawlines == D_faces ? 64 : 8);
                    makent();
                    defent("VERTEX");
                    tackatt();
                    tackvec(10, from);
                    tackint(70, cturt->drawlines == D_faces ? 128 + 64 : 32);
                    makent();
                    cturt->polycount = 1;
                    Cpoint(cturt->firstvert, from);
                }
                if ((cturt->drawlines == D_faces) &&
                    (cturt->polycount >= 2) &&
                    (ads_distance(cturt->firstvert, to) < Fuzz)) {
                    flushturtle(cturt);
                } else {
                    defent("VERTEX");
                    tackatt();
                    tackvec(10, to);
                    tackint(70, cturt->drawlines == D_faces ? 128 + 64 : 32);
                    makent();
                    cturt->polycount++;
                }
                break;

            case D_lines:
                defent("LINE");   /* Create a LINE entity */
                tackatt();
                tackvec(10, from);
                tackvec(11, to);
                makent();
                break;
        }
    }
}

/*  DRAWTURTLE  --  Draw the turtle icon.  We draw in XOR ink so
                    the icon doesn't crud up the screen. */

static void drawturtle()
{
    point p1, p2, p3, th, tl;

    vecscal(th, cturt->h, turtleheight / 2.0);
    vecscal(tl, cturt->l, turtlewidth / 2.0);
    vecadd(p1, cturt->p, th);
    vecsub(p2, cturt->p, th);
    vecsub(p3, p2, tl);
    vecadd(p2, p2, tl);
    transpt(p1, p1);
    transpt(p2, p2);
    transpt(p3, p3);
    ads_grdraw(p1, p2, -1, 0);
    ads_grdraw(p2, p3, -1, 0);
    ads_grdraw(p3, p1, -1, 0);
    vecscal(th, cturt->u, turtledepth / 2.0);
    vecadd(p1, cturt->p, th);
    transpt(p1, p1);
    transpt(p2, cturt->p);
    ads_grdraw(p2, p1, -1, 0);
}

/*  CLEARTURTLE  --  Clear the turtle icon if it's on the screen.  */

static void clearturtle()
{
    if (turtleshown) {
        drawturtle();
        turtleshown = False;
    }
}

/*  FORWARD  --  Move forward N units.  */

prim P_forward()
{                                     /* fdist --  */
    point newp;

    Sl(Realsize);
    vecscal(newp, cturt->h, REAL0);
    Realpop;
    vecadd(newp, newp, cturt->p);
    draw(cturt->p, newp);
    pointcopy(cturt->p, newp);
}

/*  BACK  --  Move back N units.  */

prim P_back()
{                                     /* fdist --  */
    Sl(Realsize);
    SREAL0(-REAL0);
    P_forward();
}

/*  ROTATE  --  Perform rotation around general axis.  */

static void rotate(a, b)
  ads_point a, b;
{
    ads_real ang;
    ads_point t;

    Sl(Realsize);
    ang = REAL0 * (M_PI / 180.0);
    Realpop;
    pointget(t, a[X] * cos(ang) + b[X] * sin(ang),
                a[Y] * cos(ang) + b[Y] * sin(ang),
                a[Z] * cos(ang) + b[Z] * sin(ang));
    pointget(b, b[X] * cos(ang) - a[X] * sin(ang),
                b[Y] * cos(ang) - a[Y] * sin(ang),
                b[Z] * cos(ang) - a[Z] * sin(ang));
    pointcopy(a, t);
}

/*  LEFT  --  Turn left N degrees.  */

prim P_left()
{                                     /* fangle --  */
    rotate(cturt->h, cturt->l);
}

/*  PITCH  --  Pitch (rotate around local X axis) N degrees.  */

prim P_pitch()
{                                     /* fangle --  */
    rotate(cturt->h, cturt->u);
}

/*  ROLL  --  Roll (rotate around local Y axis) N degrees.  */

prim P_roll()
{                                     /* fangle --  */
    rotate(cturt->l, cturt->u);
}

/*  SETHEADING  --  Set 2D heading vectors.  As documented on Page 115,
                    setheading uses angles where 0 denotes the X axis
                    and angles increase counterclockwise. */

prim P_setheading()
{                                     /* heading --  */
    ads_real ang;

    Sl(Realsize);
    ang = REAL0 * (M_PI / 180.0);
    Realpop;
    cturt->h[X] = cos(ang);
    cturt->h[Y] = sin(ang);
    cturt->l[X] = -sin(ang);
    cturt->l[Y] = cos(ang);
}

/*  SETPOSITION  --  Set position in space.  Takes X, Y, and Z position
                     from the stack.  */

prim P_setposition()
{                                     /* xpos ypos zpos --  */
    point newp;
    Sl(Realsize * 3);

    pointget(newp, REAL2, REAL1, REAL0);
    draw(cturt->p, newp);
    pointcopy(cturt->p, newp);
    Npop(Realsize * 3);
}

/*  PENUP  --  Raise the pen.  */

prim P_penup()
{                                     /*  --  */
    flushturtle(cturt);
    cturt->pendown = False;
}

/*  PENDOWN  --  Lower the pen.  */

prim P_pendown()
{                                     /*  --  */
    cturt->pendown = True;
}

/*  SHOWTURTLE  --  Show turtle icon.  */

prim P_showturtle()
{                                     /*  --  */
    cturt->visible = True;
}

/*  HIDETURTLE  --  Hide turtle icon.  */

prim P_hideturtle()
{                                     /*  --  */
    cturt->visible = False;
}

/*  LEAVETRACKS  --  Control whether entities are created as turtle moves.  */

prim P_leavetracks()
{
    Sl(1);
    flushturtle(cturt);
    cturt->drawlines = S0;
    Pop;
}

/*  RESET  --  Reinitialise all the turtle parameters.  */

prim P_reset()
{                                     /*  --  */
    startup();                        /* Clear current SGLIB transformation */
    flushturtle(cturt);
    iturtle(cturt);
    turtleheight = 0.5;
    turtlewidth = 0.4;
    turtledepth = 0.25;
}

/*  RIGHT  --  Turn right N degrees.  */

prim P_right()
{                                     /* fangle --  */
    Sl(Realsize);
    SREAL0(-REAL0);
    P_left();
}

/*  TURTLE  --  Declare a new turtle.  */

prim P_turtle()
{                                     /*  --  */
    int hl = (sizeof(turtle) + (sizeof(stackitem) - 1)) / sizeof(stackitem);

    Ho(hl);
    P_create();
    iturtle((turtle *) hptr);
    hptr += hl;
}

/*  LISTEN:  --  Make a new turtle active.  */

prim P_listen()
{                                     /* turtle --  */
    turtle *tp;

    Sl(1);
    Hpc(S0);
    tp = (turtle *) S0;
    if (tp->tsent != TurtleSent) {
        atl_error("Not a turtle");
        return;
    }
    flushturtle(cturt);               /* Sorry, ads_entmake doesn't nest */
    cturt = tp;
    Pop;
}

/*  ME  --  Place current turtle on top of stack.  */

prim P_me()
{                                     /*  -- turtle */
    So(1);
    Push = (stackitem) cturt;
}

/*  LOAD  --  Load a file.  */

prim P_load()
{                                     /* sfname --  */
    FILE *fp;
    char fname[256];

    Sl(1);
    Hpc(S0);
    strcpy(fname, (char *) S0);
    if (strchr(fname, '.') == NULL)
        strcat(fname, ".tur");
    fp = fopen(fname,
#ifdef FBmode
                       "rb"
#else
                       "r"
#endif
    );
    Pop;
    if (fp != NULL) {
        flushturtle(cturt);
        V atl_load(fp);
        fclose(fp);
    } else {
        atl_error("LOAD: bad file name.");
    }
}

/*  REDRAW  --  Redraw the AutoCAD screen.  */

prim P_redraw()
{
    flushturtle(cturt);
    ads_redraw(NULL, 0);
}

/*  MOUSE  --  Sense pointing device location  */

prim P_mouse()
{
    int what;
    struct resbuf rb;

    So(Realsize * 3);
    while (True) {
        if (ads_grread(True, &what, &rb) == RTNORM) {
            if (what == 3 || what == 5) {
                for (what = X; what <= Z; what++) {
                    Push = 0;
                    Push = 0;
                    SREAL0(rb.resval.rpoint[what]);
                }
            }
            break;
        } else {
            for (what = 0; what < Realsize * 2; what++) {
                Push = 0;
            }
            break;
        }
    }
}

/*  PICKPOINT  --  Request point from user. */

prim P_pickpoint()
{
    int what;
    ads_point pt;
    char *prompt;

    Sl(1);
    Hpc(S0);
    prompt = (char *) S0;
    Pop;
    So(Realsize * 3);
    if (ads_getpoint(NULL, prompt, pt) == RTNORM) {
        for (what = X; what <= Z; what++) {
            Push = 0;
            Push = 0;
            SREAL0(pt[what]);
        }
    } else {
        for (what = 0; what < Realsize * 2; what++) {
            Push = 0;
        }
    }
}

/*  GETREAL  --  Get real number from user.  */

prim P_getreal()
{                                     /* prompt -- fresult istat */
    char *prompt;
    ads_real r;

    Sl(1);
    Hpc(S0);
    prompt = (char *) S0;
    Pop;
    So(Realsize + 1);
    Push = 0;
    Push = 0;
    if (ads_getreal(prompt, &r) == RTNORM) {
        SREAL0(r);
        Push = -1;
    } else {
        Push = 0;
    }
}

/*  GETINT  --  Get integer from user.  */

prim P_getint()
{                                     /* prompt -- iresult istat */
    char *prompt;
    int i;

    Sl(1);
    Hpc(S0);
    prompt = (char *) S0;
    Pop;
    So(2);
    if (ads_getint(prompt, &i) == RTNORM) {
        Push = i;
        Push = -1;
    } else {
        Push = 0;
        Push = 0;
    }
}

/*  ACOR  --  Obtain arbitrary co-ordinate.  */

static void acor(which)
  int which;
{
    So(Realsize);
    Push = 0;
    Push = 0;
    SREAL0(cturt->p[which]);
}

/*  XCOR  --  Obtain X co-ordinate.  */

prim P_xcor()
{                                     /*  -- fxcor */
    acor(X);
}

/*  YCOR  --  Obtain Y co-ordinate.  */

prim P_ycor()
{                                     /*  -- fycor */
    acor(Y);
}

/*  ZCOR  --  Obtain Z co-ordinate.  */

prim P_zcor()
{                                     /*  -- fzcor */
    acor(Z);
}

/*  DEFPRIM  --  Define primitives accessible from the ATLAS program. */

static void defprim()
{
    static struct primfcn primt[] = {

        /* Turtle stuff */

        {"0BACK",           P_back},
        {"0FORWARD",        P_forward},
        {"0GETINT",         P_getint},
        {"0GETREAL",        P_getreal},
        {"0HIDETURTLE",     P_hideturtle},
        {"0LEAVETRACKS",    P_leavetracks},
        {"0LEFT",           P_left},
        {"0LISTEN:",        P_listen},
        {"0LOAD",           P_load},
        {"0ME",             P_me},
        {"0MOUSE",          P_mouse},
        {"0PENDOWN",        P_pendown},
        {"0PENUP",          P_penup},
        {"0PICKPOINT",      P_pickpoint},
        {"0PITCH",          P_pitch},
        {"0REDRAW",         P_redraw},
        {"0RESET",          P_reset},
        {"0RIGHT",          P_right},
        {"0ROL",            P_roll},
        {"0SETHEADING",     P_setheading},
        {"0SETPOSITION",    P_setposition},
        {"0SHOWTURTLE",     P_showturtle},
        {"0TURTLE",         P_turtle},
        {"0XCOR",           P_xcor},
        {"0YAW",            P_left},
        {"0YCOR",           P_ycor},
        {"0ZCOR",           P_zcor},

        {NULL,          (codeptr) 0}
    };
#ifdef PANICPAUSE
    fprintf(stderr, "\nCLASS---WAITING FOR DEBUGGER SNAG.\n");
    sleep(60);
#endif
    atl_primdef(primt);
    cls_init();                       /* Define class system primitives */
    atlads_init();                    /* Initialise Atlas ADS primitives */
    atlgeom_init();                   /* Initialise Atlas geometry primitives*/
}

/*  MAIN  --   Main ADS transaction processor.  */

void main(argc, argv)
  int argc;
  char *argv[];
{
    int stat, cindex, scode = RSRSLT;
    char *heapspec;

    ads_init(argc, argv);             /* Initialise the application */
atl_heaplen = 5000;  /*  ** HUGE HEAP FOR DEBUGGING ** */
    if ((heapspec = getenv("CLASSHEAP")) != NULL) {
        long hl = atol(heapspec);

        atl_heaplen = max(atl_heaplen, hl);
    }
    atl_init();                       /* Initialise Atlas */
    defprim();                        /* Define our custom primitives */

    /* Main dispatch loop. */

    while (True) {

        if ((stat = ads_link(scode)) < 0) {
            V printf("Bad status from ads_link() = %d\n", stat);
            ads_exit(1);
        }

        scode = -RSRSLT;              /* Default return code */

        switch (stat) {

            case RQSAVE:              /* Drawing being saved */
                save_classvars();     /* Store any modified class variables */
                break;

            case RQEND:               /* End of drawing editor */
                save_classvars();     /* Store any modified class variables */
                /* Note fall-through */

            case RQQUIT:              /* Quit without saving changes */
                classclean();         /* Clean up all storage and reset */
                break;

            case RQTERM:              /* Terminate.  Clean up and exit. */
                ads_exit(0);

            case RQXLOAD:             /* Load functions.  Called at the start
                                         of the drawing editor.  Re-initialise
                                         the application here. */
                scode = -(funcload() ? RSRSLT : RSERR);
                if (!initacad(False)) {
                    ads_printf("\nUnable to initialise application.\n");
                }
                break;

            case RQXUNLD:             /* Application unload request. */
                save_classvars();     /* Store any modified class variables */
                break;

            case RQSUBR:              /* Evaluate external lisp function */
                cindex = ads_getfuncode();
                functional = False;

                /* Execute the command from the command table with
                   the index associated with this function. */

                if (cindex > 0) {
                    if (cindex >= 10000) {
                        cls_vcommand(cindex);
                        ads_retvoid();
                    } else {
                        cindex--;
                        (*cmdtab[cindex].cmdfunc)();
                        if ((cmdtab[cindex].cmdname[0] != '(') &&
                            !functional)
                            ads_retvoid();    /* Void result */
                    }
                }
                break;

            default:
                break;
        }
    }
}

/* FUNCLOAD  --  Load external functions into AutoLISP */

static Boolean funcload()
{
    char ccbuf[40];
    int i;

    V strcpy(ccbuf, "C:");
    for (i = 0; i < ELEMENTS(cmdtab); i++) {
        if (cmdtab[i].cmdname[0] == '(')
            ads_defun(cmdtab[i].cmdname + 1, i + 1);
        else {
            V strcpy(ccbuf + 2, cmdtab[i].cmdname);
            ads_defun(ccbuf, i + 1);
        }
    }

    return initacad(True);            /* Reset AutoCAD initialisation */
}

/*  INITACAD  --  Initialise the required modes in the AutoCAD
                  drawing.  */

static Boolean initacad(reset)
  Boolean reset;
{
    static Boolean initdone, initok;

    if (reset) {
        initdone = False;
        initok = True;
    } else {
        if (!initdone) {

#ifdef INITPAUSE
            char ipr[132];

            V ads_getstring(False, "CLASS ready to initialise: ", ipr);
#endif

            /* Reset the program modes to standard values upon
               entry to the drawing editor. */

            initdone = initok = True;

            /* If our dictionary bookmark is defined, forget everything
               defined back through it.  This gets rid of anything defined
               in the last execution of the drawing editor. */

#define BookMark    "(DICT_BOOKMARK)"

            if (atl_lookup(BookMark) != NULL) {
                V atl_eval("forget (DICT_BOOKMARK)");
            }
            V atl_vardef(BookMark, 4);

            /* Now create the shared variables we use to communicate with
               Atlas. */

            cturt = (turtle *) atl_body(atl_vardef("KELVIN", sizeof(turtle)));
#define ShareReal(x,y) x=(atl_real *)atl_body(atl_vardef((y),sizeof(atl_real)))
            ShareReal(v_turtleheight, "TURTLE.HEIGHT");
            ShareReal(v_turtlewidth,  "TURTLE.WIDTH");
            ShareReal(v_turtledepth,  "TURTLE.DEPTH");
#undef ShareReal
            P_reset();
            turtleshown = False;
            cls_edinit(AppName);      /* Perform class system initialisation */
        }
    }
    return initok;
}

/*  ATLAS1 (The "/" command)  --  Execute a single Atlas command.  */

static void atlas1()
{
    char atlstr[134];

    if ((ads_getstring(1, NULL, atlstr) == RTNORM) ||
        (strlen(atlstr) > 0))
        atl_eval(atlstr);
    ads_retvoid();
}

/*  ATLAS  --  Execute an Atlas string.  */

static void atlas()
{
    char atlstr[134];
    static char aprompt[20];

    while (True) {
        if (cturt->visible) {
            drawturtle();
            turtleshown = True;
        }
        sprintf(aprompt, "Atlas[%d]%s ", stk - stack,
            atl_comment ? "( " :
            (((heap != NULL) && state) ? ":> " : "-> "));

        if ((ads_getstring(1, aprompt, atlstr) != RTNORM) ||
            (strlen(atlstr) == 0))
            break;

        clearturtle();
        atl_eval(atlstr);
    }
    clearturtle();
    ads_retvoid();
}

#ifdef DEBUG

/*  SNAG  --  Let the debugger snag us.  */

static void snag()
{
    ads_printf("\nWhat do you know?  Somebody typed the SNAG command!\n");
#ifndef HIGHC                         /* Unix compatibility?  Fuck that! */
    sleep(15);                        /* Delay to let debugger snag us */
#endif
}

/*  UBI  --  Dump and reset user break instrumentation.  */

static int breakcalls = 0;            /* Calls on user break */

void UbI()
{
    breakcalls++;
}

static void ubi()
{
    ads_printf("%d calls on ads_usrbrk.\n", breakcalls);
    breakcalls = 0;
}
#endif


#ifdef  HIGHC

/*  Stupid High C put abort() in the same module with exit(); ADS defines
    its own exit(), so we have to define our own abort(): */

void abort(void)
{
    ads_abort("");
}

#endif  /* HIGHC */
