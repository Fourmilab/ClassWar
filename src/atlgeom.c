/*

                            A T L G E O M

          Linear algebra and geometry primitives for Atlas.

      Designed and implemented in March of 1990 by John Walker.

*/

#include "class.h"
#include "sglib.h"

#define NpushR(n)   stk += (Realsize * (n))  /* Push N reals */
#define SREAL2(x) *((atl_real *) &S5) = (x)

prim P_4vector()
{                                     /* <name> --  */
    P_create();
    Ho(4 * Realsize);
    vecget((vector *) hptr, 0.0, 0.0, 0.0);
    hptr += 4 * Realsize;
}

prim P_vecget()
{                                     /* fx fy fz 4vec --  */
    vector *v;

    Sl(Realsize * 3 + 1);
    Hpc(S0);
    v = (vector *) S0;
    Pop;
    vecget(v, REAL2, REAL1, REAL0);
    Npop(Realsize * 3);
}

prim P_vecput()
{                                     /* 4vec -- fx fy fz */
    atl_real x, y, z;

    Sl(1);
    Hpc(S0);
    vecput(&x, &y, &z, (vector *) S0);
    Pop;
    So(3 * Realsize);
    NpushR(3);
    SREAL2(x);
    SREAL1(y);
    SREAL0(z);
}

prim P_veccopy()
{                                     /* 4vsource 4vdest --  */
    Sl(2);
    Hpc(S0);
    Hpc(S1);
    veccopy((vector *) S0, (vector *) S1);
    Pop2;
}

prim P_vecxmat()
{                                     /* 4vsource matrix 4vdest -- */
    Sl(3);
    Hpc(S0);
    Hpc(S1);
    Hpc(S2);
    vecxmat((vector *) S0, (vector *) S2, (matrix *) S1);
    Npop(3);
}

prim P_pointget()
{                                     /* fx fy fz point --  */
    point *p;

    Sl(Realsize * 3 + 1);
    Hpc(S0);
    p = (point *) S0;
    Pop;
    pointget(p, REAL2, REAL1, REAL0);
    Npop(Realsize * 3);
}

prim P_pointcopy()
{                                     /* psource pdest --  */
    Sl(2);
    Hpc(S0);
    Hpc(S1);
    pointcopy((point *) S0, (point *) S1);
    Pop2;
}

prim P_vecdot()
{                                     /* p1 p2 -- fdot */
    double dot;

    Sl(2);
    Hpc(S0);
    Hpc(S1);
    dot = vecdot((point *) S1, (point *) S0);
    SREAL0(dot);
}

prim P_veccross()
{                                     /* p1 p2 pcross --  */
    Sl(3);
    Hpc(S0);
    Hpc(S1);
    Hpc(S2);
    veccross((point *) S0, (point *) S2, (point *) S1);
    Npop(3);
}

prim P_vecadd()
{                                     /* p1 p2 psum --  */
    Sl(3);
    Hpc(S0);
    Hpc(S1);
    Hpc(S2);
    vecadd((point *) S0, (point *) S2, (point *) S1);
    Npop(3);
}

prim P_vecsub()
{                                     /* p1 p2 pdif --  */
    Sl(3);
    Hpc(S0);
    Hpc(S1);
    Hpc(S2);
    vecsub((point *) S0, (point *) S2, (point *) S1);
    Npop(3);
}

prim P_vecscal()
{                                     /* p1 fscale presult --  */
    point *r;

    Sl(Realsize + 2);
    Hpc(S0);
    Hpc(S3);
    r = (point *) S0;
    Pop;
    vecscal(r, (point *) S2, REAL0);
}

prim P_vecmag()
{                                     /* p1 -- fmag */
    Sl(1);
    So(1);
    Hpc(S0);
    Push = 0;
    SREAL0(vecmag((point *) S1));
}

prim P_vecnorm()
{                                     /* p1 presult --  */
    Sl(2);
    Hpc(S0);
    Hpc(S1);
    vecnorm((point *) S0, (point *) S1);
    Pop2;
}

prim P_vecprint()
{                                     /* 4vec --  */
    Sl(1);
    Hpc(S0);
    vecprint((vector *) S0);
    Pop;
}

prim P_matrix()
{                                     /* <name> --  */
    P_create();
    Ho(16 * Realsize);
    matident((matrix *) hptr);
    hptr += 16 * Realsize;
}

prim P_matmul()
{                                     /* m1 m2 mresult --  */
    Sl(3);
    Hpc(S0);
    Hpc(S1);
    Hpc(S2);
    matmul((matrix *) S0, (matrix *) S2, (matrix *) S1);
    Npop(3);
}

