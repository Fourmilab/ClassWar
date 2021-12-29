
\   Expression evaluator class definition

    PUBLIC:
        80 characters expression      \ Expression to evaluate
        real size


: tack
    dup addgroup setgroup
;

132 string ns
80 string result

PRIVATE:

method draw
{
    expression evaluate expression ns s!
    ?dup 0= if
        " = %g" result fstrform
    else
        " = (Error %ld)" result strform
    then
    result ns strcat
    clearitem
        "TEXT" 0 tack
        0.0 0.0 0.0 10 tack
        size 2@ 40 tack
        ns 1 tack
        0 72 tack
        ads_entmake drop
}

PROTECTED:

method acquire
{
    "1.0" expression s!
    0.2 size 2!
    True
}

PUBLIC:


