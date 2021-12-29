
\   Autodesk logo class definition

PUBLIC:
    REAL size                         \ Scale factor

PUBLIC:

method draw
{
    xpush
        size 2@ 2dup 2dup xscal
        0.0 0.0 0.0 1 pnt
        0.0 0.0 2100.0 2 pnt
        925.0 0.0 2100.0 3 pnt
        1080.0 0.0 1850.0 4 pnt
        2160.0 0.0 0.0 5 pnt
        1895.0 0.0 0.0 6 pnt
        1240.0 0.0 1540.0 7 pnt
        930.0 0.0 1540.0 8 pnt
        275.0 0.0 0.0 9 pnt
        1080.0 0.0 1380.0 10 pnt
        1620.0 0.0 0.0 11 pnt
        1240.0 0.0 1065.0 12 pnt
        930.0 0.0 1065.0 13 pnt
        550.0 0.0 0.0 14 pnt
        1080.0 0.0 910.0 15 pnt
        2160.0 0.0 0.0 16 pnt
        2160.0 0.0 2100.0 17 pnt
        1260.0 0.0 2100.0 18 pnt
        0.0 1.0 0.0 21 pnt
        0.0 1.0 2100.0 22 pnt
        925.0 1.0 2100.0 23 pnt
        1080.0 1.0 1850.0 24 pnt
        2160.0 1.0 0.0 25 pnt
        1895.0 1.0 0.0 26 pnt
        1240.0 1.0 1540.0 27 pnt
        930.0 1.0 1540.0 28 pnt
        275.0 1.0 0.0 29 pnt
        1080.0 1.0 1380.0 30 pnt
        1620.0 1.0 0.0 31 pnt
        1240.0 1.0 1065.0 32 pnt
        930.0 1.0 1065.0 33 pnt
        550.0 1.0 0.0 34 pnt
        1080.0 1.0 910.0 35 pnt
        2160.0 1.0 0.0 36 pnt
        2160.0 1.0 2100.0 37 pnt
        1260.0 1.0 2100.0 38 pnt
        -0.5 -0.5 0.0 xtran
        0.000476 1.0 0.000463 xscal
        raton
        0 1 2 3 poly
        0 -1 8 9 poly
        0 1 -4 -8 poly
        0 -4 7 -8 poly
        0 -5 7 6 poly
        0 -7 4 -5 poly
        0 -9 13 14 poly
        0 9 -10 -13 poly
        0 -10 12 -13 poly
        0 -6 12 11 poly
        0 -12 10 -6 poly
        0 14 15 11 poly
        0 16 17 18 poly
        0 1 21 22 2 poly
        0 2 22 23 3 poly
        0 3 23 21 1 poly
        0 1 21 24 4 poly
        0 4 24 25 5 poly
        0 6 26 27 7 poly
        0 7 27 28 8 poly
        0 8 28 29 9 poly
        0 9 29 30 10 poly
        0 10 30 26 6 poly
        0 11 31 32 12 poly
        0 12 32 33 13 poly
        0 13 33 34 14 poly
        0 14 34 35 15 poly
        0 15 35 31 11 poly
        0 16 36 37 17 poly
        0 17 37 38 18 poly
        0 18 38 36 16 poly
        0 1 21 25 5 poly
        0.0 1.0 0.0 xtran
        0 1 2 3 poly
        0 -1 8 9 poly
        0 1 -4 -8 poly
        0 -4 7 -8 poly
        0 -5 7 6 poly
        0 -7 4 -5 poly
        0 -9 13 14 poly
        0 9 -10 -13 poly
        0 -10 12 -13 poly
        0 -6 12 11 poly
        0 -12 10 -6 poly
        0 14 15 11 poly
        0 16 17 18 poly
        ratoff
    xpop
}

PROTECTED:

2variable dsize 1.0 dsize 2!

method acquire
{
    distance "Size" dsize default arg
        0<> if false return then size 2!
    ( Save last acquisition parameters as defaults for the next )
    size 2@ dsize 2!
    true
}

PUBLIC:

command method grow
{
    1.5 size 2@ f* size 2!
}

command method shrink
{
size 2@ 0.1 f< if return then
    2.0 3.0 f/ size 2@ f* size 2!
}
