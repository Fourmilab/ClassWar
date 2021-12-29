/*

        Fractal mountain landscape generator

        Designed and implemented in November of 1989 by John Walker

        This  program generates fractal mountain landscapes by Fourier
        filtering of random data.  The landscapes are  represented  as
        rat  nest  meshes  within  an  AutoCAD  drawing.   This module
        implements the generation  and  editing  of  mountain  objects
        defined in the accompanying file mountain.cls, which is loaded
        as an AutoCAD user-defined entity class definition.  To enable
        mountain  editing,  you  must  load  this application with the
        command:

            (xload "mtnapp")

        The  fractal dimension controls the roughness of the generated
        landscape by applying a low-pass spatial filter to the  random
        frequency  domain  data  before performing the inverse Fourier
        transform.  The higher the fractal dimension the  rougher  the
        terrain;  values  between  2  and  3  are typical.  Dimensions
        between 1.7 and  2  generate  Appalachian-type  hills;  values
        between  2 and 2.5 yield Sierra Nevada like peaks; and numbers
        close to 3 result in fantasy landscapes of spires (which  look
        good only at high mesh densities).

        The output of the mountain generation algorithm is a table  of
        heights.   Negative  heights  are  set to zero and coloured as
        water.  You can scale elevations above sea  level  by  raising
        the  raw  values to a power specified by the power law scaling
        exponent.  The default value of 1 simply uses  the  elevations
        as generated.  Powers of less than 1 mimic eroded terrain (try
        0.33--cube root scaling often produces  landscapes  that  look
        glacially sculpted like Yosemite).  Powers greater than 1 make
        increasingly forbidding peaks.  Again, if you use large  power
        law  exponents,  the  results  will  look  best at higher mesh
        densities.

        References:

            Peitgen, H.-O., and Saupe, D. eds., The Science Of Fractal
                Images, New York: Springer Verlag, 1988.

            Press, W. H., Flannery, B. P., Teukolsky, S. A., Vetterling,
                W. T., Numerical Recipes In C, New Rochelle; Cambridge
                University Press, 1988.

*/

#include <stdio.h>
#include <math.h>
#include <strings.h>
#ifdef unix
#include <memory.h>
#endif

#include "adslib.h"
#include "mountain.h"                 /* From CLASSTOC of mountain.cls */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define max(a,b) ((a) >  (b) ? (a) : (b))
#define min(a,b) ((a) <= (b) ? (a) : (b))

/* Definitions used to address real and imaginary parts in a two-dimensional
   array of complex numbers as stored by fourn(). */

#define Real(v, x, y)  v[1 + (((x) * n) + (y)) * 2]
#define Imag(v, x, y)  v[2 + (((x) * n) + (y)) * 2]

/* Set point variable from three co-ordinates */

#define Spoint(pt, x, y, z)  pt[X] = (x);  pt[Y] = (y);  pt[Z] = (z)

/* Copy point from another */

#define Cpoint(d, s)   d[X] = s[X];  d[Y] = s[Y];  d[Z] = s[Z]

/* Definitions for building result buffer chains. */

#define tacky()     struct resbuf *rb, *rbtail, *ri
#define tackrb(t)   ri=ads_newrb(t);  rbtail->rbnext=ri;  rbtail=ri
#define defent(e)   rb=ri=rbtail=ads_newrb(0); ri->resval.rstring = strsave(e)
#define makent() { int stat=ads_entmake(rb); if (stat!=RTNORM) { \
                   ads_printf("Error creating %s entity\n",       \
                   rb->resval.rstring); } ads_relrb(rb); }
#define tackint(code, val) tackrb(code); ri->resval.rint = (val)
#define tackreal(code, val) tackrb(code); ri->resval.rreal = (val)
#define tackpoint(code, x, y, z) tackrb(code); Spoint(ri->resval.rpoint, (x), (y), (z))
#define tackvec(code, vec) tackrb(code); Cpoint(ri->resval.rpoint, vec)

#define V  (void)

/* Utility definition to get an  array's  element  count  (at  compile
   time).   For  example:

       int  arr[] = {1,2,3,4,5};
       ...
       printf("%d", ELEMENTS(arr));

   would print a five.  ELEMENTS("abc") can also be used to  tell  how
   many  bytes are in a string constant INCLUDING THE TRAILING NULL. */

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

