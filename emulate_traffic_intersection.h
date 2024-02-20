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

typedef enum traffic_light_color {
    RED, GREEN, YELLOW
} traffic_light_color;

typedef struct position {
    int x; /* from 0 to 2 only */
    int y; /* from 0 to 2 only */
} position;

struct traffic_intersection_map;

typedef struct vehicle {
    /* thread id */
    uintptr_t vehicle_no;
    direction from;
    direction to;
    position pos;
    pthread_t handler;
    struct traffic_intersection_map *imap;
} vehicle;

/* Define one intersection */
typedef struct traffic_intersection_map {
    synched_thread_wait_queue *wq;
    traffic_light_color horizontal_direction;
    traffic_light_color vertical_direction;
    linked_list *vehicles;
} traffic_intersection_map;

void *vmalloc(size_t size);
void print_vehicle(vehicle *v);
void print_intersection_map(traffic_intersection_map *imap);
vehicle *create_vehicle(uintptr_t vehicle_no, direction from);
traffic_intersection_map *create_intersection_map(void);
void insert_vehicle_into_intersection_map(traffic_intersection_map *imap,
					  vehicle *v);
void place_moving_vehicle_on_map(traffic_intersection_map *imap,
				 vehicle *v);

#endif
