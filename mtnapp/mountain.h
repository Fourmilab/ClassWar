/*

    External method interface definition for class MOUNTAIN

*/

typedef struct {

    /* Class variables */

    long interface_level;

    /* Instance variables */

    long mesh_size;
    ads_real fractal_dimension;
    ads_real power_factor;
    long colour_mode;
    long random_seed;
} s_mountain;

static s_mountain mountain;

/* Message protocol description */

#define Gv(x) ((char *) &(mountain.x))
#define Gp(x) ((char *)  (mountain.x))

#ifndef m_Protocol_defined
#define m_Protocol_defined 1
typedef struct {
    int p_type;
    char *p_field;
} m_Protocol;
typedef struct {
    char *methname;
    void (*methfunc)();
} method_Item;
extern int beg_method(), end_method();
extern void define_class();
#endif

static m_Protocol mP_mountain[] = {
    { 71, Gv(interface_level) },
    { 71, Gv(mesh_size) },
    { 40, Gv(fractal_dimension) },
    { 40, Gv(power_factor) },
    { 71, Gv(colour_mode) },
    { 71, Gv(random_seed) },
    {  0, NULL}
};
#undef Gv
#undef Gp

/* Arguments for ROUGHER method */

typedef struct {
    struct {
        int kwflag;
        union {
            char *kwtext;
            ads_real value;
        } kw;
    } arg1;
} aS_rougher;

static aS_rougher rougher;

/* Protocol for ROUGHER method */

#define Gv(x) ((char *) &(rougher.x))
#define Gp(x) ((char *)  (rougher.x))

static m_Protocol aP_rougher[] = {
    {  -40, Gv(arg1) },
    {   0, NULL}
};
#undef Gv
#undef Gp

extern void M_rougher_mountain();
#define rougher_mountain void M_rougher_mountain() { if (beg_method(mP_mountain, aP_rougher) != RTNORM) return; {
#define end_rougher_mountain } end_method(); }

/* Protocol for DRAW method */

static m_Protocol aP_draw[] = {
    {   0, NULL}
};

extern void M_draw_mountain();
#define draw_mountain void M_draw_mountain() { if (beg_method(mP_mountain, aP_draw) != RTNORM) return; {
#define end_draw_mountain } end_method(); }

/* Method definition table */

method_Item MOUNTAIN[] = {
    {"MOUNTAIN.ROUGHER", M_rougher_mountain},
    {"MOUNTAIN.DRAW", M_draw_mountain},
    {NULL, 0}
};
#define mountain_methods void main(c,v)int c;char *v[];{main_method(c,v,MOUNTAIN);}
