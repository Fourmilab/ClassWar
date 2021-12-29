/* #define TESTIT */
/*

        The Simple Graphics Library

        Implemented by John Walker in March of 1988.

        This is a simple three dimensional transformation and modeling
        library based on Jim Blinn's modeling primitives, as documented
        in Jim Blinn's Corner, IEEE Computer Graphics and Applications,
        October 1987, and subsequent columns.

*/

#include "class.h"

#include "sglib.h"
#ifdef ANSIARGS
#include "stdarg.h"                /* Brain-dead compiler */
#else
#include <varargs.h>
#endif

#define INVISEG                    /* Enable invisible edge support */

#ifndef HIGHC                      /* I mean, like who needs this shit? */
extern char *malloc();
#endif /* De-Reamed again */

#define IPDBL    256               /* Initial point database size */

/*  Coordinate system transforms  */

static matrix ct = {               /*  Current transformation matrix */
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
};

struct ctstack {
        struct ctstack *ctlast;    /* Previous coordinate system */
        matrix ctsm;               /* Saved coordinate system */
};

static struct ctstack *cts = NULL; /* Coordinate system stack */
static int ctdepth = 0;            /* Coordinate system nesting depth */

/*  The point database  */

static point *pdb = NULL;          /* Point database array */
static int pdbl = 0;               /* Point database length */
static int pdbratmax = 0;          /* Highest point used in rat nest mesh */
static int ratmode = FALSE;        /* Rat nest mesh generation underway */

/*  General vector routines  */

/*  VECGET  --  Set vector from X, Y, and Z coordinates  */

void vecget(v, x, y, z)
vector v;
double x, y, z;
{
        v[X] = x;
        v[Y] = y;
        v[Z] = z;
        v[T] = 1.0;
}

/*  VECPUT  --  Store vector into X, Y, and Z coordinates  */

void vecput(x, y, z, v)
double *x, *y, *z;
vector v;
{
        double w;

        w = v[T];
        *x = v[X] / w;
        *y = v[Y] / w;
        *z = v[Z] / w;
}

/*  VECCOPY  --  Copy vector to another  */

void veccopy(vo, v)
vector vo, v;
{
        register int i;

        for (i = X; i <= T; i++)
           vo[i] = v[i];
}

/*  VECXMAT  --  Multiply a vector by a matrix  */

void vecxmat(vo, v, m)
vector vo, v;
matrix m;
{
        register int i, j;
        register double sum;

        for (i = 0; i < 4; i++) {
           sum = 0;
           for (j = 0; j < 4; j++) {
              sum += v[j] * m[j][i];
           }
           vo[i] = sum;
        }
}

/*  Vector algebra routines which operate on points  */

/*  POINTGET  --  Set point from X, Y, and Z coordinates  */

void pointget(p, x, y, z)
point p;
double x, y, z;
{
        p[X] = x;
        p[Y] = y;
        p[Z] = z;
}

/*  POINTCOPY  --  Copy point to another  */

void pointcopy(po, p)
point po, p;
{
        po[X] = p[X];
        po[Y] = p[Y];
        po[Z] = p[Z];
}

/*  VECDOT  --  Computes the dot (inner) product of two vectors and
                returns the result as a double.  Since this will frequently
                be used on points as well as vectors, only the first
                three terms are computed.  */

double vecdot(a, b)
point a, b;
{
        int i;
        double product;

        product = 0.0;
        for (i = 0; i < 3; i++) {
           product += a[i] * b[i];
        }

        return product;
}

/*  VECCROSS  --  Computes the cross product of two vectors and stores
                  the result in a third.  This actually works on points;
                  if a vector is passed, the fourth item is ignored.  */

void veccross(o, a, b)
point o, a, b;
{
        point r;

        r[X] = a[Y] * b[Z] - a[Z] * b[Y];
        r[Y] = a[Z] * b[X] - a[X] * b[Z];
        r[Z] = a[X] * b[Y] - a[Y] * b[X];

        pointcopy(o, r);
}

/*  VECADD  --  Add two vectors and store the sum in a third.
                Operates on points.  */

