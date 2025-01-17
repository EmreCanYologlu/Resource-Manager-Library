#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "reman.h"

// Global variables
int t_count; // number of threads
int r_count; // number of resources
int avoid;   // deadlock avoidance flag

int available[MAXR];            // Available resources
int claim[MAXT][MAXR];          // Claim matrix
int allocation[MAXT][MAXR];     // Allocation matrix
int request[MAXT][MAXR];        // Request matrix
int need[MAXT][MAXR];           // Need matrix, for deadlock avoidance

int connected[MAXT];            // Indicates if a thread is connected (1) or not (0)

pthread_mutex_t mutex;
pthread_cond_t cond;

pthread_key_t tid_key;
int tid_key_created = 0;

// Initialize reman library
int reman_init(int t_count_param, int r_count_param, int avoid_param)
{
    int ret = 0;
    int i, j;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // Check the parameters
    if (t_count_param <= 0 || t_count_param > MAXT || r_count_param <= 0 || r_count_param > MAXR) {
        fprintf(stderr, "reman_init: Invalid parameters\n");
        return -1;
    }

    pthread_mutex_lock(&mutex);

    t_count = t_count_param;
    r_count = r_count_param;
    avoid = avoid_param;

    // Initialize available resources
    for (i = 0; i < r_count; i++) {
        available[i] = 1; // Each resource has one instance
    }

    // Initialize claim, allocation, request, need matrices
    for (i = 0; i < t_count; i++) {
        connected[i] = 0; // No thread is connected yet
        for (j = 0; j < r_count; j++) {
            claim[i][j] = 0;
            allocation[i][j] = 0;
            request[i][j] = 0;
            need[i][j] = 0;
        }
    }

    pthread_mutex_unlock(&mutex);

    return (ret);
}

// Thread connects to reman
int reman_connect(int tid)
{
    int ret = 0;

    // Check if tid is valid
    if (tid < 0 || tid >= t_count) {
        fprintf(stderr, "reman_connect: Invalid tid %d\n", tid);
        return -1;
    }

    if (!tid_key_created) {
        pthread_key_create(&tid_key, NULL);
        tid_key_created = 1;
    }

    pthread_setspecific(tid_key, (void*)(intptr_t)(tid));

    pthread_mutex_lock(&mutex);

    if (connected[tid]) {
        fprintf(stderr, "reman_connect: Thread %d already connected\n", tid);
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    connected[tid] = 1;

    pthread_mutex_unlock(&mutex);

    return (ret);
}

// Thread disconnects from reman
int reman_disconnect()
{
    int ret = 0;
    int tid = -1;
    int i;

    if (!tid_key_created) {
        fprintf(stderr, "reman_disconnect: Thread not connected\n");
        return -1;
    }

    tid = (int)(intptr_t)(pthread_getspecific(tid_key));
    if (tid < 0 || tid >= t_count) {
        fprintf(stderr, "reman_disconnect: Thread not connected\n");
        return -1;
    }

    pthread_mutex_lock(&mutex);

    connected[tid] = 0;

    // Release any allocated resources
    for (i = 0; i < r_count; i++) {
        if (allocation[tid][i] > 0) {
            available[i] += allocation[tid][i];
            allocation[tid][i] = 0;
            need[tid][i] = 0;
        }
        request[tid][i] = 0;
    }

    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);

    return (ret);
}

