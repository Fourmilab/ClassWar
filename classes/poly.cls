
\   Polygon class definition

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

PUBLIC:

method draw
{
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
    2.0 3.0 f/ size 2@ f* size 2!
}

command method more
{
    1 nsides +!
}

command method less
{
    nsides @ 3 > if
        -1 nsides +!
    then
}

\ Constructor

method new
{
    8 nsides !
    0.5 size 2!
    10 polyseq !
}