void vecadd(o, a, b)
point o, a, b;
{
        o[X] = a[X] + b[X];
        o[Y] = a[Y] + b[Y];
        o[Z] = a[Z] + b[Z];
}

/*  VECSUB  --  Subtracts vector b from vector a and stores the
                result in vector o.  Expects points as arguments. */

void vecsub(o, a, b)
point o, a, b;
{
        o[X] = a[X] - b[X];
        o[Y] = a[Y] - b[Y];
        o[Z] = a[Z] - b[Z];
}

/*  VECSCAL  --  Multiply vector by a scalar and store the result
                 in a second vector.  Expects points.  */

void vecscal(o, a, s)
point o, a;
double s;
{
        o[X] = a[X] * s;
        o[Y] = a[Y] * s;
        o[Z] = a[Z] * s;
}

/*  VECMAG  --  Returns magnitude of a vector.  This expects a point
                and uses only the first three terms.  */

double vecmag(a)
point a;
{
        return sqrt(a[X] * a[X] + a[Y] * a[Y] + a[Z] * a[Z]);
}

/*  VECNORM  --  Normalise vector and store normalised result in
                 a second vector.  Works on points.  */

void vecnorm(o, a)
point o, a;
{
        vecscal(o, a, 1.0 / vecmag(a));
}

/*  VECPRINT  --  Print a vector  */

void vecprint(v)
vector v;
{
        int j;

        printf("+-----------------------------------------+\n");
        printf("|");
        for (j = 0; j < 4; j++) {
           printf(" %9.4f", v[j]);
        }
        printf(" |\n");
        printf("+-----------------------------------------+\n");
}

/*  General matrix routines  */

/*  MATMUL  --  Multiply two 4 X 4 matrices, storing copy in a third.  */

void matmul(o, a, b)
matrix o, a, b;
{
        register int i, j, k;
        register double sum;

        for (i = 0; i < 4; i++) {
           for (k = 0; k < 4; k++) {
              sum = 0.0;
              for (j = 0; j < 4; j++) {
                 sum += a[i][j] * b[j][k];
              }
              o[i][k] = sum;
           }
        }
}

/*  MATIDENT  --  Set a matrix to the identity matrix  */

void matident(a)
matrix a;
{
        register int i, j;

        for (i = 0; i < 4; i++) {
           for (j = 0; j < 4; j++) {
              a[i][j] = (i == j) ? 1.0 : 0.0;
           }
        }
}

/*  MATCOPY  --  Copy a matrix to another  */

void matcopy(o, a)
matrix o, a;
{
        register int i, j;

        for (i = 0; i < 4; i++) {
           for (j = 0; j < 4; j++) {
              o[i][j] = a[i][j];
           }
        }
}

/*  MATPRINT  --  Print a matrix  */

void matprint(a)
matrix a;
{
        int i, j;

        printf("+-----------------------------------------+\n");
        for (i = 0; i < 4; i++) {
           printf("|");
           for (j = 0; j < 4; j++) {
              printf(" %9.4f", a[i][j]);
           }
           printf(" |\n");
        }
        printf("+-----------------------------------------+\n");
}

/*  Transformation matrix construction routines  */

/*  MATTRAN  --  Build translation matrix  */

void mattran(m, tx, ty, tz)
matrix m;
double tx, ty, tz;
{
        matident(m);
        m[T][X] = tx;
        m[T][Y] = ty;
        m[T][Z] = tz;
}

/*  MATSCAL  --  Build scaling matrix  */

void matscal(m, sx, sy, sz)
matrix m;
double sx, sy, sz;
{
        matident(m);
        m[X][X] = sx;
        m[Y][Y] = sy;
        m[Z][Z] = sz;
}

/*  MATROT  --  Build rotation matrix.  THETA is the rotation
                angle, in radians, and J is the axis about which
                the rotation is to be performed, expressed as one
                 of the manifest constants X, Y, or Z. */

