
\   Three dimensional helix

    PRIVATE:

        REAL stepsize
        REAL yaw_step
        REAL pitch_step
        REAL roll_step
        INTEGER step_count

: draw
    step_count @ 0 do
        stepsize 2@ forward
        yaw_step 2@ yaw
        pitch_step 2@ pitch
        roll_step 2@ rol
    loop
;

: new
    "Step size: " getreal drop stepsize 2!
    "Yaw step: " getreal drop yaw_step 2!
    "Pitch step: " getreal drop pitch_step 2!
    "Roll step: " getreal drop roll_step 2!
    "Number of steps: " getint drop step_count !
;
