/*

        The Simple Graphics Library: Platonic Solids

        See Jim Blinn's Corner, IEEE Computer Graphics and Applications,
        November 1987, Page 62.

        Implemented by John Walker in March of 1983.  This program
        uses the subroutines in sglib.c.

*/

#include "class.h"

#include "sglib.h"

/*  TETRAHEDRON  --  Draw a tetrahedron  */

void tetrahedron()
{
        push();
           tran(0.0, 0.0, 0.57735);
           rot(torad(-35.2644), X);
           rot(torad(45.0), Y);

           pnt(1,  1.0,  1.0,  1.0);
           pnt(2,  1.0, -1.0, -1.0);
           pnt(3, -1.0,  1.0, -1.0);
           pnt(4, -1.0, -1.0,  1.0);

           poly(4, 3, 2, 0);
           poly(3, 4, 1, 0);
           poly(2, 1, 4, 0);
           poly(1, 2, 3, 0);
        pop();
}

/*  OCTAHEDRON  --  Draw an octahedron  */

void octahedron()
{
        pnt(1,  1.0,  0.0,  0.0);
        pnt(2, -1.0,  0.0,  0.0);
        pnt(3,  0.0,  1.0,  0.0);
        pnt(4,  0.0, -1.0,  0.0);
        pnt(5,  0.0,  0.0,  1.0);
        pnt(6,  0.0,  0.0, -1.0);

        push();
           tran(0.0, 0.0, 0.57735);
           rot(torad(-35.2644), X);
           rot(torad(45.0), Y);
           poly(1, 3, 5, 0);
           poly(3, 1, 6, 0);
           poly(4, 1, 5, 0);
           poly(1, 4, 6, 0);
           poly(3, 2, 5, 0);
           poly(2, 3, 6, 0);
           poly(2, 4, 5, 0);
           poly(4, 2, 6, 0);
        pop();
}

/*  DODECAHEDRON  --  Draw a dodecahedron  */

void dodecahedron()
{
        push();

        tran(0.0, 0.0, 1.37638);
        rot(torad(-58.2825), X);

        pnt(1,  1.0,  1.0,  1.0);
        pnt(2,  1.0,  1.0, -1.0);
        pnt(3,  1.0, -1.0,  1.0);
        pnt(4,  1.0, -1.0, -1.0);
        pnt(5, -1.0,  1.0,  1.0);
        pnt(6, -1.0,  1.0, -1.0);
        pnt(7, -1.0, -1.0,  1.0);
        pnt(8, -1.0, -1.0, -1.0);

#define GR 1.618034                /* Golden ratio */
#define G0 0.618034

        pnt(11,  G0,  GR, 0.0);
        pnt(12, -G0,  GR, 0.0);
        pnt(13,  G0, -GR, 0.0);
        pnt(14, -G0, -GR, 0.0);

        pnt(21,  GR, 0.0,  G0);
        pnt(22,  GR, 0.0, -G0);
        pnt(23, -GR, 0.0,  G0);
        pnt(24, -GR, 0.0, -G0);

        pnt(31, 0.0,  G0,  GR);
        pnt(32, 0.0, -G0,  GR);
        pnt(33, 0.0,  G0, -GR);
        pnt(34, 0.0, -G0, -GR);

#undef GR
#undef G0

        poly(2, 11, 1, 21, 22, 0);
        poly(5, 12, 6, 24, 23, 0);
        poly(3, 13, 4, 22, 21, 0);
        poly(8, 14, 7, 23, 24, 0);

        poly(3, 21, 1, 31, 32, 0);
        poly(2, 22, 4, 34, 33, 0);
        poly(5, 23, 7, 32, 31, 0);
        poly(8, 24, 6, 33, 34, 0);

        poly(5, 31, 1, 11, 12, 0);
        poly(3, 32, 7, 14, 13, 0);
        poly(2, 33, 6, 12, 11, 0);
        poly(8, 34, 4, 13, 14, 0);

        pop();
}

/*  ICOSAHEDRON  --  Draw an icosahedron  */

void icosahedron()
{
#define GR 1.618034                /* Golden ratio */

        pnt(11,  GR,  1.0, 0.0);
        pnt(12, -GR,  1.0, 0.0);
        pnt(13,  GR, -1.0, 0.0);
        pnt(14, -GR, -1.0, 0.0);

        pnt(21,  1.0, 0.0,  GR);
        pnt(22,  1.0, 0.0, -GR);
        pnt(23, -1.0, 0.0,  GR);
        pnt(24, -1.0, 0.0, -GR);

        pnt(31, 0.0,  GR,  1.0);
        pnt(32, 0.0, -GR,  1.0);
        pnt(33, 0.0,  GR, -1.0);
        pnt(34, 0.0, -GR, -1.0);

#undef GR

        push();
           tran(0.0, 0.0, 1.51152);
           rot(torad(-20.9051), X);

           poly(11, 31, 21, 0);
           poly(11, 22, 33, 0);
           poly(13, 21, 32, 0);
           poly(13, 34, 22, 0);
           poly(12, 23, 31, 0);
           poly(12, 33, 24, 0);
           poly(14, 32, 23, 0);
           poly(14, 24, 34, 0);

           poly(11, 33, 31, 0);
           poly(12, 31, 33, 0);
           poly(13, 32, 34, 0);
           poly(14, 34, 32, 0);

           poly(21, 13, 11, 0);
           poly(22, 11, 13, 0);
           poly(23, 12, 14, 0);
           poly(24, 14, 12, 0);

           poly(31, 23, 21, 0);
           poly(32, 21, 23, 0);
           poly(33, 22, 24, 0);
           poly(34, 24, 22, 0);
        pop();
}
