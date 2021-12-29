
\   The Blobby Man class definition

\   See Jim Blinn's Corner, IEEE Computer Graphics and Applications,
\   October 1987, Page 59.

PUBLIC:

    distance stature                  \ Height

    real bending                      \ Extension: bending forward/backward
    real rotation                     \ Rotation: body and shoulders around Z
    real leaning                      \ Angle of body leaning (Y axis)
    real head_nod                     \ Head nod
    real head_shake                   \ Head shake

    \ Left...

    real l_kick_direction             \ Angle direction leg is kicked
    real l_kick_angle                 \ Angular distance leg is kicked
    real l_leg_twist                  \ Angle leg twisted around its length
    real l_knee_bend                  \ Knee bend
    real l_ankle_bend                 \ Ankle bend
    real l_arm_to_side                \ Arm rotation to side
    real l_arm_fwd_back               \ Arm rotation forward and back
    real l_arm_twist                  \ Arm rotation around its own length
    real l_elbow_angle                \ Elbow angle

    \ Right...

    real r_kick_direction             \ Angle direction leg is kicked
    real r_kick_angle                 \ Angular distance leg is kicked
    real r_leg_twist                  \ Angle leg twisted around its length
    real r_knee_bend                  \ Knee bend
    real r_ankle_bend                 \ Ankle bend
    real r_arm_to_side                \ Arm rotation to side
    real r_arm_fwd_back               \ Arm rotation forward and back
    real r_arm_twist                  \ Arm rotation around its own length
    real r_elbow_angle                \ Elbow angle

PRIVATE:

0 constant X
1 constant Y
2 constant Z

pi 180.0 f/ 2constant kdtr

: dtr kdtr f* ;

: X-rot
    2@ dtr X xrot
;

: Y-rot
    2@ dtr Y xrot
;

: Z-rot
    2@ dtr Z xrot
;

: clr
    0.0 rot 2!
;

: uparm                               ( Upper arm )
    xpush
        0.0 0.0 -0.275 xtran
        0.09 0.09 0.275 xscal
        sphere
    xpop
;

: lowarm                              ( Lower arm )
    xpush
        0.0 0.0 -0.25 xtran
        0.08 0.08 0.25 xscal
        sphere
    xpop
;

: hand                                ( Hand )
    xpush
        0.0 0.0 -0.116 xtran
        0.052 0.091 0.155 xscal
        sphere
    xpop
;

: thigh                               ( Thigh )
    xpush
        0.0 0.0 -0.425 xtran
        0.141 0.141 0.425 xscal
        sphere
    xpop
;

: calf                                ( Calf )
    xpush
        0.05 0.05 0.05 xscal
        sphere
    xthen
        0.0 0.0 -0.425 xtran
        0.1 0.1 0.425 xscal
        sphere
    xpop
;

: foot                                ( Foot )
    xpush
        0.05 0.04 0.04 xscal
        sphere
    xthen
        0.0 0.05 -0.05 xtran
        0.04 0.04 0.04 xscal
        sphere
    xthen
        0.0 -0.15 0.05 xtran
        -10.0 dtr X xrot
        0.08 0.19 0.05 xscal
        sphere
    xpop
;

: head                                ( Head )
    xpush
        0.0 0.0 0.4 xtran
        0.2 0.23 0.3 xscal
        sphere
    xthen
        0.0 -0.255 0.42 xtran
        0.035 0.075 0.035 xscal
        sphere
    xthen
        0.0 0.0 0.07 xtran
        0.065 0.065 0.14 xscal
        sphere
    xthen
        0.0 -0.162 0.239 xtran
        0.0533 0.0508 0.0506 xscal
        sphere
    xpop
;

: leftarm                             ( Left arm )
    xpush
        uparm
        0.0 0.0 -0.55 xtran
        l_elbow_angle X-rot
        lowarm
        0.0 0.0 -0.5 xtran
        hand
    xpop
;

