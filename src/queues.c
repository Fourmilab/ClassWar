/*

        Queue manipulation routines

        Designed and implemented by John Walker in January 1986

        Derived from the Masterstroke multitasking process scheduler,
        and before that the Marinchip NOS/MT scheduler, the Marinchip
        Disc Executive kernel, the ISD Interactive Network real time
        operating system, the FANG task handler, and CHI/OS.

*/

#include <stdio.h>
#include "adssert.h"                  /* Special <assert.h> for ADS programs */

#include "queues.h"

#define FALSE    0
#define TRUE     1

typedef int Boolean;

/*  QINIT  --  Initialise links for null queue  */

void qinit(qhead)
struct queue *qhead;
{
        qhead->qnext = qhead->qprev = qhead;
}

/*  QINSERT  --  Insert object at end of queue  */

void qinsert(qhead, object)
struct queue *qhead, *object;
{
        assert(qhead->qprev->qnext == qhead);
        assert(qhead->qnext->qprev == qhead);

        object->qnext = qhead;
        object->qprev = qhead->qprev;
        qhead->qprev = object;
        object->qprev->qnext = object;
}

/*  QPUSH  --  Push object at start of queue  */

void qpush(qhead, object)
struct queue *qhead, *object;
{
        assert(qhead->qprev->qnext == qhead);
        assert(qhead->qnext->qprev == qhead);

        object->qprev = qhead;
        object->qnext = qhead->qnext;
        qhead->qnext = object;
        object->qnext->qprev = object;
}

/*  QREMOVE  --  Remove object from queue.  Returns NULL if queue empty  */

struct queue *qremove(qhead)
struct queue *qhead;
{
        struct queue *object;

        assert(qhead->qprev->qnext == qhead);
        assert(qhead->qnext->qprev == qhead);

        if ((object = qhead->qnext) == qhead)
           return NULL;
        qhead->qnext = object->qnext;
        object->qnext->qprev = qhead;
        return object;
}

/*  QNEXT  --  Get next object from queue nondestructively. Returns
               NULL at end of queue */

struct queue *qnext(qthis, qhead)
struct queue *qthis;
struct queue *qhead;
{
        struct queue *object;

        if ((object = qthis->qnext) == qhead)
           object = NULL;
        return object;
}

/*  QDCHAIN  --  Dequeue an item from the middle of a queue.  Passed
                 the queue item, returns the (now dechained) queue item. */

struct queue *qdchain(qitem)
struct queue *qitem;
{
        assert(qitem->qprev->qnext == qitem);
        assert(qitem->qnext->qprev == qitem);

        return qremove(qitem->qprev);
}

/*  QLENGTH  --  Return number of items on queue, zero if queue empty.
                 Note that this must traverse all the links of the
                 queue, so if all you're testing is whether the queue
                 is empty or not, you should use the qempty(queue)
                 macro (defined in queues.h) which uses qnext() to
                 detect an empty queue much faster. */

int qlength(qhead)
struct queue *qhead;
{
        int l;
        struct queue *qp;

        l = 0;
        qp = qhead->qnext;

        while (qp != qhead) {
           l++;
           qp = qp->qnext;
        }
        return l;
}


/*  QVALID  --  Validates all of the links in a queue.  Returns
                1 if all of the links are valid and 0 otherwise.
                Note that if the queue contains any bad pointers
                this routine may crash.  */

Boolean qvalid(qhead)
struct queue *qhead;
{
        struct queue *qp;

        qp = qhead;
        if (qp == NULL)
           return FALSE;

        do {
           if ((qp->qnext == NULL) ||
               (qp->qprev == NULL) ||
               (qp->qnext->qprev != qp) ||
               (qp->qprev->qnext != qp))
              return FALSE;
           qp = qp->qnext;
        } while (qp != qhead);

        return TRUE;
}