/*  Display parameters  */

#define COL_BASE    (wtype == 0 ? 4 : 250) /* Baseplate colour */
#define BASE_THICK  0.1            /* Baseplate thickness */

/*  Globals imported  */

#ifdef HIGHC
/* Like  so  many  other things in High C,  time() is not only utterly
   incompatible (it returns a double, not a long), but  doesn't  work.
   So,  we redefine time to call clock(), the low-level DOS clock tick
   call, which does seem to be working this week and returns  a  long. */
#define time(x) clock()
#endif

extern long time();

/*  Local variables  */

#define nrand 4                    /* Gauss() sample count */
static double arand, gaussadd, gaussfac; /* Gaussian random parameters */
static int rseed;                  /* Current random seed */
static ads_real fracdim = 2.15;    /* Fractal dimension */
static ads_real powscale = 1.0;    /* Elevation power factor */
static int wtype = 0;              /* World type */

/*      FOURN  --  Multi-dimensional fast Fourier transform

        Called with arguments:

           data       A  one-dimensional  array  of  floats  (NOTE!!!   NOT
                      DOUBLES!!), indexed from one (NOTE!!!   NOT  ZERO!!),
                      containing  pairs of numbers representing the complex
                      valued samples.  The Fourier transformed results  are
                      returned in the same array.

           nn         An  array specifying the edge size in each dimension.
                      THIS ARRAY IS INDEXED FROM  ONE,  AND  ALL  THE  EDGE
                      SIZES MUST BE POWERS OF TWO!!!

           ndim       Number of dimensions of FFT to perform.  Set to 2 for
                      two dimensional FFT.

           isign      If 1, a Fourier transform is done, if -1 the  inverse
                      transformation is performed.

        This  function  is essentially as given in Press et al., "Numerical
        Recipes In C", Section 12.11, pp.  467-470.
*/

static void fourn(data, nn, ndim, isign)
  float data[];
  int nn[], ndim, isign;
{
        register int i1, i2, i3;
        int i2rev, i3rev, ip1, ip2, ip3, ifp1, ifp2;
        int ibit, idim, k1, k2, n, nprev, nrem, ntot;
        float tempi, tempr;
        double theta, wi, wpi, wpr, wr, wtemp;

#define SWAP(a,b) tempr=(a); (a) = (b); (b) = tempr

        ntot = 1;
        for (idim = 1; idim <= ndim; idim++)
           ntot *= nn[idim];
        nprev = 1;
        for (idim = ndim; idim >= 1; idim--) {
           n = nn[idim];
           nrem = ntot / (n * nprev);
           ip1 = nprev << 1;
           ip2 = ip1 * n;
           ip3 = ip2 * nrem;
           i2rev = 1;
           for (i2 = 1; i2 <= ip2; i2 += ip1) {
              if (i2 < i2rev) {
                 for (i1 = i2; i1 <= i2 + ip1 - 2; i1 += 2) {
                    for (i3 = i1; i3 <= ip3; i3 += ip2) {
                       i3rev = i2rev + i3 - i2;
                       SWAP(data[i3], data[i3rev]);
                       SWAP(data[i3 + 1], data[i3rev + 1]);
                    }
                 }
              }
              ibit = ip2 >> 1;
              while (ibit >= ip1 && i2rev > ibit) {
                 i2rev -= ibit;
                 ibit >>= 1;
              }
              i2rev += ibit;
           }
           ifp1 = ip1;
           while (ifp1 < ip2) {
              ifp2 = ifp1 << 1;
              theta = isign * 6.28318530717959 / (ifp2 / ip1);
              wtemp = sin(0.5 * theta);
              wpr = -2.0 * wtemp * wtemp;
              wpi = sin(theta);
              wr = 1.0;
              wi = 0.0;
              for (i3 = 1; i3 <= ifp1; i3 += ip1) {
                 for (i1 = i3; i1 <= i3 + ip1 - 2; i1 += 2) {
                    for (i2 = i1; i2 <= ip3; i2 += ifp2) {
                       k1 = i2;
                       k2 = k1 + ifp1;
                       tempr = wr * data[k2] - wi * data[k2 + 1];
                       tempi = wr * data[k2 + 1] + wi * data[k2];
                       data[k2] = data[k1] - tempr;
                       data[k2 + 1] = data[k1 + 1] - tempi;
                       data[k1] += tempr;
                       data[k1 + 1] += tempi;
                    }
                 }
                 wr = (wtemp = wr) * wpr - wi * wpi + wr;
                 wi = wi * wpr + wtemp * wpi + wi;
              }
              ifp1 = ifp2;
           }
           nprev *= n;
        }
}
#undef SWAP

