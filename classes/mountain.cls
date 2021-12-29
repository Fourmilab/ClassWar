

\   Fractal mountain class definition

PUBLIC:

    integer mesh_size
    real    fractal_dimension
    real    power_factor
    integer colour_mode
    integer random_seed

PRIVATE:
    static integer interface_level

method newclass
{
    1 interface_level !
}

external command method draw
{
}

method acquire
{
    8 mesh_size !
    1.75 fractal_dimension 2!
    1.0 power_factor 2!
    0 colour_mode !
    0 random_seed !
    this object.inspect
}

PUBLIC:

2variable two 2.0 two 2!

external command method rougher (( real "How much rougher" two default
                                  ARG_nozero ARG_noneg or argmodes
                                  "Little Lots Smooth" keywords
                                  arg ))
{
}
