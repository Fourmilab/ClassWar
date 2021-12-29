
\   Sierpinski gasket class definition

\   See Abelson & diSessa, "Turtle Geometry" (MIT Press, 1986), pp. 87-88

PUBLIC:
    REAL size
    INTEGER levels

: gasket                              ( fsize levels -- )
    ?dup if
    dup 1 = if pendown else penup then  \ Avoid duplicate vectors
        3 0 do
            dup 2over 2.0 f/ rot 1- gasket
            forward
            120.0 right
        loop
        drop
    then
;

PRIVATE:

method draw
{
    size 2@ levels @ gasket 2drop
}

2variable dsize 2.0 dsize 2!
variable dlevels 3 dlevels !

method acquire
{
    distance "Size" dsize default arg
        0<> if false return then size 2!
    integer "Levels" dlevels default arg
        0<> if false return then levels !
    True
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
