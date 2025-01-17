#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include "reman.h"

#define NUMR 5 // Number of resources
#define NUMT 3 // Number of threads

// Utility function to print resource requests and holdings
void pr(int tid, char astr[], int m, int r[]) {
    int i;
    printf("Thread %d, %s, [", tid, astr);
    for (i = 0; i < m; ++i) {
        if (i == (m - 1))
            printf("%d", r[i]);
        else
            printf("%d,", r[i]);
    }
    printf("]\n");
}

// Utility function to set arrays
void setarray(int r[], int m, ...) {
    va_list valist;
    int i;
    va_start(valist, m);
    for (i = 0; i < m; i++) {
        r[i] = va_arg(valist, int);
    }
    va_end(valist);
}

// Thread function for Thread 1
void *threadfunc1(void *a) {
    int tid = *((int *)a);
    int claim[NUMR];
    int request1[NUMR];
    int request2[NUMR];
    int release[NUMR];

    reman_connect(tid);

    // Declare maximum resources this thread may claim
    setarray(claim, NUMR, 0, 1, 1, 0, 0); // Max claim
    reman_claim(claim);

    // First request
    setarray(request1, NUMR, 0, 0, 1, 0, 0); // Request Resource 3
    pr(tid, "REQ", NUMR, request1);
    reman_request(request1);

    sleep(5); // Simulate some work

    // Second request
    setarray(request2, NUMR, 0, 1, 0, 0, 0); // Request Resource 2
    pr(tid, "REQ", NUMR, request2);
    reman_request(request2);

    // Release resources
    setarray(release, NUMR, 0, 0, 1, 0, 0); // Release Resource 3
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    setarray(release, NUMR, 0, 1, 0, 0, 0); // Release Resource 2
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    reman_disconnect();
    pthread_exit(NULL);
}

// Thread function for Thread 2
void *threadfunc2(void *a) {
    int tid = *((int *)a);
    int claim[NUMR];
    int request1[NUMR];
    int request2[NUMR];
    int release[NUMR];

    reman_connect(tid);

    // Declare maximum resources this thread may claim
    setarray(claim, NUMR, 1, 1, 0, 0, 0); // Max claim
    reman_claim(claim);

    // First request
    setarray(request1, NUMR, 0, 1, 0, 0, 0); // Request Resource 2
    pr(tid, "REQ", NUMR, request1);
    reman_request(request1);

    sleep(3);

    // Second request
    setarray(request2, NUMR, 1, 0, 0, 0, 0); // Request Resource 1
    pr(tid, "REQ", NUMR, request2);
    reman_request(request2);

    // Release resources
    setarray(release, NUMR, 1, 0, 0, 0, 0); // Release Resource 1
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    setarray(release, NUMR, 0, 1, 0, 0, 0); // Release Resource 2
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    reman_disconnect();
    pthread_exit(NULL);
}

// Thread function for Thread 3
void *threadfunc3(void *a) {
    int tid = *((int *)a);
    int claim[NUMR];
    int request1[NUMR];
    int request2[NUMR];
    int release[NUMR];

    reman_connect(tid);

    // Declare maximum resources this thread may claim
    setarray(claim, NUMR, 1, 1, 0, 0, 0); // Max claim
    reman_claim(claim);

    // First request
    setarray(request1, NUMR, 1, 0, 0, 0, 0); // Request Resource 1
    pr(tid, "REQ", NUMR, request1);
    reman_request(request1);

    sleep(1);

    // Second request
    setarray(request2, NUMR, 0, 1, 0, 0, 0); // Request Resource 2
    pr(tid, "REQ", NUMR, request2);
    reman_request(request2);

    // Release resources
    setarray(release, NUMR, 0, 1, 0, 0, 0); // Release Resource 2
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    setarray(release, NUMR, 1, 0, 0, 0, 0); // Release Resource 1
    pr(tid, "REL", NUMR, release);
    reman_release(release);

    reman_disconnect();
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    pthread_t threads[NUMT];
    int tids[NUMT];
    int ret;

    if (argc != 2) {
        printf("Usage: %s <avoid_flag>\n", argv[0]);
        return 1;
    }

    int avoid = atoi(argv[1]);
    if (avoid == 1) {
        printf("Deadlock avoidance enabled.\n");
        reman_init(NUMT, NUMR, 1);
    } else {
        printf("Deadlock avoidance disabled.\n");
        reman_init(NUMT, NUMR, 0);
    }

    // Create threads
    for (int i = 0; i < NUMT; i++) {
        tids[i] = i;
        if (i == 0) {
            pthread_create(&threads[i], NULL, threadfunc1, &tids[i]);
        } else if (i == 1) {
            pthread_create(&threads[i], NULL, threadfunc2, &tids[i]);
        } else if (i == 2) {
            pthread_create(&threads[i], NULL, threadfunc3, &tids[i]);
        }
    }

    // Monitor for deadlocks
    for (int i = 0; i < 10; i++) {
        sleep(1);
        reman_print("Current System State");
        ret = reman_detect();
        if (ret > 0) {
            printf("Deadlock detected! Number of deadlocked threads: %d\n", ret);
            reman_print("System State at Deadlock");
            break; // Exit the loop after detecting deadlock
        }
    }

    if (ret == 0) {
        for (int i = 0; i < NUMT; ++i) {
        pthread_join (threads[i], NULL);
        printf ("joined\n");
        }
    }
    return 0;
}
