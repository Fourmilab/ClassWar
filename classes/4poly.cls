

public:
    integer start_of_gorp
    real size
    integer nsides
    lpoly p1
    lpoly p2
    lpoly p3
    lpoly p4
    static lpoly stat

private:
    lpoly priv

public:
    lpoly pub2
    integer end_of_gorp

temporary:
    lpoly geep
    point kp 1.0 2.0 0.0 kp point!

public:

method draw
{
    p1 draw
    class.startdraw penup 90.0 right 3.0 size 2@ f* forward
        90.0 left pendown p2 draw
    class.startdraw penup 3.0 size 2@ f* forward pendown p3 draw
    class.startdraw penup 3.0 size 2@ f* 2dup forward 90.0 right forward
    90.0 left p4 draw
}

method acquire
{
    True
}

method new
{
    0.5 size 2!
    3 nsides !
    nsides @ dup p1 nsides <- ! 1+ dup p2 nsides <- ! 1+ dup p3 nsides <- !
                 1+ p4 nsides <- !
}

command method more
{
    p1 more p2 more p3 more p4 more
}

command method pgeep
{
    geep nsides ? geep size <- f? cr
}

2variable two 2.0 two 2!
variable one 1 one !
80 string bilge "anything" bilge s!
40 string huey "Huey" huey s!

external command method arghhh ((
    characters "String" bilge default arg
    point "Position" kp default arg
    real "Real number" two default arg
    distance "Distance" two default arg
    integer "Integer number" one default arg
    angle "Angle" kp basepoint arg
    orientation "Orientation" kp basepoint arg
    corner "Corner" kp basepoint arg
    keyword "Huey, Dewey, or Louie" "Huey Dewey Louie" keywords
        huey default arg
                       ))
{
}