void matrot(m, theta, j)
matrix m;
double theta;
int j;
{
        double s, c;

        s = sin(theta);
        c = cos(theta);

        matident(m);
        switch (j) {

           case X:
              m[1][1] = m[2][2] = c;
              m[1][2] = -s;
              m[2][1] = s;
              break;

           case Y:
              m[0][0] = m[2][2] = c;
              m[0][2] = s;
              m[2][0] = -s;
              break;

           case Z:
              m[0][0] = m[1][1] = c;
              m[0][1] = -s;
              m[1][0] = s;
              break;

           default:
              printf("\nInvalid axis (J) argument %d to matrot.\n",
                 j);
              abort();
        }
}

/*  MATPERS  --  Build perspective transformation matrix.  ALPHA is
                 the field of view, ZN is the near clipping plane,
                 and ZF is the far clipping plane.  */

void matpers(m, alpha, zn, zf)
matrix m;
double alpha, zn, zf;
{
        double s, c, q;

        s = sin(alpha / 2.0);
        c = cos(alpha / 2.0);
        q = s / (1.0 - zn / zf);
        matident(m);
        m[X][X] = m[Y][Y] = c;
        m[Z][Z] = q;
        m[T][Z] = - q * zn;
        m[Z][T] = s;
        m[T][T] = 0.0;
}

/*  MATORIE  --  Specify explicit orientation  */

void matorie(m, a, b, c, d, e, f, p, q, r)
matrix m;
double a, b, c, d, e, f, p, q, r;
{
        matident(m);
        m[0][0] = a;
        m[1][0] = b;
        m[2][0] = c;
        m[0][1] = d;
        m[1][1] = e;
        m[2][1] = f;
        m[0][2] = p;
        m[1][2] = q;
        m[2][2] = r;
}

/*  MATSHAD  --  Specify matrix for fake shadow generation.  The
                 light source is at X, Y, and Z, and W is FALSE
                 for a light source at infinity and TRUE for a
                 local light source. */

void matshad(m, x, y, z, w)
matrix m;
double x, y, z;
int w;
{
        matident(m);
        m[0][0] = z;
        m[1][1] = z;
        m[2][0] = -x;
        m[2][1] = -y;
        m[2][2] = 0.0;
        m[2][3] = w ? -1.0 : 0.0;
        m[3][3] = z;
}

/*  Current coordinate system transformation composition routines  */

/*  TRAN  --  Compose translation matrix  */

void tran(tx, ty, tz)
double tx, ty, tz;
{
        matrix m, m1;

        mattran(m, tx, ty, tz);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}

/*  SCAL  --  Build scaling matrix  */

void scal(sx, sy, sz)
double sx, sy, sz;
{
        matrix m, m1;

        matscal(m, sx, sy, sz);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}

/*  ROT  --  Build rotation matrix.  THETA is the rotation
             angle, in radians, and J is the axis about which
             the rotation is to be performed, expressed as one
             of the manifest constants X, Y, or Z. */

void rot(theta, j)
double theta;
int j;
{
        matrix m, m1;

        matrot(m, theta, j);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}

/*  PERS  --  Build perspective transformation matrix.  ALPHA is
              the field of view, ZN is the near clipping plane,
              and ZF is the far clipping plane.  */

void pers(alpha, zn, zf)
double alpha, zn, zf;
{
        matrix m, m1;

        matpers(m, alpha, zn, zf);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}

/*  ORIE  --  Specify explicit orientation  */

void orie(a, b, c, d, e, f, p, q, r)
double a, b, c, d, e, f, p, q, r;
{
        matrix m, m1;

        matorie(m, a, b, c, d, e, f, p, q, r);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}

/*  SHAD  --  Compose matrix for fake shadow generation.  The
              light source is at X, Y, and Z, and W is FALSE
              for a light source at infinity and TRUE for a
              local light source. */

void shad(x, y, z, w)
double x, y, z;
int w;
{
        matrix m, m1;

        matshad(m, x, y, z, w);
        matmul(m1, m, ct);
        matcopy(ct, m1);
}


/*  Coordinate system push and pop routines  */

/*  PUSH  --  Save coordinate system  */

void push()
{
        struct ctstack *c;

        c = (struct ctstack *) sgalloc(sizeof(struct ctstack));
        c->ctlast = cts;
        matcopy(c->ctsm, ct);
        cts = c;
        ctdepth++;
}

/*  POP  --  Restore coordinate system  */

