#ifndef __TRAFFIC_INTERSECTION__
#define __TRAFFIC_INTERSECTION__

#include "stdint.h"
#include "synched_thread_core.h"
#include "Linked-List/linked_list.h"

/*
 * Emulate a simplified intersection to demonstrate wait queue functionality.

 * There is one traffic light and some vehicles (threads) only.
 */
typedef enum DIRECTION {
    EAST = 0, WEST, NORTH, SOUTH
} DIRECTION;

/* Define the life cycle of a vehicle in the map */
typedef enum VEHICLE_STATUS {
    ENTER,
    WAIT,
    PASS,
    EXIT
} VEHICLE_STATUS;

typedef struct car {
    uintptr_t car_no; /* Works as thread_id also */
    DIRECTION start;
    DIRECTION goal; /* Defined by 'start' */
    VEHICLE_STATUS status;
} car;

typedef struct traffic_intersection_map {
    synched_thread_wait_queue *wq;
    linked_list *vehicles;
} traffic_intersection_map;


#endif
