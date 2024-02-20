#include <assert.h>
#include <stdio.h>
#include "emulate_traffic_intersection.h"

#define THREADS_NO 8
int
main(int argc, char *argv[]){
    vehicle *v;
    traffic_intersection_map *imap;
    int i;

    imap = create_intersection_map();
    for (i = 0; i < THREADS_NO; i++){
	v = create_vehicle(i, i % 4);
	place_moving_vehicle_on_map(imap, v);
    }

    assert(ll_get_length(imap->vehicles) == THREADS_NO);

    print_intersection_map(imap);

    pthread_exit(0);

    return 0;
}
