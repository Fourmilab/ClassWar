
\   Labeled polygon class definition

    :poly PUBLIC DERIVED

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

    this draw <- drop
}