prim P_matident()
{                                     /* m1 -- */
    Sl(1);
    Hpc(S0);
    matident((matrix *) S0);
    Pop;
}

prim P_matcopy()
{                                     /* msource mdest --  */
    Sl(2);
    Hpc(S0);
    Hpc(S1);
    matcopy((matrix *) S0, (matrix *) S1);
    Pop2;
}

prim P_matprint()
{                                     /* matrix --  */
    Sl(1);
    Hpc(S0);
    matprint((matrix *) S0);
    Pop;
}

prim P_mattran()
{                                     /* ftx fty ftz matrix --  */
    matrix *m;

    Sl(3 * Realsize + 1);
    Hpc(S0);
    m = (matrix *) S0;
    Pop;
    mattran(m, REAL2, REAL1, REAL0);
    Npop(3 * Realsize);
}

prim P_matscal()
{                                     /* sx sy sz matrix --  */
    matrix *m;

    Sl(3 * Realsize + 1);
    Hpc(S0);
    m = (matrix *) S0;
    Pop;
    matscal(m, REAL2, REAL1, REAL0);
    Npop(3 * Realsize);
}

prim P_matrot()
{                                     /* theta axis m --  */
    Sl(Realsize + 2);
    Hpc(S0);
    matrot((matrix *) S0, S1, REAL1);
    Npop(4);
}

prim P_matpers()
{                                     /* alpha zn zf m --  */
    matrix *m;

    Sl(3 * Realsize + 1);
    Hpc(S0);
    m = (matrix *) S0;
    Pop;
    matpers(m, REAL2, REAL1, REAL0);
    Npop(6);
}

prim P_matorie()
{                                     /* a b c d e f p q r m -- */
    double mo[9];
    matrix *m;
    int i;

    Sl(Realsize * 9 + 1);
    m = (matrix *) S0;
    Pop;
    for (i = 8; i >= 0; i--) {
        mo[i] = REAL0;
        Realpop;
    }
    matorie(m, mo, mo + 1, mo + 2, mo + 3, mo + 4, mo + 5,
                   mo + 6, mo + 7, mo + 8);
}

prim P_xtran()
{                                     /* tx ty tz --  */
    Sl(Realsize * 3);
    tran(REAL2, REAL1, REAL0);
    Npop(Realsize * 3);
}

prim P_xscal()
{                                     /* sx sy sz --  */
    Sl(Realsize * 3);
    scal(REAL2, REAL1, REAL0);
    Npop(Realsize * 3);
}

prim P_xrot()
{                                     /* theta axis --  */
    int axis;

    Sl(Realsize + 1);
    axis = S0;
    Pop;
    rot(REAL0, axis);
    Realpop;
}

prim P_xpers()
{
    Sl(Realsize * 3);
    pers(S2, S1, S0);
    Npop(Realsize * 2);
}

prim P_xorie()
{                                     /* a b c d e f p q r --  */
    double mo[9];
    int i;

    Sl(Realsize * 9);
    for (i = 8; i >= 0; i--) {
        mo[i] = REAL0;
        Realpop;
    }
    orie(mo, mo + 1, mo + 2, mo + 3, mo + 4, mo + 5,
             mo + 6, mo + 7, mo + 8);
}

prim P_xpush()
{                                     /*  --  */
    push();
}

prim P_xpop()
{                                     /*  --  */
    pop();
}

prim P_xthen()
{                                     /*  --  */
    then();
}

prim P_xreset()                       /*  --  */
{
    startup();
}

prim P_drawvec()
{                                     /* p1 p2 --  */
    double *p1, *p2;

    Sl(2);
    Hpc(S0);
    Hpc(S1);
    p1 = (double *) S1;
    p2 = (double *) S0;
    drawvec(p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z]);
    Pop2;
}

prim P_drawface()
{                                     /* n p1 p2 p3 p4 visbit --  */
    double *p1, *p2, *p3, *p4;

    Sl(6);
    Hpc(S1);
    Hpc(S2);
    Hpc(S3);
    Hpc(S4);
    p1 = (double *) S4;
    p2 = (double *) S3;
    p3 = (double *) S2;
    p4 = (double *) S1;
    drawface((int) S5, p1[X], p1[Y], p1[Z], p2[X], p2[Y], p2[Z],
                       p3[X], p3[Y], p3[Z], p4[X], p4[Y], p4[Z], (int) S0);
    Npop(6);
}

prim P_gplane()
{                                     /* gpfancy --  */
    Sl(1);
    gplane((int) S0);
    Pop;
}

