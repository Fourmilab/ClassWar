
\   Hilbert curve class definition

\   See Abelson & diSessa, "Turtle Geometry" (MIT Press, 1986), pp. 96-98

PUBLIC:
    REAL size
    INTEGER levels

PRIVATE:

: hilbert.gen                         ( size level parity --  )
    over 0<> if
        dup float 90.0 f* left
        2over 2over negate swap 1- swap hilbert.gen
        2over forward
        dup float 90.0 f* right
        2over 2over swap 1- swap hilbert.gen
        2over forward
        2over 2over swap 1- swap hilbert.gen
        dup float 90.0 f* right
        2over forward
        2over 2over negate swap 1- swap hilbert.gen
        dup float 90.0 f* left
    then
    2drop
    2drop
;

method draw
{
    size 2@
    2.0 levels @ float pow 1.0 f- f/
    levels @ -1 hilbert.gen
}

variable dlevels 2 dlevels !
2variable dsize 2.0 dsize 2!

method acquire
{
    distance "Size" dsize default arg
        0<> if false return then size 2!
    integer "Order of curve" dlevels default arg
        0<> if false return then levels !
    ( Save last acquisition parameters as defaults for the next )
    levels @ dlevels !
    size 2@ dsize 2!
    true
}

PUBLIC:

2variable two 2.0 two 2!

command method grow (( real "Growth factor" two default
                        ARG_nozero ARG_noneg or argmodes
                        "Smaller Bigger" keywords arg ))
{
    if
        "Smaller" strcmp 0= if 0.5 else 2.0 then
    then
        size 2@ f* size 2!
}

command method deeper
{
    1 levels +!
}

command method shallower
{
    levels @ 1 > if
        -1 levels +!
    then
}

2variable two 2.0 two 2!
variable one 1 one !

command method growmore (( real "Growth factor" two default
                                ARG_nozero ARG_noneg or argmodes
                                "Smaller Bigger" keywords arg
                           integer "Additional sides" one default arg
                        ))
{
    levels +!
    if
        "Smaller" strcmp 0= if 0.5 else 2.0 then
    then
        size 2@ f* size 2!
}
