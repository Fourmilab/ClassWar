
\   Branching trees class definition

\   See Abelson & diSessa, "Turtle Geometry" (MIT Press, 1986), pp. 84-85

PUBLIC:
    INTEGER forklevels                \ Forking depth
    REAL size                         \ Size of branches
    REAL branchang                    \ Angle of branching

PRIVATE:

: rtree
    ?dup if
        >r
            \ Left branch
            2dup left
            2over 2.0 f* forward
            2over 2over r@ 1- rtree
            penup
            2over 2.0 f* back
            pendown

            \ Right branch
            2dup 2.0 f* right
            2over forward
            2over 2over r> 1- rtree
            penup
            2over back
            pendown
            2dup left
    then
    2drop
    2drop
;

method draw
{
    size 2@ branchang 2@ forklevels @ rtree
}

method acquire
{
(
    "Branch angle: " getreal 0= if 2drop false return then branchang 2!
    "Branch size: " getreal 0= if 2drop false return then size 2!
    "Branching depth: " getint 0= if drop false return then forklevels !
    True
)
    15.0 branchang 2! 0.2 size 2! 6 forklevels ! this object.inspect
}

PUBLIC:

command method taller
{
    size 2@ 1.1 f* size 2!
}

command method grow
{
    1 forklevels +!
}

: width
    branchang 2@ f* branchang 2!
;

command method widen
{
    1.25 width
}

command method narrow
{
    0.8 width
}
