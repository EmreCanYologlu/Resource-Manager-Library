# Resource Manager Library (libreman)

## Overview

This project involves the development of a thread-safe **Resource Manager Library** (`libreman`) in C/Linux. The library enables multithreaded applications to manage resources safely and efficiently, with built-in mechanisms for deadlock detection and avoidance.

A multithreaded program linked with this library can:
- Request and release resources.
- Handle deadlocks (detect or avoid them).
- Manage resource allocation with a resource allocation graph.

The library uses **POSIX threads**, mutex locks, and condition variables to ensure thread safety and synchronization.

---

## Features

1. **Resource Management:**
   - Support for up to 1000 resource types.
   - Resource requests, allocation, and release by threads.

2. **Deadlock Handling:**
   - Detection of deadlocks.
   - Avoidance of deadlocks using Banker's Algorithm.

3. **Thread Safety:**
   - Proper synchronization using mutex locks and condition variables.

4. **State Monitoring:**
   - Prints the current resource allocation state using `reman_print()`.

---

## Library API

### Header File
The library provides the following interface through `reman.h`:

```c
#ifndef REMAN_H
#define REMAN_H

#define MAXR 1000 // Maximum resource types supported
#define MAXT 100  // Maximum threads supported

int reman_init(int t_count, int r_count, int avoid);
int reman_connect(int tid);
int reman_disconnect();
int reman_claim(int claim[]);
int reman_request(int request[]);
int reman_release(int release[]);
int reman_detect();
void reman_print(char titlemsg[]);

#endif /* REMAN_H */
```

### Function Descriptions

1. **`reman_init(int t_count, int r_count, int avoid)`**
   - Initializes the library.
   - `t_count`: Number of threads.
   - `r_count`: Number of resource types.
   - `avoid`: 1 for deadlock avoidance, 0 otherwise.

2. **`reman_connect(int tid)`**
   - Called by a thread upon creation to register with the library.
   - `tid`: Unique thread identifier (assigned by the application).

3. **`reman_disconnect()`**
   - Called by a thread before termination or when it no longer needs resources.

4. **`reman_claim(int claim[])`**
   - Used for deadlock avoidance to declare the maximum resources a thread might request.

5. **`reman_request(int request[])`**
   - Requests resources for the calling thread.
   - Blocks the thread if resources are unavailable or unsafe to allocate.

6. **`reman_release(int release[])`**
   - Releases resources held by the calling thread.
   - Wakes up blocked threads to check for resource availability.

7. **`reman_detect()`**
   - Checks for deadlocks.
   - Returns the number of deadlocked threads or 0 if none.

8. **`reman_print(char titlemsg[])`**
   - Prints the current state of the resource allocation graph and resource availability.

---

## Sample Application

A sample application `myapp.c` demonstrates the use of `libreman`. The application creates threads, requests and releases resources, and showcases deadlock detection and avoidance.

### Compilation and Execution

1. **Compile the Library:**
   ```bash
   gcc -Wall -c reman.c
   ar -cvq libreman.a reman.o
   ranlib libreman.a
   ```

2. **Compile the Application:**
   ```bash
   gcc -Wall -o myapp myapp.c -L. -lreman -lpthread
   ```

3. **Run the Application:**
   ```bash
   ./myapp [avoid_flag]
   ```
   - `avoid_flag`: 1 for deadlock avoidance, 0 for detection only.

---

## Resource Allocation Example

### Sample Output from `reman_print()`:

```plaintext
###########################
The current state
###########################
Resource Count: 2
Thread Count: 3

Available (Free) Information:
     R0 R1
     0  0

Claim:
     R0 R1
T0:  1  1
T1:  1  1
T2:  1  1

Allocation:
     R0 R1
T0:  0  0
T1:  1  0
T2:  0  1

Request:
     R0 R1
T0:  0  0
T1:  0  1
T2:  1  0
###########################
```

---

## Performance Experiments

- **Objective:** Evaluate the performance of the library under various workloads.
- **Scenarios:**
  - Deadlock detection with multiple threads and resources.
  - Deadlock avoidance under high contention.
- **Metrics:** Time taken for resource allocation, detection, and avoidance.
- **Report:** Results and interpretations are included in `report.pdf`.

---

## Submission Structure

- **Files:**
  - `reman.c`: Library implementation.
  - `reman.h`: Library interface.
  - `myapp.c`: Sample application.
  - `Makefile`: Compilation instructions.
  - `report.pdf`: Experimental results and analysis.

- **Directory Structure:**
  ```plaintext
  [StudentID]/
    |-- README.txt
    |-- reman.c
    |-- reman.h
    |-- myapp.c
    |-- Makefile
    |-- report.pdf
  ```

- **Compilation:** Ensure that the programs compile using:
  ```bash
  make
  ```

---

## Notes

- Test thoroughly on Ubuntu Linux (24.04 or later).
- Maximum supported threads: 100.
- Maximum resource types: 1000.
- Ensure no race conditions or deadlocks in the library implementation.

---

## References

1. [POSIX Threads](https://man7.org/linux/man-pages/man7/pthreads.7.html)
2. Course Textbook

---

## Author
- Names: Emre Can Yologlu
- Course: CS342 - Operating Systems, Bilkent University

