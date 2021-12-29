
\   Dual polygon spirograph figures

\   See Abelson & diSessa, "Turtle Geometry" (MIT Press, 1986), p. 114

PUBLIC:
    REAL side1
    REAL angle1
    REAL side2
    REAL angle2
    INTEGER max-steps

: duopoly                             ( side1 angle1 side2 angle2 steps --  )
    0 do
        5 pick 4 pick i float f* setheading 7 pick 6 pick forward
        2dup i float f* setheading 2over forward
    loop
    4 0 do 2drop loop
;

PROTECTED:

method draw
{
    side1 2@ angle1 2@ side2 2@ angle2 2@ max-steps @ duopoly
}


pi 180.0 f/ 2constant kdtr

2variable dsize1 0.15 dsize1 2!
2variable dangle1 90.0 kdtr f* dangle1 2!
2variable dsize2 0.15 dsize2 2!
2variable dangle2 320.0 kdtr f* dangle2 2!

method acquire
{
    distance "Edge size 1" dsize1 default arg
        0<> if false return then side1 2!
    angle "Turning angle 1" dangle1 default arg
        0<> if false return then kdtr f/ angle1 2!
    distance "Edge size 2" dsize2 default arg
        0<> if false return then side2 2!
    angle "Turning angle 2" dangle2 default arg
        0<> if false return then kdtr f/ angle2 2!
    100 max-steps !
    true
}

PUBLIC:

command method grow
{
    1.5 side1 2@ f* side1 2!
    1.5 side2 2@ f* side2 2!
}

command method shrink
{
    2.0 3.0 f/ side1 2@ f* side1 2!
    2.0 3.0 f/ side2 2@ f* side2 2!
}
