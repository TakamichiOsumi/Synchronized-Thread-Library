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

    v = (vehicle *) data;
    if (v->vehicle_no == (uintptr_t) key)
        return true;
    else
        return false;
}

char *
traffic_light_color_char(traffic_light_color color){
    switch(color){
	case RED:
	    return "RED";
	case GREEN:
	    return "GREEN";
	case YELLOW:
	    return "YELLOW";
	default:
	    assert(0);
	    return NULL;
    }
}

void
print_intersection_map(traffic_intersection_map *imap){
    node *n;

    printf("---- The current map ----\n");
    printf("traffic light : vertical direction : %s\n",
	   traffic_light_color_char(imap->horizontal_direction));
    printf("traffic light : horizontal direction : %s\n",
	   traffic_light_color_char(imap->vertical_direction));


    if (ll_is_empty(imap->vehicles)){
	printf("--------------------------\n");
	return;
    }else{
	printf("---- The current vehicles in the map ----\n");
	n = imap->vehicles->head;
	while(n){
	    print_vehicle((vehicle *) n->data);
	    n = n->next;
	}
	printf("--------------------------\n");
    }
}

traffic_intersection_map *
create_intersection_map(void){
    traffic_intersection_map *map;

    map = (traffic_intersection_map *) vmalloc(sizeof(traffic_intersection_map));
    map->horizontal_direction = RED;
    map->vertical_direction = RED;
    map->wq = synched_thread_wait_queue_init();
    map->vehicles = ll_init(vehicle_key_match);

    return map;
}

void
insert_vehicle_into_intersection_map(traffic_intersection_map *imap,
				      vehicle *v){
    if (!imap || !v)
	return;

    ll_insert(imap->vehicles, (void *) v);
}

static void *
let_vehicle_move(void *arg){
    traffic_intersection_map *imap;
    vehicle *v;

    v = (vehicle *) arg;
    imap = v->imap;

    printf("vehicle (id = %lu) has been set\n", v->vehicle_no);

    return NULL;
}

void
place_moving_vehicle_on_map(traffic_intersection_map *imap,
			    vehicle *v){
    if (!imap || !v)
	return;

    insert_vehicle_into_intersection_map(imap, v);

    v->imap = imap;

    pthread_create(&v->handler, NULL, let_vehicle_move, (void *) v);
}
