#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "queue.h"

static uint8_t queueStart = 0;
static uint8_t queueEnd = 0;
static uint8_t itemsInQueue = 0;

static queueItem scheduleQueue[QUEUE_DEPTH];

static int scheduleQueueGetTop(queueItem *returnItem);
static int internalScheduleFunction(queueItem pItem);

int scheduleFunction(queuedFunction pFunction, const char *pId, queue_time_t pInitialRun)
{
    int rv = 0;

    queueItem newItem;
    newItem.fPtr = pFunction;
	memset(newItem.itemName, 0, NAME_LIMIT);
    memcpy(newItem.itemName, pId, strlen(pId));
    newItem.recur = pInitialRun;
    newItem.next = pInitialRun;

    rv = internalScheduleFunction(newItem);

    return rv;
}

int scheduleRemoveFunction(const char *pId)
{
    queueItem target;
    int rv = 0;
    for (int i = 0; i < itemsInQueue; ++i)
    {
        if(scheduleQueueGetTop(&target) == 0)
        {
            if(strcmp(target.itemName, pId) != 0)
            {
                internalScheduleFunction(target);
            }
        } else {
            rv = -1;
            break;
        }
    }

    return rv;
}

int scheduleChangeFunction(const char *pId, queue_time_t pNewNext, queue_time_t pNewRecur)
{
    queueItem target;
    int rv = 0;
    for (int i = 0; i < itemsInQueue; ++i)
    {
        if(scheduleQueueGetTop(&target) == 0)
        {
            if(strcmp(target.itemName, pId) == 0)
            {
				printf("ChangeFunc : %s => %u \r\n", target.itemName, pNewRecur);
                target.next = pNewNext;
                target.recur = pNewRecur;
				rv++;
            }
            internalScheduleFunction(target);
        } else {
            rv = -1;
            break;
        }
    }

    return rv;
}

int scheduleRun(queue_time_t pNow)
{
    queueItem target;
    int rv = 0;
		for (int i = 0; i < itemsInQueue; ++i)
		{
			if(scheduleQueueGetTop(&target)==0)
			{
				if (target.next <= pNow)
				{
					if(target.recur != 0)
					{
						target.next = pNow + target.recur;
						internalScheduleFunction(target);
					}
					//printf("%u : Running %s \r\n", pNow, target.itemName);
					target.fPtr();
					rv++;
				} else {
					internalScheduleFunction(target);
				}
			} else {
				rv = -1;
				break;
			}
	}

    return rv;
}

static int scheduleQueueGetTop(queueItem *returnItem)
{
    int rv = 0;
    //Remove the top item, stuff it into returnItem
    if (queueEnd != queueStart) {
            queueItem tempQueueItem = scheduleQueue[queueStart];
            //This Algorithm also from Wikipedia.
            queueStart = (queueStart + 1) % QUEUE_DEPTH;
            *returnItem = tempQueueItem;
            itemsInQueue--;
    } else {
    //if the buffer is empty, return an error code
        rv = -1;
    }

    return rv;   
}

static int internalScheduleFunction(queueItem pItem)
{
    int rv = 0;
    if ((queueEnd + 1) % QUEUE_DEPTH != queueStart) {
        scheduleQueue[queueEnd] = pItem;
        queueEnd = (queueEnd + 1) % QUEUE_DEPTH;
        itemsInQueue++;
    } else {
        rv = -1;
    }
    return rv;
}