: rghtarm                             ( Right arm )
    xpush
        uparm
        0.0 0.0 -0.55 xtran
        r_elbow_angle X-rot
        lowarm
        0.0 0.0 -0.5 xtran
        hand
    xpop
;

: leftleg                             ( Left leg )
    xpush
        l_kick_direction Z-rot
        l_kick_angle Y-rot
        l_kick_direction 2@ dtr fnegate Z xrot
        l_leg_twist Z-rot
        thigh
        0.0 0.0 -0.85 xtran
        l_knee_bend X-rot
        calf
        0.0 0.0 -0.84 xtran
        l_ankle_bend X-rot
        foot
    xpop
;

: rghtleg                             ( Right leg )
    xpush
        r_kick_direction Z-rot
        r_kick_angle Y-rot
        r_kick_direction 2@ dtr fnegate Z xrot
        r_leg_twist Z-rot
        thigh
        0.0 0.0 -0.85 xtran
        r_knee_bend X-rot
        calf
        0.0 0.0 -0.84 xtran
        r_ankle_bend X-rot
        foot
    xpop
;

: shoulder                            ( Shoulder, head, and arms )
    xpush
        0.45 0.153 0.12 xscal
        sphere
    xthen
        0.0 0.0 0.153 xtran
        head_nod X-rot
        head_shake Z-rot
        head
    xthen
        -0.45 0.0 0.0 xtran
        l_arm_to_side Y-rot
        l_arm_fwd_back X-rot
        l_arm_twist Z-rot
        leftarm
    xthen
        0.45 0.0 0.0 xtran
        r_arm_to_side Y-rot
        r_arm_fwd_back X-rot
        r_arm_twist Z-rot
        rghtarm
    xpop
;

: body                                ( Body of the blobby man )
    xpush
        0.0 0.0 0.62 xtran
        0.306 0.21 0.5 xscal
        sphere
    xthen
        0.0 0.0 1.0 xtran
        bending X-rot
        leaning Y-rot
        rotation Z-rot
        shoulder
    xpop
;

: torso                               ( Assemble complete figure )
    xpush
        -0.178 0.0 0.0 xtran
        leftleg
    xthen
        0.178 0.0 0.0 xtran
        rghtleg
    xthen
        0.0 0.0 0.08 xtran
        0.275 0.152 0.153 xscal
        sphere
    xthen
        bending X-rot
        leaning Y-rot
        rotation Z-rot
        body
    xpop
;

PUBLIC:

method draw
{
    xpush
        stature 2@ 2dup 2dup xscal
        0.0 0.0 1.75 xtran
        torso
    xpop
}


PUBLIC:

command method attention
{

    bending clr
    rotation clr
    leaning clr
    head_nod clr
    head_shake clr
    l_kick_direction clr
    l_kick_angle clr
    l_leg_twist clr
    l_knee_bend clr
    l_ankle_bend clr
    l_arm_to_side clr
    l_arm_fwd_back clr
    l_arm_twist clr
    l_elbow_angle clr
    r_kick_direction clr
    r_kick_angle clr
    r_leg_twist clr
    r_knee_bend clr
    r_ankle_bend clr
    r_arm_to_side clr
    r_arm_fwd_back clr
    r_arm_twist clr
    r_elbow_angle clr
}

command method wave
{
    this attention
    -25.0 head_nod 2!
    28.0 head_shake 2!
    205.0 r_kick_direction 2!
    13.0 r_kick_angle 2!
    -86.0 r_leg_twist 2!
    -53.0 r_knee_bend 2!
    -45.0 l_arm_to_side 2!
    112.0 r_arm_to_side 2!
    40.0 r_arm_fwd_back 2!
    -90.0 l_arm_twist 2!
    -102.0 r_arm_twist 2!
    90.0 l_elbow_angle 2!
    85.0 r_elbow_angle 2!
}

command method grow
{
    1.5 stature 2@ f* stature 2!
}

command method shrink
{
    2.0 3.0 f/ stature 2@ f* stature 2!
}

PROTECTED:

method acquire
{
    4.0 stature 2!
    this attention
    true
}