void pop()
{
        struct ctstack *c;

        if (ctdepth <= 0) {
           printf("\nCoordinate system popped when none pushed.\n");
           abort();
        }
        c = cts;
        cts = c->ctlast;
        matcopy(ct, c->ctsm);
        free(c);
        ctdepth--;
}

/*  THEN  --  Pop old coordinate system, push new one  */

void then()
{
        pop();
        push();
}

/*  Current transformation manipulation routines.  */

/*  TRANSPT  -- Transform a point by the current transformation matrix.  */

void transpt(po, pin)
point po, pin;
{
        vector v, vt;

        vecget(v, pin[X], pin[Y], pin[Z]);
        vecxmat(vt, v, ct);
        vecput(&po[X], &po[Y], &po[Z], vt);
}

/*  Drawing routines  */

/*  DRAWVEC  --  Draw a vector  */

void drawvec(x1, y1, z1, x2, y2, z2)
double x1, y1, z1, x2, y2, z2;
{
        vector vl1, vl2, vt1, vt2;

        vecget(vl1, x1, y1, z1);
        vecget(vl2, x2, y2, z2);
        vecxmat(vt1, vl1, ct);
        vecxmat(vt2, vl2, ct);
        vecput(&x1, &y1, &z1, vt1);
        vecput(&x2, &y2, &z2, vt2);
        defent("LINE");
        tackatt();
        tackpoint(10, x1, y1, z1);
        tackpoint(11, x2, y2, z2);
        makent();
}

/*  DRAWFACE  --  Draw a polygonal face  */

void drawface(n, x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, visbit)
int n;
double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
int visbit;
{
        vector vl1, vl2, vl3, vl4, vt1, vt2, vt3, vt4;

        vecget(vl1, x1, y1, z1);
        vecget(vl2, x2, y2, z2);
        vecget(vl3, x3, y3, z3);
        if (n > 3)
           vecget(vl4, x4, y4, z4);
        vecxmat(vt1, vl1, ct);
        vecxmat(vt2, vl2, ct);
        vecxmat(vt3, vl3, ct);
        if (n > 3)
           vecxmat(vt4, vl4, ct);
        vecput(&x1, &y1, &z1, vt1);
        vecput(&x2, &y2, &z2, vt2);
        vecput(&x3, &y3, &z3, vt3);
        if (n > 3)
           vecput(&x4, &y4, &z4, vt4);
       if (wireframe) {
          defent("POLYLINE");
          tackatt();
          tackint(66, 1);
          tackint(70, 8 + 1);         /* Closed 3D Polyline */
          makent();
          defent("VERTEX");
          tackatt();
          tackpoint(10, x1, y1, z1);
          tackint(70, 32);
          makent();
          defent("VERTEX");
          tackatt();
          tackpoint(10, x2, y2, z2);
          tackint(70, 32);
          makent();
          defent("VERTEX");
          tackatt();
          tackpoint(10, x3, y3, z3);
          tackint(70, 32);
          makent();
          if (n > 3) {
             defent("VERTEX");
             tackatt();
             tackpoint(10, x4, y4, z4);
             tackint(70, 32);
             makent();
          }
          defent("SEQEND");
          tackatt();
          makent();
       } else {
          defent("3DFACE");
          tackatt();
          tackpoint(10, x1, y1, z1);
          tackpoint(11, x2, y2, z2);
          tackpoint(12, x3, y3, z3);
          if (n > 3) {
             tackpoint(13, x4, y4, z4);
          } else {
             tackpoint(13, x3, y3, z3);
          }
          tackint(70, visbit | (((n == 3) && (visbit & 4)) ? 8 : 0));
          makent();
       }
}

/*  DRAWMESH  --  Start drawing polygon mesh.  The argument specify the
                  M and N size of the mesh, and whether the mesh is
                  closed in the M or N directions. */

static void drawmesh(m, n, mclosed, nclosed)
int m, n, mclosed, nclosed;
{
        defent("POLYLINE");
        tackatt();
        tackint(66, 1);
        tackint(70, 16 | (mclosed ? 1 : 0) | (nclosed ? 32 : 0));
        tackint(71, m);
        tackint(72, n);
        makent();
}

