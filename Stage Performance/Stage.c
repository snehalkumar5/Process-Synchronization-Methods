#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#include <semaphore.h>
#include <errno.h>

//COLOURS 
#define CYAN "\e[1;36m" // 
#define RESET "\e[0m"
#define RED "\e[1;31m"
#define BLUE "\e[1;34m"
#define YELLOW "\e[1;33m"
#define PURPLE "\e[0;35m"   // SINGER FINISH
#define GREEN "\e[1;32m"    // ARRIVED
#define WHITE "\e[1;37m"
#define BOLDPURPLE "\e[1;35m"


//STATES

#define IS_ACOUSTIC 0
#define IS_ELECTRIC 1
#define STAGE_EMPTY 0
#define STAGE_SOLO_SING 1
#define STAGE_SOLO_MUS 2
#define STAGE_DUO 3

#define IS_MUSICIAN 0
#define MUSICIAN_NOT_YET_ARRIVED 0
#define MUSICIAN_WAITING_TO_PERFORM 1
#define MUSICIAN_PERFORMING_SOLO 2
#define MUSICIAN_PERFORMING_WITH_SINGER 3
#define MUSICIAN_WAITING_FOR_SHIRT 4
#define MUSICIAN_COLLECTING_SHIRT 5
#define MUSICIAN_EXITED 6

#define IS_SINGER 1
#define SINGER_NOT_YET_ARRIVED 0
#define SINGER_WAITING_TO_PERFORM 1
#define SINGER_PERFORMING_SOLO 2
#define SINGER_PERFORMING_WITH_MUSICIAN 3
#define SINGER_WAITING_FOR_SHIRT 4
#define SINGER_COLLECTING_SHIRT 5
#define SINGER_EXITED 6

#define COORDINATOR_FREE 0
#define COORDINATOR_BUSY 1

// STRUCTURES

typedef struct Stage
{
    int id;
    pthread_t tid;

    int state;
    char *type;
    struct Musician* mymusician_a;
    struct Musician* mysinger_b;

    struct Musician** musicians;

} Stage;

typedef struct Musician {
    int id;
    pthread_t tid;

    int type;
    int perform_time,wait_time,arrival_time;
    int state;
    char instrument;
    char *name;
    int a_stage,e_stage;

    Stage* mystage;

    pthread_mutex_t lock;       
    pthread_cond_t condlock;  
    sem_t performed;           
    sem_t collected;          
    sem_t dual;          

} Musician;

typedef struct Coordinator {
    int id;
    pthread_t tid;

    int state;

    struct Musician* mymusician;
    struct Musician** musicians;
} Coordinator;

int num_a_stages,num_e_stages,num_musicians,num_coord,T1,T2,T;

sem_t gocollect;

void ck_eligible(Musician* mu,char c)
{
    mu->type=IS_MUSICIAN;
    switch (c)
    {
    case 'p': 
        mu->a_stage=1;
        mu->e_stage=1;
        break;
    case 'g': 
        mu->a_stage=1;    
        mu->e_stage=1;
    case 'v': 
        mu->a_stage=1;
        mu->e_stage=0;
        break;
    case 'b': 
        mu->a_stage=0;
        mu->e_stage=1;
        break;
    case 's': 
        mu->type=IS_SINGER;
        mu->a_stage=1;
        mu->e_stage=1;
        break;
    default: printf("Invalid instrument character!\n");
        break;
    }
    return;
}
void init_musician(Musician* r, int i, char *name, char c, int time) 
{
    r->id = i;
    r->instrument=c;
    ck_eligible(r,c);
    r->name = (char*) malloc(sizeof(char)*100);
    strcpy(r->name,name);
    r->arrival_time = time;
    r->perform_time = rand()%(T2-T1+1)+T1;   // perform time [t1,t2]
    r->wait_time = T;
    r->state = MUSICIAN_NOT_YET_ARRIVED;    // same value for singer
    r->mystage = NULL;
    sem_init(&(r->performed), 0, 0);
    sem_init(&(r->collected), 0, 0);
    sem_init(&(r->dual), 0, 0);
    pthread_mutex_init(&(r->lock), NULL);
    pthread_cond_init(&(r->condlock), NULL);
}

