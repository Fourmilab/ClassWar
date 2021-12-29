
\   Regular polyhedron class definition

PUBLIC:
    REAL size                         \ Scale factor
    INTEGER nfaces                    \ Number of faces

PUBLIC:

variable rptab
here 1 + rptab !
0 c, 4 c, 6 c, 8 c, 12 c, 20 c, 0 c, c=

variable bfit
variable bn

: ffit                                ( faces -- bestfitindex )
    10000 bfit !
    5 0 do
        rptab @ i + c@ over - abs dup bfit @
        <= if bfit ! i bn ! else drop then
    loop
    drop bn @
;

: ffix                                ( index -- facecount )
    rptab @ + c@
;

method draw
{
    xpush
    size 2@ 2dup 2dup xscal
    nfaces @ ffit ffix dup nfaces ! dup
        4 = if tetrahedron else
            dup 6 = if cube else
                dup 8 = if octahedron else
                    dup 12 = if dodecahedron else
                        dup 20 = if icosahedron else
 dup . cr
                            sphere
                        then
                    then
                then
            then
        then
        drop
    xpop
}

PROTECTED:

variable dnfaces 6 dnfaces !
2variable dsize 1.0 dsize 2!

method acquire
{
    integer "Number of faces" dnfaces default arg
        0<> if false return then ffit ffix nfaces !
    distance "Size" dsize default arg
        0<> if false return then size 2!
    ( Save last acquisition parameters as defaults for the next )
    nfaces @ dnfaces !
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

    ( MORE  --  Increase number of faces. )

command method more
{
    nfaces @ ffit dup 4 < if
        1+ ffix nfaces !
    else
        drop
    then
}

    ( LESS  --  Decrease number of faces. )

command method less
{
    nfaces @ ffit dup 0 > if
        1- ffix nfaces !
    else
        drop
    then
}