/*  DRAWMPT  --  Emit next point for mesh  */

static void drawmpt(x, y, z)
double x, y, z;
{
        vector vl1, vt1;

        vecget(vl1, x, y, z);
        vecxmat(vt1, vl1, ct);
        vecput(&x, &y, &z, vt1);
        defent("VERTEX");
        tackatt();
        tackpoint(10, x, y, z);
        tackint(70, 64);
        makent();
}

/*  DRAWMEND  --  End mesh acquisition  */

static void drawmend()
{
        defent("SEQEND");
        tackatt();
        makent();
}

/*  ACADCOLOUR  --  Set AutoCAD colour.  */

void acadcolour(c)
int c;
{
        drawcolour = c;               /* Set current drawing colour */
}

/*  Vertex-specified polygon routines  */

/*  PNT  --  Stores a point, specified by X, Y, and Z coordinates,
             into the point database as point number PN.  The point
             database is acquired if this is the first point, and
             expanded if this point has a number larger than the
             current database.  */

void pnt(pn, x, y, z)
int pn;
double x, y, z;
{
        point *opdb;
        int i, j;

        if (pdb == NULL) {
           pdb = (point *) sgalloc((sizeof(point)) *
              (pdbl = ((pn < IPDBL) ? IPDBL : pn + IPDBL)));
        }
        if (pn >= pdbl) {
           opdb = pdb;
           pdb = (point *) sgalloc((sizeof(point)) * (pn + IPDBL));
           for (i = 0; i < pdbl; i++) {
              for (j = X; j <= Z; j++) {
                 pdb[i][j] = opdb[i][j];
              }
           }
           free(opdb);
           pdbl = pn + IPDBL;
        }
        if (pn > pdbratmax)
            pdbratmax = pn;
        pdb[pn][X] = x;
        pdb[pn][Y] = y;
        pdb[pn][Z] = z;
}

/*  RATON  --  Start generation of a rat nest mesh.  */

void raton()
{
    int i;

    defent("POLYLINE");
    tackatt();
    tackint(66, 1);
    tackint(70, 64);
    tackint(71, pdbratmax);
    tackint(72, 1);                   /* Lies, all lies. */
    makent();

    for (i = 0; i < pdbratmax; i++) {
        vector v, vt;
        double tx, ty, tz;

        vecget(v, pdb[i + 1][X], pdb[i + 1][Y], pdb[i + 1][Z]);
        vecxmat(vt, v, ct);
        vecput(&tx, &ty, &tz, vt);
        defent("VERTEX");
        tackatt();
        tackpoint(10, tx, ty, tz);
        tackint(70, 128 + 64);
        makent();
    }
    pdbratmax = 0;
    ratmode = TRUE;
}

/*  RATOFF  --  Complete generation of a rat nest mesh.  */

void ratoff()
{
    defent("SEQEND");
    tackatt();
    makent();
    ratmode = FALSE;
}

/*  POLY  --  Draws a polygon with vertices chosen from the point
              database by indices in a variable argument list,
              terminated by zero.  */