//------------------------------------------------
//              STAGE
//------------------------------------------------
void* stge_exec(void *input)
{
    Stage* s = (Stage*)input;
    while(1)
    {
        if(s->state == STAGE_EMPTY)
        {
            for(int i=0;i<num_musicians;i++)
            {
                Musician* mus = s->musicians[i];
                pthread_mutex_lock(&(mus->lock));
                // printf("%s :%d,%d :: %s\n",mus->name,mus->a_stage,mus->e_stage,s->type);

                // not eligible
                if((!mus->a_stage && !strcmp(s->type,"acoustic")) || (!mus->e_stage && !strcmp(s->type,"electric")))
                {
                    pthread_mutex_unlock(&(mus->lock));
                    continue;
                }
                // performer not ready
                if(mus->state != MUSICIAN_WAITING_TO_PERFORM)
                {
                    pthread_cond_signal(&(mus->condlock));
                    pthread_mutex_unlock(&(mus->lock));
                    continue;
                }
                // found musician
                if(mus->type == IS_MUSICIAN)
                {
                    // pthread_mutex_lock(&(mus->lock));
                    pthread_cond_signal(&(mus->condlock));
                    mus->mystage = s;
                    mus->state = MUSICIAN_PERFORMING_SOLO;
                    pthread_mutex_unlock(&(mus->lock));
                    s->state = STAGE_SOLO_MUS;
                    s->mymusician_a = mus;
                    printf(BOLDPURPLE"Musician %s performing %c on %s stage %d for %d secs\n"RESET, mus->name, mus->instrument, mus->mystage->type, mus->mystage->id+1, mus->perform_time);
                    break;

                }
                // found singer
                else if(mus->type == IS_SINGER)
                {
                    // pthread_mutex_lock(&(mus->lock));
                    pthread_cond_signal(&(mus->condlock));
                    mus->state = SINGER_PERFORMING_SOLO;
                    mus->mystage = s;
                    pthread_mutex_unlock(&(mus->lock));
                    s->state = STAGE_SOLO_SING;
                    s->mysinger_b = mus;
                    s->mymusician_a = NULL;
                    printf(BOLDPURPLE"Singer %s performing on %s stage %d for %d secs\n"RESET, mus->name, mus->mystage->type, mus->mystage->id+1, mus->perform_time);
                    sem_wait(&(mus->performed));
                    s->state = STAGE_EMPTY;
                    break;
                }
            }
        }
        else if(s->state == STAGE_SOLO_MUS)
        {
            // check if musician has finished performance
            if(sem_trywait(&(s->mymusician_a->performed))==0)
            {
                s->mymusician_a = NULL;
                s->state = STAGE_EMPTY;
                    continue;
            }
            for(int i=0;i<num_musicians;i++)
            {
                Musician* mus = s->musicians[i];
                pthread_mutex_lock(&(mus->lock)); 
                if(mus->type == IS_MUSICIAN || mus->state != MUSICIAN_WAITING_TO_PERFORM)
                {
                    pthread_cond_signal(&(s->musicians[i]->condlock));
                    pthread_mutex_unlock(&(mus->lock));
                    continue;
                }
                else if(mus->type == IS_SINGER)
                {
                    pthread_cond_signal(&(mus->condlock));
                    mus->mystage = s;
                    s->state = STAGE_DUO;
                    s->mysinger_b = mus;
                    mus->state = SINGER_PERFORMING_WITH_MUSICIAN;
                    pthread_mutex_unlock(&(mus->lock));

                    pthread_mutex_lock(&(s->mymusician_a->lock));
                    s->mymusician_a->state = MUSICIAN_PERFORMING_WITH_SINGER;
                    pthread_mutex_unlock(&(s->mymusician_a->lock));
                    
                    printf(BOLDPURPLE"Singer %s joined musician %s on %s stage %d. Performance time extended by 2 secs\n"RESET, mus->name, mus->mystage->mymusician_a->name, mus->mystage->type, mus->mystage->id+1);

                    // sem_post(&(s->mymusician_a->dual));
                    // waits for musician to finish
                    sem_wait(&(s->mymusician_a->performed));

                    s->mymusician_a=NULL;
                    sem_wait(&(mus->performed));
                    s->mysinger_b=NULL;
                    s->state = STAGE_EMPTY;
                    continue;
                }
            }
        }
    }
    return NULL;
}

