#ifndef __TRAFFIC_INTERSECTION__
#define __TRAFFIC_INTERSECTION__

#include "stdint.h"
#include "synched_thread_core.h"
#include "Linked-List/linked_list.h"

/*
 * Emulate a simplified intersection to demonstrate wait queue functionality.
 * There is one traffic light and some vehicles (threads) only.
 */
typedef enum direction {
    NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3
} direction;

/* Define the life cycle of a vehicle in the map */
typedef enum vehicle_status {
    ENTER,
    WAIT,
    PASS,
    EXIT
} vehicle_status;

typedef struct position {
    int x; /* from 0 to 2 only */
    int y; /* from 0 to 2 only */
} position;

typedef struct vehicle {
    /* thread id */
    uintptr_t vehicle_no;
    direction from;
    direction to;
    vehicle_status status;
    position pos;
} vehicle;

/* Define one intersection */
typedef struct traffic_intersection_map {
    synched_thread_wait_queue *wq;
    linked_list *vehicles;
} traffic_intersection_map;

void *vmalloc(size_t size);
void print_vehicle(vehicle *v);
vehicle *create_vehicle(uintptr_t vehicle_no, direction from);
traffic_intersection_map *create_intersection_map(void);

#endif