#ifdef ANSIARGS
void poly(int v0, ...)
#else
void poly(va_alist)
va_dcl
#endif
{
        int nvert, i, cv, stella, vtab[4];
        double l1, l2;
        point pavg, v1, v2, xvec;
        va_list vp;

        /* Count vertices in this polygon */

        stella = stellation != 0.0;  /* Generating stellated polygon ? */
        if (stella) {
           pointget(pavg, 0.0, 0.0, 0.0);
        }
#ifdef ANSIARGS
        va_start(vp, v0);
        for (nvert = 0;
            (cv = ((nvert == 0) ? v0 : va_arg(vp, int))) != 0; nvert++) {
#else
        va_start(vp);
        for (nvert = 0; (cv = va_arg(vp, int)) != 0; nvert++) {
#endif
           if (nvert < 4)
              vtab[nvert] = cv;
           if (stella) {
              vecadd(pavg, pavg, pdb[abs(cv)]);
           }
        }
        va_end(vp);

#define pta(j) pdb[abs(vtab[j])][X],pdb[abs(vtab[j])][Y],pdb[abs(vtab[j])][Z]

        /* If stellation is enabled, take the centre of the face (calculated
           as the average of the face point co-ordinates, then displace
           by the stellation factor times the length of the first edge
           of the polygon.  The direction of the stellation is found
           from the normal determined by the cross product of the first two
           edges. */

        if (stella) {
           vecscal(pavg, pavg, 1.0 / ((double) nvert));
#ifdef ANSIARGS
           va_start(vp, v0);
#else
           va_start(vp);
#endif
           vtab[0] = va_arg(vp, int); /* Get first vertex */
           vtab[1] = va_arg(vp, int); /* ...second vertex  */
           vtab[2] = va_arg(vp, int); /* ...and third vertex */
           vtab[3] = vtab[0];      /* Save first vertex for closure */
           vecsub(v1, pdb[vtab[0]], pdb[vtab[1]]);
           vecsub(v2, pdb[vtab[2]], pdb[vtab[1]]);
           veccross(xvec, v2, v1);
           vecnorm(xvec, xvec);

           l1 = vecmag(v1);
           vecsub(v1, pdb[vtab[0]], pavg);
           l2 = vecmag(v1);

           vecscal(xvec, xvec, stellation * sqrt((l1 * l1) - (l2 * l2)));
           vecadd(pavg, xvec, pavg);

           drawface(3, pta(0), pta(1), pavg[X], pavg[Y], pavg[Z],
              pta(0), 0);
           vtab[0] = vtab[1];
           vtab[1] = vtab[2];
           drawface(3, pta(0), pta(1), pavg[X], pavg[Y], pavg[Z],
              pta(0), 0);
           for (i = 2; i < (nvert - 1); i++) {
              vtab[0] = vtab[1];
#ifdef ANSIARGS
              vtab[1] = (i == 2) ? v0 : va_arg(vp, int);
#else
              vtab[1] = va_arg(vp, int);
#endif
              drawface(3, pta(0), pta(1), pavg[X], pavg[Y], pavg[Z],
                 pta(0), 0);
           }
           drawface(3, pta(1), pta(3), pavg[X], pavg[Y], pavg[Z],
              pta(1), 0);

           va_end(vp);
        } else {

           if (wireframe) {
#ifdef ANSIARGS
              va_start(vp, v0);
              vtab[0] = v0;
#else
              va_start(vp);
              vtab[0] = va_arg(vp, int); /* Get first vertex */
#endif
              vtab[3] = vtab[0];      /* Save first vertex for closure */
              while ((vtab[1] = va_arg(vp, int)) != 0) {
                 if (vtab[0] > 0) {
                    drawvec(
                       pta(0), pta(1)
                    );
                 }
                 vtab[0] = vtab[1];
              }

              /* Close the polygon */

              if (vtab[0] > 0) {
                 drawvec(
                    pta(0), pta(3)
                 );
              }
              va_end(vp);
           } else {

              if (ratmode) {
                    if (nvert < 4) {
                        int i;

                        for (i = nvert; i < 4; i++)
                            vtab[i] = 0;
                    }
                    defent("VERTEX");
                    tackatt();
                    tackpoint(10, 0.0, 0.0, 0.0);
                    tackint(70, 128);
                    tackint(71, vtab[0]);
                    tackint(72, vtab[1]);
                    tackint(73, vtab[2]);
                    tackint(74, nvert > 4 ? -abs(vtab[3]) : vtab[3]);
                    makent();

                    /* If we have more than four vertices, output the rest as
                       triangular slices */

                    if (nvert > 4) {
#ifdef ANSIARGS
                       va_start(vp, v0);
                       for (i = 0; i < 3; i++) {
#else
                       va_start(vp);
                       for (i = 0; i < 4; i++) {
#endif
                          cv = va_arg(vp, int);  /* Burn the first 4 vertices */
                       }
                       for (i = 4; i < nvert; i++) {
                          vtab[2] = vtab[3];
                          vtab[3] = va_arg(vp, int);
                          defent("VERTEX");
                          tackatt();
                          tackpoint(10, 0.0, 0.0, 0.0);
                          tackint(70, 128);
                          tackint(71, -abs(vtab[0]));
                          tackint(72, vtab[2]);
                          tackint(73, (i == (nvert - 1)) ?
                            vtab[3] : -abs(vtab[3]));
                          tackint(74, 0);
                          makent();
                       }
                       va_end(vp);
                    }
              } else {

                 /* Output initial face.  If we have four vertices or less,
                    this is all we need to do. */

                    if (nvert < 4)
                       vtab[3] = vtab[2];       /* Was = vtab[0] */
                    drawface(nvert > 4 ? 4 : nvert,
                       pta(0), pta(1), pta(2), pta(3),
                       (vtab[0] < 0 ? 1 : 0) |
                       (vtab[1] < 0 ? 2 : 0) |
                       (vtab[2] < 0 ? 4 : 0) |
                       (vtab[3] < 0 ? 8 : 0) |
                       (nvert > 4 ? 8 : 0));

                    /* If we have more than four vertices, output the rest as
                       triangular slices */

                    if (nvert > 4) {
#ifdef ANSIARGS
                       va_start(vp, v0);
                       for (i = 0; i < 3; i++) {
#else
                       va_start(vp);
                       for (i = 0; i < 4; i++) {
#endif
                          cv = va_arg(vp, int);  /* Burn the first 4 vertices */
                       }
                       for (i = 4; i < nvert; i++) {
                          vtab[2] = vtab[3];
                          vtab[3] = va_arg(vp, int);
                          drawface(3,
                             pta(0), pta(2), pta(3), pta(3),
                             1 | (vtab[2] < 0 ? 2 : 0) |
                                 ((i == (nvert - 1)) ? 0 : 12));
                       }
                       va_end(vp);
                    }
                 }
              }
        }
#undef pta
}

/*  Primitive objects  */

/*  GPLANE  --  Ground plane  */

void gplane(gpfancy)
int gpfancy;
{
        double x, y;
        int i;

        if (gpfancy) {

           /* Draw linoleum floor tiles in red and green */

           i = 0;
           for (x = -2.0; x < 1.5; x += 1.0) {
              i ^= 1;
              for (y = -2.0; y < 1.5; y += 1.0) {
                 i ^= 1;
                 acadcolour(i ? 1 : 3);
                 drawface(4, x, y, 0.0, x, y + 1.0, 0.0,
                             x + 1.0, y + 1.0, 0.0, x + 1.0, y, 0.0, 0);
              }
           }

           /* Draw X and Y axes */

#define O 0.001
           acadcolour(2);
           drawface(4, -2.5, -0.05, O, -2.5,  0.05, O,
                        2.5,  0.05, O,  2.5, -0.05, O, 0);
           acadcolour(5);
           drawface(4, -0.05, -2.5, O,  0.05, -2.5, O,
                        0.05,  2.0, O, -0.05, 2.0, O, 0);
#undef O

           /* Draw X axis legend */

#define O 0.05
           acadcolour(2);
           drawface(4, 2.2, 0.2, 0.0, 2.4, 0.6, 0.0,
                       2.4 + O, 0.6, 0.0, 2.2 + O, 0.2, 0.0, 0);
           drawface(4, 2.2, 0.6, 0.0, 2.4, 0.2, 0.0,
                       2.4 + O, 0.2, 0.0, 2.2 + O, 0.6, 0.0, 0);

           /* Draw Y axis legend */

           acadcolour(2);
           drawface(4, 0.4, 2.2, 0.0, 0.4, 2.4, 0.0,
                       0.4 + O,  2.4, 0.0, 0.4 + O, 2.2, 0.0, 0);
           drawface(4, 0.4, 2.4, 0.0, 0.3, 2.6, 0.0,
                       0.3 + O, 2.6, 0.0, 0.4 + O, 2.4, 0.0, 0);
           drawface(4, 0.4, 2.4, 0.0, 0.5, 2.6, 0.0,
                       0.5 + O, 2.6, 0.0, 0.4 + O, 2.4, 0.0, 0);
#undef O

           /* Draw the Z axis barber pole */

           for (x = 0.0; x < 3.5; x += 1.0) {
              i = floor(x);
              i &= 1;
              acadcolour(i ? 1 : 2);
              drawface(4, -0.1, 2.0, x, 0.1, 2.0, x,
                           0.1, 2.0, x + 1.0, -0.1, 2.0, x + 1.0, 0);
           }

        } else {

           /* Draw X and Y axes */

           for (x = -2.0; x < 2.5; x += 1.0) {
              y = fzero(x) ? 2.5 : 2.0;
              drawvec(x, y, 0.0, x, -y, 0.0);
              drawvec(y, x, 0.0, -y, x, 0.0);
           }

           /* Draw X axis legend */

           drawvec(2.2, 0.2, 0.0, 2.4, 0.6, 0.0);
           drawvec(2.2, 0.6, 0.0, 2.4, 0.2, 0.0);

           /* Draw Y axis legend */

           drawvec(0.4, 2.2, 0.0, 0.4, 2.4, 0.0);
           drawvec(0.4, 2.4, 0.0, 0.3, 2.6, 0.0);
           drawvec(0.4, 2.4, 0.0, 0.5, 2.6, 0.0);

           /* Draw Z axis pole and ticks */

           drawvec(0.0, 2.0, 0.0, 0.0, 2.0, 4.0);
           for (x = 1.0; x < 4.5; x += 1.0) {
              drawvec(0.0, 1.9, x, 0.0, 2.1, x);
           }
        }
}

/*  CUBE  --  Cube centred around origin with edge length of 2  */

void cube()
{
        drawface(4, -1.0, -1.0, -1.0, -1.0,  1.0, -1.0,
                     1.0,  1.0, -1.0,  1.0, -1.0, -1.0, 0);
        drawface(4, -1.0, -1.0,  1.0, -1.0,  1.0,  1.0,
                     1.0,  1.0,  1.0,  1.0, -1.0,  1.0, 0);
        drawface(4, -1.0, -1.0, -1.0, -1.0, -1.0,  1.0,
                     1.0, -1.0,  1.0,  1.0, -1.0, -1.0, 0);
        drawface(4, -1.0,  1.0, -1.0, -1.0,  1.0,  1.0,
                     1.0,  1.0,  1.0,  1.0,  1.0, -1.0, 0);
        drawface(4, -1.0, -1.0, -1.0, -1.0, -1.0,  1.0,
                    -1.0,  1.0,  1.0, -1.0,  1.0, -1.0, 0);
        drawface(4,  1.0, -1.0, -1.0,  1.0, -1.0,  1.0,
                     1.0,  1.0,  1.0,  1.0,  1.0, -1.0, 0);
}

/*  SPHERE  --  Unit radius sphere centred on the origin  */

void sphere()
{
        int i, j;
        double a, b, bx, by, lx, ly, tx, ty;

        if (wireframe) {
           a = (2 * PI) / cnsegs;
           lx = 1.0;
           ly = 0.0;
           for (i = 0; i < cnsegs; i++) {
              tx = cos(a);
              ty = sin(a);
              drawvec(lx, ly, 0.0, tx, ty, 0.0);
              drawvec(0.0, lx, ly, 0.0, tx, ty);
              drawvec(lx, 0.0, ly, tx, 0.0, ty);
              a += (2 * PI) / cnsegs;
              lx = tx;
              ly = ty;
           }
        } else {
           drawmesh(cnsegs, cnsegs, TRUE, FALSE);
           for (i = 0; i < cnsegs; i++) {
              a = i * ((2 * PI) / cnsegs);
              tx = cos(a);
              ty = sin(a);
              for (j = 0; j < cnsegs; j++) {
                 b = j * (PI / (cnsegs - 1));
                 bx = cos(b);
                 by = sin(b);
                 drawmpt(tx * by, ty * by, bx);
              }
           }
           drawmend();
        }
}

/*  Initialisation and termination routines  */

void startup()
{
        matident(ct);
        while (cts != NULL) {
            pop();
        }
        ctdepth = 0;
}

void shutdown()
{
        if (ctdepth > 0) {
           ads_printf("\nWarning: %d coordinate systems still pushed.\n",
              ctdepth);
        }
        if (pdb != NULL) {
           free(pdb);
           pdbl = 0;
        }
}
