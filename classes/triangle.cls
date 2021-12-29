

\   Equilateral triangle class

:lpoly public derived

method acquire
{
    3 nsides !
    "Edge size: " getreal 0= if 2drop false return then size 2!
    True
}

method draw
{
    draw <-
}

PUBLIC:

method grow
{
    grow <-
}

2variable two 2.0 two 2!
variable one 1 one !

command method growmore (( real "Growth factor" two default
                                ARG_nozero ARG_noneg or argmodes
                                "Smaller Bigger" keywords arg
                           integer "Additional sides" one default arg
                        ))
{
    nsides +!                         \ Increment sides
    if
        "Smaller" strcmp 0= if 0.5 else 2.0 then
    then
        size 2@ f* size 2!
}
