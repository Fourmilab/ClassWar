/*

        Queue definitions

*/


/*  General purpose queue  */

struct queue {
        struct queue *qnext,       /* Next item in queue */
                     *qprev;       /* Previous item in queue */
};

/*  Queue macros  */

#define qempty(x) (qnext((struct queue *)(x),(struct queue *)(x))==NULL)

/*  Queue functions  */

void qinit(), qinsert(), qpush();
int qlength(), qvalid();
struct queue *qremove(), *qnext(), *qdchain();