//------------------------------------------------
//              MUSICIAN
//------------------------------------------------
void* music_exec(void *input)
{
    Musician* m = (Musician*)input;
    pthread_mutex_lock(&(m->lock));
    sleep(m->arrival_time);
    if(m->type==IS_SINGER)
    {
        struct timespec settime;
        clock_gettime(CLOCK_REALTIME, &settime);
        settime.tv_sec=m->wait_time+settime.tv_sec;
        printf(GREEN"Singer %s has arrived at Srujana\n"RESET,m->name);
        sleep(1);
        m->state=SINGER_WAITING_TO_PERFORM;

        while(m->state==MUSICIAN_WAITING_TO_PERFORM)
        {
            if(pthread_cond_timedwait(&(m->condlock), &(m->lock), &settime) == ETIMEDOUT)
            {
                m->state = MUSICIAN_EXITED;
                sem_post(&(m->performed));
                printf(RED"Singer %s left due to impatience\n"RESET, m->name);
                pthread_mutex_unlock(&(m->lock));
                return NULL;
            }
        }
    }
    else
    {
        printf(GREEN"Musician %s has arrived with instrument %c at Srujana\n"RESET,m->name,m->instrument);
        sleep(1);
        m->state=MUSICIAN_WAITING_TO_PERFORM;
        struct timespec settime;
        clock_gettime(CLOCK_REALTIME, &settime);
        settime.tv_sec=m->wait_time+settime.tv_sec;
        while(m->state==MUSICIAN_WAITING_TO_PERFORM)
        {
            if(pthread_cond_timedwait(&(m->condlock), &(m->lock), &settime) == ETIMEDOUT)
            {
                m->state = MUSICIAN_EXITED;
                sem_post(&(m->performed));
                printf(RED"Musician %s with instrument %c left due to impatience\n"RESET, m->name,m->instrument);
                pthread_mutex_unlock(&(m->lock));
                return NULL;
            }
        }
    }
    
    pthread_mutex_unlock(&(m->lock));
   
    if(m->type == IS_SINGER)
    {
        if(m->state == SINGER_PERFORMING_WITH_MUSICIAN)
        {
            sem_wait(&(m->dual));
            printf(YELLOW"Singer %s performance at %s stage %d ended\n"RESET,m->name,m->mystage->type,m->mystage->id+1);
            sem_post(&(m->performed));
        }
        else
        {
            sleep(m->perform_time);
            printf(YELLOW"Singer %s performance at %s stage %d ended\n"RESET,m->name,m->mystage->type,m->mystage->id+1);
            sem_post(&(m->performed));
        }
    }
    else
    {
        sleep(m->perform_time);
        if(m->state == MUSICIAN_PERFORMING_WITH_SINGER)
        {
            // sem_wait(&(m->dual));
            sleep(2);
            sem_post(&(m->mystage->mysinger_b->dual));
            printf(YELLOW"Musician %s performance at %s stage %d ended\n"RESET,m->name,m->mystage->type,m->mystage->id+1);
            sem_post(&(m->performed));
        }
        else
        {
            printf(YELLOW"Musician %s performance at %s stage %d ended\n"RESET,m->name,m->mystage->type,m->mystage->id+1);
            sem_post(&(m->performed));
        }
    }
    pthread_mutex_lock(&(m->lock));
    m->state = MUSICIAN_WAITING_FOR_SHIRT;
    m->mystage = NULL;
    pthread_mutex_unlock(&(m->lock));
    sleep(1);
    if(num_coord==0)
    {
        m->state = MUSICIAN_EXITED;
        return NULL;
    }
    if(m->type==IS_SINGER)
    {
        printf(CYAN "Singer %s waiting to collect t-shirt\n" RESET, m->name);
    }
    else
    {
        printf(CYAN "Musician %s waiting to collect t-shirt\n" RESET, m->name);
    }
    //signal coordinator to collect tshirt
    sem_post(&gocollect);
    sem_wait(&(m->collected));
    // collected tshirt and now will exit
    pthread_mutex_lock((&m->lock));
    m->state = MUSICIAN_EXITED;
    pthread_mutex_unlock((&m->lock));
    return NULL;
}
//------------------------------------------------
//              COORDINATOR
//------------------------------------------------
void* coord_exec(void *input)
{
    Coordinator* coord = (Coordinator*)input;
    while(1)
    {
        sem_wait(&gocollect);
        coord->state = COORDINATOR_BUSY;
        for(int i=0;i<num_musicians;i++)
        {
            Musician* mus = coord->musicians[i];
            pthread_mutex_lock(&(mus->lock));
            if(mus->state == MUSICIAN_WAITING_FOR_SHIRT)
            {
                mus->state = MUSICIAN_COLLECTING_SHIRT;
                coord->mymusician = mus;
                // sleep(1); // gotta wait for dem tshirt
                pthread_mutex_unlock(&(mus->lock));
                break;
            }
            pthread_mutex_unlock(&(mus->lock));
        }
        if(coord->mymusician==NULL)
        continue;
        if(coord->mymusician->type == IS_MUSICIAN)
        printf(BLUE "Musician %s collecting tshirt from coordinator %d\n" RESET, coord->mymusician->name, coord->id+1);
        else
        printf(BLUE "Singer %s collecting tshirt from coordinator %d\n" RESET, coord->mymusician->name, coord->id+1);

        sleep(2);
        pthread_mutex_lock(&(coord->mymusician->lock));
        sem_post(&(coord->mymusician->collected));
        pthread_mutex_unlock(&(coord->mymusician->lock));

        if(coord->mymusician->type == IS_MUSICIAN)
        printf(PURPLE "Musician %s collected tshirt and exited\n" RESET, coord->mymusician->name);
        else
        printf(PURPLE "Singer %s collected tshirt and exited\n" RESET, coord->mymusician->name);
        
        coord->state = COORDINATOR_FREE;
        coord->mymusician = NULL;
    }
    return NULL;
}