prim P_pnt()
{                                     /* x y z n --  */
    int n;

    Sl(3 * Realsize + 1);
    n = S0;
    Pop;
    pnt(n, REAL2, REAL1, REAL0);
    Npop(3 * Realsize);
}

prim P_poly()
{                                     /* 0 v1 v2 v3 ... vn --  */
#define VMAX 30
    int varr[VMAX];
    int i, n;
    stackitem *ssc = stk - 1;

    for (i = 0; i < VMAX; i++)
        varr[i] = 0;

    for (n = 0; (ssc >= stackbot) && (*ssc != 0); ssc--, n++) ;

#ifndef NOMEMCHECK
    if (ssc < stackbot) {
        stakunder();
        return;
    }
#endif

    if (n > VMAX) {
        atl_error("Too many vertices in polygon");
        return;
    }

    for (i = 0; i < n; i++)
        varr[i] = *(++ssc);

    /* I agree, this could be made a tad more elegant, but I don't
       feel like ripping up SGLIB's poly() to create a variant that
       accepts a vertex array in addition to a varargs list. */

    poly(varr[0],  varr[1],  varr[2],  varr[3],  varr[4],
         varr[5],  varr[6],  varr[7],  varr[8],  varr[9],
         varr[10], varr[11], varr[12], varr[13], varr[14],
         varr[15], varr[16], varr[17], varr[18], varr[19],
         varr[20], varr[21], varr[22], varr[23], varr[24],
         varr[25], varr[26], varr[27], varr[28], varr[29]);

    Npop(n + 1);
}

prim P_raton()
{                                     /*  --  */
    raton();
}

prim P_ratoff()
{                                     /*  --  */
    ratoff();
}

prim P_cube()
{                                     /*  --  */
    cube();
}

prim P_sphere()
{                                     /*  --  */
    sphere();
}

prim P_tetrahedron()
{                                     /*  --  */
    tetrahedron();
}

prim P_octahedron()
{                                     /*  --  */
    octahedron();
}

prim P_dodecahedron()
{                                     /*  --  */
    dodecahedron();
}

prim P_icosahedron()
{                                     /*  --  */
    icosahedron();
}

/*  ATLGEOM_INIT  --  Define primitives accessible from the ATLAS program. */

void atlgeom_init()
{
    static struct primfcn primt[] = {

        {"0MATRIX",         P_matrix},
        {"0MATMUL",         P_matmul},
        {"0MATIDENT",       P_matident},
        {"0MATCOPY",        P_matcopy},
        {"0MATPRINT",       P_matprint},
        {"0MATTRAN",        P_mattran},
        {"0MATSCAL",        P_matscal},
        {"0MATROT",         P_matrot},
        {"0MATPERS",        P_matpers},
        {"0MATORIE",        P_matorie},

        {"04VECTOR",        P_4vector},
        {"0VECCOPY",        P_veccopy},
        {"0VECGET",         P_vecget},
        {"0VECPUT",         P_vecput},
        {"0VECXMAT",        P_vecxmat},
        {"0POINTGET",       P_pointget},
        {"0POINTCOPY",      P_pointcopy},
        {"0VECDOT",         P_vecdot},
        {"0VECCROSS",       P_veccross},
        {"0VECADD",         P_vecadd},
        {"0VECSUB",         P_vecsub},
        {"0VECSCAL",        P_vecscal},
        {"0VECMAG",         P_vecmag},
        {"0VECNORM",        P_vecnorm},
        {"0VECPRINT",       P_vecprint},

        {"0XTRAN",          P_xtran},
        {"0XSCAL",          P_xscal},
        {"0XROT",           P_xrot},
        {"0XPERS",          P_xpers},
        {"0XORIE",          P_xorie},
        {"0XPUSH",          P_xpush},
        {"0XPOP",           P_xpop},
        {"0XTHEN",          P_xthen},
        {"0XRESET",         P_xreset},

        {"0DRAWVEC",        P_drawvec},
        {"0DRAWFACE",       P_drawface},
        {"0GPLANE",         P_gplane},
        {"0PNT",            P_pnt},
        {"0POLY",           P_poly},
        {"0RATON",          P_raton},
        {"0RATOFF",         P_ratoff},

        {"0TETRAHEDRON",    P_tetrahedron},
        {"0CUBE",           P_cube},
        {"0OCTAHEDRON",     P_octahedron},
        {"0DODECAHEDRON",   P_dodecahedron},
        {"0ICOSAHEDRON",    P_icosahedron},

        {"0SPHERE",         P_sphere},

        {NULL,              (codeptr) 0}
    };
    atl_primdef(primt);
    startup();
}