/*  INITGAUSS  --  Initialise random number generators.  */

static void initgauss(seed)
  int seed;
{
        /* Since rand() is defined to  return numbers from 0 to 32767,
           we set arand to this constant range, which does not  depend
           on the compiler's idea of the size of "int"s.  */
        arand = 32767.0;
        gaussadd = sqrt(3.0 * nrand);
        gaussfac = 2 * gaussadd / (nrand * arand);
        V srand(seed);
}

/*  GAUSS  --  Return a Gaussian random number.  */

static double gauss()
{
        int i;
        double sum = 0.0;

        for (i = 1; i <= nrand; i++) {
           sum += rand();
        }
        return gaussfac * sum - gaussadd;
}

/*  SPECTRALSYNTH  --  Spectrally synthesised fractal motion in two
                       dimensions.  */

static void spectralsynth(x, n, h)
  float **x;
  int n;
  double h;
{
        int i, j, i0, j0, nsize[3];
        double rad, phase;
        float *a;

        a = (float *) malloc((unsigned) (i =
                             (((n * n) + 1) * 2 * sizeof(float))));
        if (a == NULL) {
           ads_printf("Cannot allocate %d x %d result array (%d bytes).\n",
              n, n, i);
           ads_abort("Out of memory");
        }
        *x = a;
        V memset((char *) a, 0, i); /* Clear array to zeroes */

        for (i = 0; i <= n / 2; i++) {
           for (j = 0; j <= n / 2; j++) {
              phase = 2 * M_PI * (rand() / arand);
              if (i != 0 || j != 0) {
                 rad = pow((double) (i * i + j * j), -(h + 1) / 2) * gauss();
              } else {
                 rad = 0;
              }
              Real(a, i, j) = rad * cos(phase);
              Imag(a, i, j) = rad * sin(phase);
              i0 = (i == 0) ? 0 : n - i;
              j0 = (j == 0) ? 0 : n - j;
              Real(a, i0, j0) = rad * cos(phase);
              Imag(a, i0, j0) = - rad * sin(phase);
           }
        }
        Imag(a, n / 2, 0) = Imag(a, 0, n / 2) = Imag(a, n / 2, n / 2) = 0;
        for (i = 1; i <= n / 2 - 1; i++) {
           for (j = 1; j <= n / 2 - 1; j++) {
              phase = 2 * M_PI * (rand() / arand);
              rad = pow((double) (i * i + j * j), -(h + 1) / 2) * gauss();
              Real(a, i, n - j) = rad * cos(phase);
              Imag(a, i, n - j) = rad * sin(phase);
              Real(a, n - i, j) = rad * cos(phase);
              Imag(a, n - i, j) = - rad * sin(phase);
           }
        }

        nsize[0] = 0;
        nsize[1] = nsize[2] = n;   /* Dimension of frequency domain array */
        fourn(a, nsize, 2, -1);    /* Take inverse 2D Fourier transform */
}

/*  INITSEED  --  Generate initial random seed, if needed.  */

static void initseed()
{
    int i;

    i = (time((long *) NULL) ^ 0x5B3CF37C) & ((int) ~0);
    V srand(i);
    for (i = 0; i < 7; i++)
       V rand();
    rseed = rand();
}

/*  CALCCOL  --  Calculate colour of mesh item from its relative
                 elevation.  */

