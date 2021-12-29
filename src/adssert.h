
/* Special assertion handler for ADS applications. */

#ifndef NDEBUG
#define assert(ex) {if (!(ex)){ads_printf("\nAssertion (%s) failed: file \"%s\", line %d\n"," ex ",__FILE__,__LINE__);ads_abort("Assertion failed.");}}
#else
#define assert(ex)
#endif