// Thread claims maximum resources (only for avoidance)
int reman_claim (int claim_param[])
{
    int ret = 0;
    int tid = -1;
    int i;

    if (!tid_key_created) {
        fprintf(stderr, "reman_claim: Thread not connected\n");
        return -1;
    }

    tid = (int)(intptr_t)(pthread_getspecific(tid_key));
    if (tid < 0 || tid >= t_count) {
        fprintf(stderr, "reman_claim: Thread not connected\n");
        return -1;
    }

    pthread_mutex_lock(&mutex);

    if (avoid != 1) {
        fprintf(stderr, "reman_claim: Deadlock avoidance not enabled\n");
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    for (i = 0; i < r_count; i++) {
        if (claim_param[i] < 0 || claim_param[i] > 1) {
            fprintf(stderr, "reman_claim: Invalid claim for resource %d\n", i);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        claim[tid][i] = claim_param[i];
        need[tid][i] = claim[tid][i] - allocation[tid][i];
    }

    pthread_mutex_unlock(&mutex);

    return(ret);
}

// Thread requests resources
int reman_request (int request_param[])
{
    int ret = 0;
    int tid = -1;
    int i, j;
    int can_allocate;

    if (!tid_key_created) {
        fprintf(stderr, "reman_request: Thread not connected\n");
        return -1;
    }

    tid = (int)(intptr_t)(pthread_getspecific(tid_key));
    if (tid < 0 || tid >= t_count) {
        fprintf(stderr, "reman_request: Thread not connected\n");
        return -1;
    }

    pthread_mutex_lock(&mutex);

    // Copy the request
    for (i = 0; i < r_count; i++) {
        if (request_param[i] < 0 || request_param[i] > 1) {
            fprintf(stderr, "reman_request: Invalid request for resource %d\n", i);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        request[tid][i] = request_param[i];
        if (avoid == 1) {
            // Update need
            if (request[tid][i] > need[tid][i]) {
                fprintf(stderr, "reman_request: Request exceeds need for resource %d\n", i);
                pthread_mutex_unlock(&mutex);
                return -1;
            }
        } else {
            if (request[tid][i] > 1) {
                fprintf(stderr, "reman_request: Invalid request for resource %d\n", i);
                pthread_mutex_unlock(&mutex);
                return -1;
            }
        }
    }

    // Now try to allocate resources
    while (1) {
        // Check if request can be granted
        can_allocate = 1;

        // Check availability
        for (i = 0; i < r_count; i++) {
            if (request[tid][i] > available[i]) {
                can_allocate = 0;
                break;
            }
        }

        if (can_allocate && avoid == 1) {
            // Check for safety
            // Simulate allocation
            for (i = 0; i < r_count; i++) {
                available[i] -= request[tid][i];
                allocation[tid][i] += request[tid][i];
                need[tid][i] -= request[tid][i];
            }

            // Check for safe state
            int work[MAXR];
            int finish[MAXT];
            for (i = 0; i < r_count; i++) {
                work[i] = available[i];
            }
            for (i = 0; i < t_count; i++) {
                finish[i] = 0;
            }

            int done = 0;
            while (!done) {
                done = 1;
                for (i = 0; i < t_count; i++) {
                    if (!finish[i]) {
                        int can_finish = 1;
                        for (j = 0; j < r_count; j++) {
                            if (need[i][j] > work[j]) {
                                can_finish = 0;
                                break;
                            }
                        }
                        if (can_finish) {
                            for (j = 0; j < r_count; j++) {
                                work[j] += allocation[i][j];
                            }
                            finish[i] = 1;
                            done = 0;
                        }
                    }
                }
            }

            int safe = 1;
            for (i = 0; i < t_count; i++) {
                if (!finish[i]) {
                    safe = 0;
                    break;
                }
            }

            if (safe) {
                // Allocation is safe, commit allocation
                for (i = 0; i < r_count; i++) {
                    request[tid][i] = 0;
                }
                pthread_mutex_unlock(&mutex);
                return 0; // Success
            } else {
                // Not safe, roll back allocation
                for (i = 0; i < r_count; i++) {
                    available[i] += request[tid][i];
                    allocation[tid][i] -= request[tid][i];
                    need[tid][i] += request[tid][i];
                }
                // Wait
                pthread_cond_wait(&cond, &mutex);
            }
        } else if (can_allocate) {
            // Allocate resources
            for (i = 0; i < r_count; i++) {
                available[i] -= request[tid][i];
                allocation[tid][i] += request[tid][i];
            }
            // Reset request
            for (i = 0; i < r_count; i++) {
                request[tid][i] = 0;
            }
            pthread_mutex_unlock(&mutex);
            return 0; // Success
        } else {
            // Cannot allocate, wait
            pthread_cond_wait(&cond, &mutex);
        }
    }

    pthread_mutex_unlock(&mutex);

    return(ret);
}

// Thread releases resources
int reman_release (int release_param[])
{
    int ret = 0;
    int tid = -1;
    int i;

    if (!tid_key_created) {
        fprintf(stderr, "reman_release: Thread not connected\n");
        return -1;
    }

    tid = (int)(intptr_t)(pthread_getspecific(tid_key));
    if (tid < 0 || tid >= t_count) {
        fprintf(stderr, "reman_release: Thread not connected\n");
        return -1;
    }

    pthread_mutex_lock(&mutex);

    // Release the resources
    for (i = 0; i < r_count; i++) {
        if (release_param[i] < 0 || release_param[i] > allocation[tid][i]) {
            fprintf(stderr, "reman_release: Invalid release for resource %d\n", i);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        allocation[tid][i] -= release_param[i];
        available[i] += release_param[i];
        if (avoid == 1) {
            need[tid][i] += release_param[i];
        }
    }

    // Wake up waiting threads
    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mutex);

    return (ret);
}

// Detect deadlocks
int reman_detect()
{
    int ret = 0;
    int i, j;
    int work[MAXR];
    int finish[MAXT];
    int deadlocked_threads = 0;

    pthread_mutex_lock(&mutex);

    // Initialize work and finish
    for (i = 0; i < r_count; i++) {
        work[i] = available[i];
    }
    for (i = 0; i < t_count; i++) {
        if (!connected[i]) {
            finish[i] = 1;
        } else {
            finish[i] = 0;
        }
    }

    // Begin detection
    int done = 0;
    while (!done) {
        done = 1;
        for (i = 0; i < t_count; i++) {
            if (!finish[i]) {
                int can_finish = 1;
                for (j = 0; j < r_count; j++) {
                    if (request[i][j] > work[j]) {
                        can_finish = 0;
                        break;
                    }
                }
                if (can_finish) {
                    for (j = 0; j < r_count; j++) {
                        work[j] += allocation[i][j];
                    }
                    finish[i] = 1;
                    done = 0;
                }
            }
        }
    }

    // Now, find deadlocked threads
    for (i = 0; i < t_count; i++) {
        if (!finish[i]) {
            deadlocked_threads++;
        }
    }

    pthread_mutex_unlock(&mutex);

    ret = deadlocked_threads;

    return (ret);
}

// Print current state
void reman_print (char titlemsg[])
{
    int i, j;

    pthread_mutex_lock(&mutex);

    printf("##########################\n");
    printf("%s\n", titlemsg);
    printf("###########################\n");
    printf("Resource Count: %d\n", r_count);
    printf("Thread Count: %d\n", t_count);

    printf("Available (Free) Information:\n");
    for (i = 0; i < r_count; i++) {
        printf("R%d ", i);
    }
    printf("\n");
    for (i = 0; i < r_count; i++) {
        printf("%d  ", available[i]);
    }
    printf("\n");

    printf("Claim:\n");
    printf("    ");
    for (i = 0; i < r_count; i++) {
        printf("R%d ", i);
    }
    printf("\n");
    for (i = 0; i < t_count; i++) {
        printf("T%d: ", i);
        for (j = 0; j < r_count; j++) {
            printf("%d  ", claim[i][j]);
        }
        printf("\n");
    }

    printf("Allocation:\n");
    printf("    ");
    for (i = 0; i < r_count; i++) {
        printf("R%d ", i);
    }
    printf("\n");
    for (i = 0; i < t_count; i++) {
        printf("T%d: ", i);
        for (j = 0; j < r_count; j++) {
            printf("%d  ", allocation[i][j]);
        }
        printf("\n");
    }

    printf("Request:\n");
    printf("    ");
    for (i = 0; i < r_count; i++) {
        printf("R%d ", i);
    }
    printf("\n");
    for (i = 0; i < t_count; i++) {
        printf("T%d: ", i);
        for (j = 0; j < r_count; j++) {
            printf("%d  ", request[i][j]);
        }
        printf("\n");
    }

    printf("###########################\n");

    pthread_mutex_unlock(&mutex);
}
