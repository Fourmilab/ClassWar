/*

        Definitions for the simple graphics library

*/

#define PI 3.14159265358979323846

#ifndef abs
#define abs(x) (((x) < 0) ? (-(x)) : (x))
#endif
#define fzero(x) (abs((x)) < 0.01)
#define torad(x) ((x) * (PI / 180.0))

#define X  0                       /* Coordinate indices */
#define Y  1
#define Z  2
#define T  3

#define TRUE  1
#define FALSE 0

#ifdef ADS
#define printf  ads_printf
#endif

typedef double point[3];           /* Three dimensional point */
typedef double vector[4];          /* Homogeneous coordinate vector */
typedef double matrix[4][4];       /* Transformation matrix */

extern double vecdot();            /* Dot product */
extern double vecmag();            /* Magnitude of a vector */

#define sgalloc alloc              /* Error-checking memory allocator */
