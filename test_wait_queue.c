#include <stdio.h>
#include "emulate_traffic_intersection.h"

int
main(int argc, char *argv[]){
    vehicle *v;

    v = create_vehicle(1, NORTH);

    print_vehicle(v);

    return 0;
}