static int calccol(a, n, x, y, rmax, rmin)
  float *a;
  int n, x, y;
  ads_real rmax, rmin;
{
        ads_real h1 = Real(a, x, y),
                 h2 = Real(a, x + 1, y),
                 h3 = Real(a, x, y + 1),
                 h4 = Real(a, x + 1, y + 1),
                 h;
        int rcol;

        static struct {
           ads_real cthresh;
           int tcolour;
        } ctab[] = {
           {0.25, 80},
           {0.6, 90},
           {0.75, 50},
           {0.80, 40},
           {0.90, 131},
           {1.00, 255}
        },
          etab[] = {
           {0.50, 3},
           {0.60, 2},
           {0.80, 1},
           {0.90, 6},
           {1.00, 7}
        },
          clouds[] = {
           {0.1666, 250},
           {0.3333, 251},
           {0.5000, 252},
           {0.6666, 253},
           {0.8333, 254},
           {1.0000, 255}
        };

        h = max(h1, max(h2, max(h3, h4)));
        if (h <= 0) {
           int ih;

           if (wtype == 0 || wtype == 2) {
              rcol = 5;            /* Flat blue */
           } else {
              h = (h1 + h2 + h3 + h4) / 4;
              if (rmin == 0)
                 rmin = 1;
              h = h / rmin;
              ih = (h * 3) + 0.5;
              rcol = 140 + min(3, ih) * 10;
           }
        } else {
           int i;

           if (rmax == 0)
              rmax = 1;
           h = h / rmax;
           if (wtype == 0) {
              for (i = 0; i < ELEMENTS(etab); i++) {
                 if (h <= etab[i].cthresh)
                    break;
              }
              rcol = etab[i].tcolour;
           } else if (wtype == 2) {
              for (i = 0; i < ELEMENTS(clouds); i++) {
                 if (h <= clouds[i].cthresh)
                    break;
              }
              rcol = clouds[i].tcolour;
           } else {
              for (i = 0; i < ELEMENTS(ctab); i++) {
                 if (h <= ctab[i].cthresh)
                    break;
              }
              rcol = ctab[i].tcolour;
           }
        }
        return rcol;
}

/*  STRSAVE  --  Allocate a duplicate of a string.  */

static char *strsave(s)
  char *s;
{
        char *c = malloc((unsigned) (strlen(s) + 1));

        if (c == NULL)
           ads_abort("Out of memory");
        V strcpy(c, s);
        return c;
}

/*  MAKEVTX  --  Append vertex entity to the database. */

static void makevtx(p)
  ads_point p;
{
        tacky();

        defent("VERTEX");
        tackvec(10, p);               /* Vertex point */
        tackint(70, 128 + 64);        /* Flags = Rat nest vertex point */
        makent();
}

/*  MAKEFACE  --  Append vertex entity representing a face in the mesh. */

static void makeface(colour, p1, p2, p3, p4)
  int colour, p1, p2, p3, p4;
{
        tacky();

        defent("VERTEX");
        tackpoint(10, 0, 0, 0);       /* Vertex point */
        tackint(62, colour);          /* Face colour */
        tackint(70, 128);             /* Vertex flags = Rat nest mesh face */
        tackint(71, p1);              /* Vertex 1 */
        tackint(72, p2);              /* Vertex 2 */
        tackint(73, p3);              /* Vertex 3 */
        tackint(74, p4);              /* Vertex 4 */
        makent();
}

/*  MAKEMESH  --  Create the mesh using the ads_entmake mechanism.  */

