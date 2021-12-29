
\   Labeled polygon class definition

    PUBLIC:
        STATIC INTEGER polycount      \ Total polygons in drawing

        REAL size                     \ Radius of circumscribed circle
        INTEGER nsides                \ Number of sides

    PRIVATE:
        INTEGER polyseq               \ Unique sequence number

    TEMPORARY:
        REAL anginc                   \ Angle increment
        point kp 1.0 2.0 0.0 kp point!

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
        xcor ycor zcor 10 tack
        xcor ycor zcor 11 tack
        size 2@ 4.0 f/ 40 tack
        nsides @ "%ld" ns strform ns 1 tack
        4 72 tack
        ads_entmake drop
    360.0 nsides @ float f/ 2dup anginc 2!
    penup
        nsides @ 1 and 0= if
            2dup 2.0 f/ 90.0 f+ 2dup right size 2@ forward left
        else
            90.0 2dup right size 2@ forward anginc 2@ 2.0 f/ f+ left
        then
    pendown
    deg_rad cos 1.0 f- fnegate size 2@ 2dup f* 2.0 f* f* sqrt
    nsides @ 0 do
        2dup forward anginc 2@ left
    loop
    2drop
}

PRIVATE:

method newclass
{
    1000 polycount !
}

PROTECTED:

variable dnsides 3 dnsides !
2variable dsize 1.0 dsize 2!

method acquire
{
    integer "Number of sides" dnsides default arg
        0<> if false return then nsides !
    distance "Edge size" dsize default arg
        0<> if false return then size 2!
    1 polycount +! polycount @ polyseq !
    ( Save last acquisition parameters as defaults for the next )
    nsides @ dnsides !
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

: as
    ." "Arg status: " . ." "  "
;

command method iarg
{
    characters "String" bilge default arg as type cr
    point "Location" kp default arg as point? cr
    real "Real number" two default arg as f. cr
    distance "Distance" two default arg as f. cr
    integer "Integer number" one default arg as . cr
    angle "Angle" kp basepoint arg as pi f/ 180.0 f* f. cr
    orientation "Orientation" kp basepoint arg as pi f/ 180.0 f* f. cr
    corner "Corner" kp basepoint arg as point? cr
    keyword "Huey, Dewey, or Louie" "Huey Dewey Louie" keywords
        huey default arg as type cr
}

80 string st

command method ptr
{
    pointer "Choose target" arg as dup type " " type
        fetch 's-class st classname st type cr
}
