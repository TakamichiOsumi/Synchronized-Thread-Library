#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "emulate_traffic_intersection.h"
#include "Linked-List/linked_list.h"

void *
vmalloc(size_t size){
    void *p;

    if ((p = malloc(size)) == NULL){
	perror("malloc");
	exit(-1);
    }

    return p;
}

void
print_vehicle(vehicle *v){
    printf("No=%lu : pos[%d, %d]\n", v->vehicle_no,
	   v->pos.x, v->pos.y);
}

vehicle *
create_vehicle(uintptr_t vehicle_no, direction from){
    vehicle *v;

    v = (vehicle *) vmalloc(sizeof(vehicle));
    v->vehicle_no = vehicle_no;
    v->from = from;
    v->to = (from + 2) % 4;
    switch(from){
	case NORTH:
	    v->pos.x = 1;
	    v->pos.y = 0;
	    break;
	case EAST:
	    v->pos.x = 2;
	    v->pos.y = 1;
	    break;
	case SOUTH:
	    v->pos.x = 1;
	    v->pos.y = 2;
	    break;
	case WEST:
	    v->pos.x = 0;
	    v->pos.y = 1;
	    break;
	default:
	    assert(0);
	    break;
    }

    return v;
}

bool
vehicle_key_match(void *data, void *key){
    vehicle *v;

    /* 'data' is (void *) &vehicle */
    v = (vehicle *) data;
    if (v->vehicle_no == (uintptr_t) key)
        return true;
    else
        return false;
}

traffic_intersection_map *
create_intersection_map(void){
    traffic_intersection_map *map;

    map = (traffic_intersection_map *) vmalloc(sizeof(traffic_intersection_map));
    map->wq = synched_thread_wait_queue_init();
    map->vehicles = ll_init(vehicle_key_match);

    return map;
}
