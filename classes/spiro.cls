
\   Spirolateral class definition

\   See Abelson & diSessa, "Turtle Geometry" (MIT Press, 1986), pp. 36-37

PUBLIC:
    REAL side
    REAL tangle
    INTEGER cycle-length
    INTEGER max-steps

TEMPORARY:
   INTEGER count

PROTECTED:

method draw
{
    max-steps @ 0 do
        cycle-length @ 0 do
            side 2@ i 1+ float f* forward
            tangle 2@ right
        loop
    loop
}

pi 180.0 f/ 2constant kdtr

2variable dsize 0.2 dsize 2!
2variable dangle 144.0 kdtr f* dangle 2!
variable dclen 8 dclen !

method acquire
{
    distance "Edge size" dsize default arg
        0<> if false return then side 2!
    angle "Turning angle" dangle default arg
        0<> if false return then kdtr f/ tangle 2!
    integer "Cycle length" dclen default arg
        0<> if false return then cycle-length !
    20 max-steps !
    true
}

PUBLIC:

command method grow
{
    1.5 side 2@ f* side 2!
}

command method shrink
{
    2.0 3.0 f/ side 2@ f* side 2!
}
