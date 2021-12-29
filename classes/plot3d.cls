
\   3D function plotter

PUBLIC:
    80 characters function
    real a
    real b
    real c
    real x_min
    real x_max
    real y_min
    real y_max
    integer steps
    real plot_size

PRIVATE:

132 string s

2variable xstep
2variable ystep
variable func
variable eks
variable wye
2variable xmin
2variable ymin

: eval
    evaluate
    0<> if
        type cr
        abort" "Error in above expression"
    then
;

: tack
    dup addgroup setgroup
;

: scale
    y_max 2@ y_min 2@ f- f/ plot_size 2@ f*
;

: coord
    @ 2@ scale
;

method draw
{
    \ Purge any trash definition left by error in earlier draw call
    "(plot3d)" find if drop "forget (plot3d)" evaluate drop then drop

    ": (plot3d) ;" eval
    1 (redefine)
    a 2@ "2constant a" eval
    b 2@ "2constant b" eval
    c 2@ "2constant c" eval
    0 (redefine)
    x_min 2@ 2dup xmin 2! "2constant x" eval
        "x" find drop >body eks !
    y_min 2@ 2dup ymin 2! "2constant y" eval
        "y" find drop >body wye !

    ": zfun " s s! function s s+ " ; " s s+ s eval

    x_max 2@ xmin 2@ f- steps @ 1- float f/ xstep 2!
    y_max 2@ ymin 2@ f- steps @ 1- float f/ ystep 2!
    "zfun" find drop func !

    clearitem
        "POLYLINE" 0 tack
        1 66 tack
        16 70 tack
        steps @ 71 tack
        steps @ 72 tack
    ads_entmake drop

    ymin 2@ wye @ 2!
    steps @ 0 ?do
        xmin 2@ eks @ 2!
        steps @ 0 ?do
            clearitem
                "VERTEX" 0 tack
                eks coord wye coord func @ execute scale 10 tack
                64 70 tack
            ads_entmake drop
            eks @ 2@ xstep 2@ f+ eks @ 2!
        loop
        wye @ 2@ ystep 2@ f+ wye @ 2!
    loop
    clearitem
        "SEQEND" 0 tack
    ads_entmake drop
    "forget (plot3d)" evaluate drop
}

method acquire
{
    "x x f* 2.0 f* y y f* f+ sqrt cos" function s!
    -10.0 x_min 2!
    10.0 x_max 2!
    -10.0 y_min 2!
    10.0 y_max 2!
    25 steps !
    4.0 plot_size 2!
    this object.inspect
}
