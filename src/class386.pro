/*

    Compiler profile to DeReam ClassWar with the PharLap
    compatible version of SourNote C.

*/

#define MSDOS       1
#define HIGHC       1
#define OS2         0
#define P386        1
#define ANSIARGS    1

#define MEMSTAT     1
#define EXPORT
#define ADS         1

pragma On(PCC_msgs);                  /* Shut the fuck up, ok? */

#define unlink(x) remove(x)           /* Oh thank you, Santa Cruz assholes */