static void makemesh(a, n, rmax, rmin)
  float *a;
  int n;
  ads_real rmax, rmin;
{
        int i, j;
        tacky();

        defent("POLYLINE");
        tackint(66, 1);               /* Complex entity flag */
        tackint(70, 64);              /* Polyline type flags: rat nest mesh */
        tackint(71, n * n + 4 * n);   /* Vertex count = Mesh plus
                                                        baseplate edges */
/*      Total faces = n^2 +                   mesh tiles
                            1 +               baseplate
                                4 * (n - 1)   wall tiles. */
        tackint(72, n * n + 1 + 4 * (n - 1));
        makent();

        /* Send the vertex table. */

        for (i = 0; i < n; i++) {
           for (j = 0; j < n; j++) {
              ads_point p;

              Spoint(p, i, j,
                 BASE_THICK + (n / (rmax * 2)) * max(0, Real(a, i, j)));

              makevtx(p);
           }
        }

        /* Send the baseplate vertices.  We compute and store the
           four edges of the baseplate, addressed by edge number from
           0 to 3 and vertex number along the edge. */

        for (i = 0; i < n; i++) {
           ads_point p;

           Spoint(p, i, 0, 0);
           makevtx(p);
           Spoint(p, i, n - 1, 0);
           makevtx(p);
           Spoint(p, 0, i, 0);
           makevtx(p);
           Spoint(p, n - 1, i, 0);
           makevtx(p);
        }

        /* Send the mesh tiles. */

#define Vtx(x, y) ((((x) * n) + (y)) + 1)     /* Address mesh vertex */
#define Bpx(e, x) (((n * n) + 1) + (((x) * 4) + (e)))
        for (i = 0; i < (n - 1); i++) {
           for (j = 0; j < (n - 1); j++) {
              makeface(calccol(a, n, i, j, rmax, rmin),
                             Vtx(i, j),
                             Vtx(i, j + 1) * (j == n - 2 ? 1 : -1),
                             Vtx(i + 1, j + 1) * (i == n - 2 ? 1 : -1),
                             Vtx(i + 1, j));
           }
        }


        /* Draw the baseplate */

        makeface(COL_BASE,  -Bpx(0, 0), -Bpx(1, 0),
                            -Bpx(1, n - 1), -Bpx(0, n - 1));

        /* Build the "walls" that connect the baseplate to the edges
           of the model. */

        for (i = 0; i < n - 1; i++) {
           makeface(COL_BASE,
                        -Bpx(0, i),
                        -Vtx(i, 0),
                        -Vtx(i + 1, 0),
                        -Bpx(0, i + 1));

           makeface(COL_BASE,
                        -Bpx(1, i),
                        -Vtx(i, n - 1),
                        -Vtx(i + 1, n - 1),
                        -Bpx(1, i + 1));

           makeface(COL_BASE,
                        -Bpx(2, i),
                        -Vtx(0, i),
                        -Vtx(0, i + 1),
                        -Bpx(2, i + 1));

           makeface(COL_BASE,
                        -Bpx(3, i),
                        -Vtx(n - 1, i),
                        -Vtx(n - 1, i + 1),
                        -Bpx(3, i + 1));
        }
#undef Vtx
#undef Bpx

        /* Finally, tack on the SEQEND entity that triggers generation
           of the whole shebang. */

        defent("SEQEND");
        makent();
}

#undef defent
#undef tackrb

                    mountain_methods

/*  MOUNTAIN  --  Make a mountain.  */

draw_mountain
        float *a;
        int i, j, n, urs;
        ads_real rmin = 1e50, rmax = -1e50;

        if (mountain.interface_level > 1) {
            ads_printf("Interface level %d.\n", mountain.interface_level);
        }
        n = mountain.mesh_size;
        fracdim = mountain.fractal_dimension;
        powscale = mountain.power_factor;
        wtype = mountain.colour_mode;
        urs = mountain.random_seed;

        if (urs == 0) {
            initseed();
            urs = rseed;
        }
        mountain.random_seed = urs;   /* Return random seed used */

        initgauss(urs);

        spectralsynth(&a, n, 3.0 - fracdim);

        /* Apply power law scaling if non-unity scale is requested. */

        if (powscale != 1.0) {
           for (i = 0; i < n; i++) {
              for (j = 0; j < n; j++) {
                 if (Real(a, i, j) > 0) {
                    Real(a, i, j) = pow((double) Real(a, i, j), powscale);
                 }
              }
           }
        }

        /* Compute extrema for autoscaling. */

        for (i = 0; i < n; i++) {
           for (j = 0; j < n; j++) {
              rmin = min(rmin, Real(a, i, j));
              rmax = max(rmax, Real(a, i, j));
           }
        }

        makemesh(a, n, rmax, rmin);

        free((char *) a);
end_draw_mountain

/*  ROUGHER  --  Make a mountain rougher by increasing its
                 fractal dimension.  */

rougher_mountain
    if (rougher.arg1.kwflag) {
        ads_printf("\nKeyword: %s\n", rougher.arg1.kw.kwtext);
    } else {
        mountain.fractal_dimension *= rougher.arg1.kw.value;
    }
end_rougher_mountain
