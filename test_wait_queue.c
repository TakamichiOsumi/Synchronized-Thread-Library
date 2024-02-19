#include <stdio.h>
#include "emulate_traffic_intersection.h"

int
main(int argc, char *argv[]){
    vehicle *v;
    traffic_intersection_map *map;

    v = create_vehicle(1, NORTH);
    map = create_intersection_map();

    print_vehicle(v);

    return 0;
}
