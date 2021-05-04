#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

void swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int * shareMem(size_t size){
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}

void merge(int *arr, int low, int mid, int high)
{
    int n = high-low+1;
    int *full = (int*) malloc(sizeof(int)*n+1);
    int cnt=0,x=low,y=mid+1;
    for(int i=low;i<high+1;i++)
    {
        if(x>mid)
        {
            full[cnt++]=arr[y++];
        }
        else if(y>high)
        {
            full[cnt++]=arr[x++];
        }
        else if(arr[x]<arr[y])
        {
            full[cnt++]=arr[x++];
        }
        else
        {
            full[cnt++]=arr[y++];
        }
    }
    for(int i=0;i<cnt;i++)
    {
        arr[low++]=full[i];
    }
}
void selectionsort(int *arr, int low, int high)
{
    int min=0;
    for(int i=low;i<=high;i++)
    {
        min=i;
        for(int j=i+1;j<=high;j++)
        {
            if(arr[j]<arr[min])
            {
                min=j;
            }
        }
        swap(&arr[i],&arr[min]);
    }
}
void normal_mergesort(int *arr, int low, int high)
{
    int n=high-low+1;
    if(n<5)
    {
        selectionsort(arr,low,high);
    }
    else if(low<high)
    {
        int mid=low+(high-low)/2;
        normal_mergesort(arr,low,mid);
        normal_mergesort(arr,mid+1,high);
        merge(arr,low,mid,high);
    }
}
void multi_mergesort(int *arr, int low, int high)
{
    int selection=0,n=high-low+1;
    // printf("n:%d",n);
    if(n<5)
    {
        selectionsort(arr,low,high);
        return;
    }

    if(low<high)
    {
        int mid=low+(high-low)/2;
        int pid1=fork();
        int pid2;
        if(pid1==0)
        {
            multi_mergesort(arr,low,mid);
            _exit(1);
        }
        else
        {
            pid2=fork();
            if(pid2==0)
            {
                multi_mergesort(arr,mid+1,high);
                _exit(1);
            }
            else
            {
                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
                merge(arr,low,mid,high);
            }
        }
        return;
    }
}

struct arg{
    int l;
    int r;
    int* arr;
};

void *threaded_mergesort(void* a){
    //note that we are passing a struct to the threads for simplicity.
    struct arg *args = (struct arg*) a;

    int l = args->l;
    int r = args->r;
    int *arr = args->arr;
    if(l>=r) return NULL;
    if(r-l+1<5)
    {
        selectionsort(arr,l,r);
        return NULL;
    }
    int mid=l+(r-l)/2;
    //sort left half array
    struct arg a1;
    a1.l = l;
    a1.r = mid;
    a1.arr = arr;
    pthread_t tid1;
    pthread_create(&tid1, NULL, threaded_mergesort, &a1);

    //sort right half array
    struct arg a2;
    a2.l = mid+1;
    a2.r = r;
    a2.arr = arr;
    pthread_t tid2;
    pthread_create(&tid2, NULL, threaded_mergesort, &a2);

    //wait for the two halves to get sorted
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    merge(arr,l,mid,r);
}

void runSorts(long long int n){

    struct timespec ts;

    //getting shared memory
    int *arr = shareMem(sizeof(int)*(n+1));
    for(int i=0;i<n;i++) scanf("%d", arr+i);

    int *brr = (int*) malloc(sizeof(int)*n+1);
    for(int i=0;i<n;i++) 
    {
        brr[i] = arr[i];
    }

     for(int i=0; i<n; i++){
         printf("%d ",arr[i]);
     }
    printf("\nRunning multiprocess_concurrent_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;
    //multiprocess mergesort
    multi_mergesort(arr,0,n-1);
    for(int i=0; i<n; i++){
        printf("%d ",arr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t1 = en-st;

    pthread_t tid;
    struct arg a;
    a.l = 0;
    a.r = n-1;
    a.arr = (int*)malloc(sizeof(int)*n+1);
    for(int i=0;i<n;i++)
    {
        a.arr[i]=brr[i];
    }
    
     for(int i=0; i<n; i++){
         printf("%d ",a.arr[i]);
     }
    printf("\nRunning multithreaded_concurrent_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    //multithreaded mergesort
    pthread_create(&tid, NULL, threaded_mergesort, &a);
    pthread_join(tid, NULL);
    for(int i=0; i<n; i++){
       printf("%d ",a.arr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t2 = en-st;
    for(int i=0; i<n; i++){
         printf("%d ",brr[i]);
     }
    printf("\nRunning normal_mergesort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    // normal mergesort
    normal_mergesort(brr, 0, n-1);

  for(int i=0; i<n; i++){
        printf("%d ",brr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t3 = en - st;

    printf("normal_mergesort ran:\n\t[ %Lf ] times faster than multiprocess_concurrent_mergesort\n\t[ %Lf ] times faster than multithreaded_concurrent_mergesort\n\n\n", t1/t3, t2/t3);
    shmdt(arr);
    return;
}

int main(){

    long long int n;
    scanf("%lld", &n);
    runSorts(n);
    return 0;
}
