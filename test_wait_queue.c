#include <stdio.h>
#include "emulate_traffic_intersection.h"

int
main(int argc, char *argv[]){
    vehicle *v;
    traffic_intersection_map *imap;

    v = create_vehicle(1, NORTH);
    imap = create_intersection_map();

    print_vehicle(v);
    print_intersection_map(imap);

    return 0;
}
