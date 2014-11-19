#ifndef QUEUE_H
#define QUEUE_H

// Settings
#define QUEUE_DEPTH 20
#define NAME_LIMIT 20
typedef uint16_t queue_time_t;

typedef void (*queuedFunction)();

typedef struct _queue_item
{
    queuedFunction fPtr;
    queue_time_t recur;
    queue_time_t next;
    char itemName[NAME_LIMIT];
	int8_t priority; //-5 to +5
} queueItem;

//Functions
int scheduleFunction(queuedFunction, const char *, queue_time_t, queue_time_t, int8_t priority);
int scheduleRemoveFunction(const char *);
int scheduleChangeFunction(const char *, queue_time_t, queue_time_t);

int scheduleRun(queue_time_t);

#endif