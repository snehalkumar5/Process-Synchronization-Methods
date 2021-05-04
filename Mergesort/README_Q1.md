## ASSIGNMENT - 3
## SNEHAL KUMAR
## 2019101003
## Q1 - CONCURRENT MERGESORT
Given an array of n numbers, we try to sort the array using multiprocessing, multithreaded and normal mergesorts and compare their processing times.
#### Note: when size of array < 5, we use selection sort instead to speed up the process
###  Multi-processing: (multi_mergesort())
For multi processing, we recursively fork the child processes which in turn perform the merge sort. Since the parent and child processes are using the same array and making changes within it, we declare a shared memory to store the array so the forked process can use the shared memory for faster access. The child processes first sort the first half of the array and then the parent process recursively forks and sorts the second half of the array. Finally the parent process waits for the child processes to sort and finishes the process.

### Multi-threaded: (threaded_mergesort())
Using 'pthread_create' we create threads and pass a structure to them to maintain the array. The 'parent thread'(created in main) recursively creates threads to sort the two halves of the array simultaneously in . This enables concurrent mergesort and makes the sorting faster.

### Normal: (normal_mergesort())
We use the standard definition of mergesort using normal_mergesort() and merge().

### COMPARISON
We find that mergesort completes in order:
1. Normal
2.  Multi-threading
3. Multi-processing
a) The reasons for multi processing to be slowest is attributed to the fact that creating forks and child processes increases a lot of overhead. Each child process has its own control block and inherits the parent properties as well.
b) The reason for multi-threading to be slower than normal mergesort is due to creation of threads. Creating threads is has lesser overheads than creating a process (faster than multi-process) since it shares the memory block and cache and doesn't need to maintain memory addresses during context switching. But the numerous threads created still has more overhead than normal and causes unnecessary load on CPU.

n=10
```
Running multiprocess_concurrent_mergesort for n = 10 
time = 0.006742

Running multithreaded_concurrent_mergesort for n = 10
time = 0.001716

Running normal_mergesort for n = 10
time = 0.000033

normal_mergesort ran:
	[ 205.702639 ] times faster than multiprocess_concurrent_mergesort
	[ 52.343259 ] times faster than multithreaded_concurrent_mergesort


```
n=100:
```
Running multiprocess_concurrent_mergesort for n = 100
time = 0.032843

Running multithreaded_concurrent_mergesort for n = 100
time = 0.010890

Running normal_mergesort for n = 100
time = 0.000209

normal_mergesort ran:
	[ 156.947247 ] times faster than multiprocess_concurrent_mergesort
	[ 52.042857 ] times faster than multithreaded_concurrent_mergesort



```
n=1000:
```
Running multiprocess_concurrent_mergesort for n = 1000
time = 0.099159

Running multithreaded_concurrent_mergesort for n = 1000
time = 0.013595

Running normal_mergesort for n = 1000
time = 0.000163

normal_mergesort ran:
	[ 608.668612 ] times faster than multiprocess_concurrent_mergesort
	[ 83.451765 ] times faster than multithreaded_concurrent_mergesort



```