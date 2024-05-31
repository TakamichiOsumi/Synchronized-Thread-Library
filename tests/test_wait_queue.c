#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "emulate_traffic_intersection.h"

#define THREADS_NO 8
#define BUF_SIZE 16

static uintptr_t vehicle_no = 1;

void
basic_wq_test(traffic_intersection_map *imap){
    /* Check the number of vehicles is same as that of threads */
    test_vehicles_number_in_map(imap, THREADS_NO);

    /* Allow four vehicles to pass the traffic intersection */
    imap->vertical_direction = GREEN;
    synched_thread_wait_queue_signal(imap->wq, true, true);
    /* Wait for the vehicles leave */
    sleep(3);
    test_vehicles_number_in_map(imap, 4);

    /* Allow four left vehicles to pass the traffic intersection */
    synched_thread_wait_queue_signal(imap->wq, true, true);
    imap->horizontal_direction = GREEN;
    /* Wait for the vehicles leave */
    sleep(3);
    test_vehicles_number_in_map(imap, 0);
}

void
dynamic_wq_test(traffic_intersection_map *imap){
    vehicle *v1, *v2;

    test_vehicles_number_in_map(imap, 0);

    imap->horizontal_direction = GREEN;
    imap->vertical_direction = RED;

    /* A new vehicle v1 can pass the intersection with green traffic light */
    v1 = create_vehicle(vehicle_no++, EAST);
    place_moving_vehicle_on_map(imap, v1);
    sleep(2);
    test_vehicles_number_in_map(imap, 0);

    /* A new vehicle v2 gets blocked the intersection with red/yellow traffic light */
    v2 = create_vehicle(vehicle_no++, NORTH);
    place_moving_vehicle_on_map(imap, v2);
    sleep(2);
    /* The v2 vehicle should be blocked */
    test_vehicles_number_in_map(imap, 1);
    imap->vertical_direction = GREEN;
    synched_thread_wait_queue_signal(imap->wq, true, true);
    sleep(2);
    /* The v2 vehicle should be released by the color change and the signal */
    test_vehicles_number_in_map(imap, 0);
}

int
main(int argc, char *argv[]){
    traffic_intersection_map *imap;
    vehicle *v;
    int i;

    imap = create_intersection_map();

    /*
     * Create four vertically moving vehicles and
     * four horizontally moving vehicles each.
     */
    for (i = 1; i <= THREADS_NO; i++){
	v = create_vehicle(vehicle_no++, i % 4);
	place_moving_vehicle_on_map(imap, v);
    }

    basic_wq_test(imap);

    dynamic_wq_test(imap);

    pthread_exit(0);

    return 0;
}