int main()
{
    srand(time(0));
    sem_init(&gocollect, 0, 0);

    printf("Enter musicians, acoustic stages, electric stages, coordinators, t1, t2, t:\n");
    scanf("%d %d %d %d %d %d %d",&num_musicians,&num_a_stages,&num_e_stages,&num_coord,&T1,&T2,&T);
    Musician** musicians = (Musician**) malloc(num_musicians*sizeof(Musician*));
    Stage** a_stages = (Stage**) malloc(num_a_stages* sizeof(Stage*));
    Stage** e_stages = (Stage** )malloc(num_e_stages * sizeof(Stage*));
    Coordinator** coords = (Coordinator**) malloc(num_coord*sizeof(Coordinator*));
    if(num_musicians == 0)
    {
        printf("Simulation cannot run. Insufficient number of musicians.\n");
        return 0;
    }
    for(int i=0;i<num_musicians;i++)
    {
        musicians[i] = (Musician*)malloc(sizeof(Musician));
        char *name = (char*)malloc(sizeof(char)*100);
        char c;
        int arr_time;
        printf("Enter musician/singer name, instrument, time of arrival:\n");
        scanf("%s %c %d",name,&c,&arr_time);
        init_musician(musicians[i],i,name,c,arr_time);
        // pthread_create(&(musicians[i]->tid),NULL,music_exec,(void*)musicians[i]);
        free(name);
    }
    for(int i=0;i<num_a_stages;i++)
    {
        a_stages[i]=(Stage*) malloc(sizeof(Stage));
        a_stages[i]->id = i;
        a_stages[i]->type = (char*)malloc(sizeof(char)*100);
        strcpy(a_stages[i]->type,"acoustic");
        a_stages[i]->state = STAGE_EMPTY;
        a_stages[i]->mymusician_a = NULL;
        a_stages[i]->mysinger_b = NULL;
        a_stages[i]->musicians = musicians;
        // pthread_create(&(a_stages[i]->tid),NULL,stge_exec,(void*)a_stages[i]);
    }
    for(int i=0;i<num_e_stages;i++)
    {
        e_stages[i]=(Stage*) malloc(sizeof(Stage));
        e_stages[i]->id = i;
        e_stages[i]->type = (char*)malloc(sizeof(char)*100);
        strcpy(e_stages[i]->type,"electric");
        e_stages[i]->state = STAGE_EMPTY;
        e_stages[i]->mymusician_a = NULL;
        e_stages[i]->mysinger_b = NULL;
        e_stages[i]->musicians = musicians;
        // pthread_create(&(e_stages[i]->tid),NULL,stge_exec,(void*)e_stages[i]);
    }
    for(int i=0;i<num_coord;i++)
    {
        coords[i]=(Coordinator*) malloc(sizeof(Coordinator));
        coords[i]->id=i;
        coords[i]->state=COORDINATOR_FREE;
        coords[i]->mymusician=NULL;
        coords[i]->musicians=musicians;
        // pthread_create(&(coords[i]->tid),NULL,coord_exec,(void*)coords[i]);
    }
    for(int i=0;i<num_musicians;i++)
    {
        pthread_create(&(musicians[i]->tid),NULL,music_exec,(void*)musicians[i]);
    }
    for(int i=0;i<num_a_stages;i++)
    {
        pthread_create(&(a_stages[i]->tid),NULL,stge_exec,(void*)a_stages[i]);
    }
    for(int i=0;i<num_e_stages;i++)
    {
        pthread_create(&(e_stages[i]->tid),NULL,stge_exec,(void*)e_stages[i]);
    }
    for(int i=0;i<num_coord;i++)
    {
        pthread_create(&(coords[i]->tid),NULL,coord_exec,(void*)coords[i]);
    }
    for(int i=0;i<num_musicians;i++)
    {
        pthread_join(musicians[i]->tid,NULL);
    }
    printf("Simulation Over\n");
    for(int i=0;i<num_musicians;i++)
    {
        pthread_mutex_destroy(&(musicians[i]->lock));
    }
    return 0;
}