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

static int
mystrtoi(char *str, bool *success){
    long result;
    errno = 0;
    *success = false;

    result = strtol(str, NULL, 10);
    if(errno != 0){
	return 0;
    }else if (result == LONG_MIN || result == LONG_MAX){
	return 0;
    }else if (result <= INT_MIN || result >= INT_MAX){
	return 0;
    }

    *success = true;

    return (int) result;
}

static int
get_integer_within_range(char *description,
			 int lower_limit, int upper_limit){
    char buf[BUF_SIZE];
    bool success = false;
    int val = -1;

    do {
	/* Show the prompt for user */
	printf("%s", description);

	fgets(buf, sizeof(buf), stdin);
	buf[sizeof(buf) - 1] = '\0';

	val = mystrtoi(buf, &success);

	if (success == false ||
	    (val < lower_limit || upper_limit < val)){

	    fprintf(stderr, "invalid input value. try again\n");
	    /*
	     * This 'success' flag can be true, if the input gets
	     * converted to integer in nfc_strtol(). In this case,
	     * reset the flag and let the user input something else.
	     */
	    success = false;
	}

    }while(!success);

    return val;
}

static void
handle_user_input(traffic_intersection_map *imap){
    int val;

    while(1){
	printf("Choose one of the numbers : \n"
	       "[1] Change the traffic light\n"
	       "[2] Add a new vehicle into the map\n"
	       "[3] Print traffic intersection\n");
	val = get_integer_within_range(">>> ", 1, 3);

	printf("Got %d\n", val);
	switch(val){
	    case 1:
		break;
	    case 2:
		break;
	    case 3:
		print_intersection_map(imap);
		break;
	    default:
		break;
	}
    }
}

int
main(int argc, char *argv[]){
    traffic_intersection_map *imap;
    vehicle *v;
    int i;

    imap = create_intersection_map();
    for (i = 0; i < THREADS_NO; i++){
	v = create_vehicle(i, i % 4);
	place_moving_vehicle_on_map(imap, v);
    }

    assert(ll_get_length(imap->vehicles) == THREADS_NO);

    /* Wait for the startups of all the threads */
    sleep(2);

    handle_user_input(imap);

    pthread_exit(0);

    return 0;
}
