
\   Labeled polygon class definition

:polygon PUBLIC DERIVED

TEMPORARY:
    REAL anginc                       \ Angle increment
    POINT kp 1.0 2.0 0.0 kp point!

: deg_rad
    Pi f* 180.0 f/
;

: tack
    dup addgroup setgroup
;

10 string ns

PUBLIC:

method draw
{
    clearitem
        "TEXT" 0 tack
    0.0 0.0 0.0 10 tack
    size 2@ 4.0 f/ 40 tack
        nsides @ "%ld" ns strform ns 1 tack
    4 72 tack
    ads_entmake drop
    this draw <-
}

command method shrink
{
size 2@ 0.1 f< if return then
    2.0 3.0 f/ size 2@ f* size 2!
}

    ( MORE  --  Increase number of sides. )

command method more
{
    1 nsides +!
}

    ( LESS  --  Decrease number of sides. )

command method less
{
    nsides @ 3 > if
    -1 nsides +!
    then
}

    ( SQUARE  --  Reset object to a unit square. )

method square
{
    4 nsides !
    1.0 size 2!
    this (entupdate)
}

\   NEW  -- Temporary constructor for debugging

method new
{
    8 nsides !
    0.5 size 2!
    10 polyseq !
}

2variable two 2.0 two 2!
variable one 1 one !

command method growmore (( real "Growth factor" two default
                ARG_nozero ARG_noneg or argmodes
                                "Smaller Bigger" keywords arg
                           integer "Additional sides" one default arg
            ))
{
\  ." "Sides: " dup .
    nsides +!                         \ Increment sides
\ ." " Kwstat: " dup .
    if
        "Smaller" strcmp 0= if 0.5 else 2.0 then
    then
    size 2@ f* size 2!
}

80 string bilge "anything" bilge s!
40 string huey "Huey" huey s!

public:

command method argtest ((
    characters "String" bilge default arg
    point "Position" kp default arg
    real "Real number" two default arg
    distance "Distance" two default arg
    integer "Integer number" one default arg
    angle "Angle" kp basepoint arg
    orientation "Orientation" kp basepoint arg
    corner "Corner" kp basepoint arg
    keyword "Huey, Dewey, or Louie" "Huey Dewey Louie" keywords
    huey default arg
               ))
{
    drop type cr
    point? cr
    f. cr
    f. cr
    . cr
    f. cr
    f. cr
    point? cr
    type cr
}
