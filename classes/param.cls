
\   Parametric curve evaluator

PUBLIC:
    80 characters Curve_Name
    80 characters x_expression
    80 characters y_expression
    80 characters z_expression
    real a
    real b
    real c
    80 characters t_min
    80 characters t_max
    integer steps

PRIVATE:

132 string s

2variable tstep
variable px
variable py
variable pz
variable tee
2variable tmin

: eval
    evaluate
    0<> if
        type cr
        abort" "Error in above expression"
    then
;

method draw
{
    \ Purge any trash definition left by error in earlier draw call
    "(param)" find if drop "forget (param)" evaluate drop then drop

    ": (param) ;" eval
    1 (redefine)
    a 2@ "2constant a" eval
    b 2@ "2constant b" eval
    c 2@ "2constant c" eval
    0 (redefine)
    t_min eval 2dup tmin 2! "2constant t" eval
    "t" find drop >body tee !

    ": parx " s s! x_expression s s+ " ; " s s+ s eval
    ": pary " s s! y_expression s s+ " ; " s s+ s eval
    ": parz " s s! z_expression s s+ " ; " s s+ s eval

    t_max eval tmin 2@ f- steps @ float f/ tstep 2!
    "parx" find drop px !
    "pary" find drop py !
    "parz" find drop pz !

    penup
    steps @ 1+ 0 ?do
        px @ execute
        py @ execute
        pz @ execute
        setposition
        tee @ 2@ tstep 2@ f+ tee @ 2!
        pendown
    loop
    "forget (param)" evaluate drop
}

method acquire
{
    "Ellipse" curve_name s!
    "t cos a f*" x_expression s!
    "t sin b f*" y_expression s!
    1.0 a 2!
    0.75 b 2!
    "0.0" z_expression s!
    "0.0" t_min s!
    "2.0 Pi f*" t_max s!
    30 steps !
    this object.inspect
}
