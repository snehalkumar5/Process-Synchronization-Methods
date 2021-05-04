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

#define CYAN "\e[1;36m"
#define RESET "\e[0m"
#define RED "\e[1;31m"
#define BLUE "\e[1;34m"
#define YELLOW "\e[1;33m"
#define PURPLE "\e[1;35m"
#define GREEN "\e[1;32m"

pthread_mutex_t global_lock;    // for protecting number of waiting students

int num_comp,num_stud,num_zones,wait_students;
typedef struct Company {
    int id;
    pthread_t tid;
    int prep_time;
    int total_batch;
    int batch_used,batch_left;
    int batch_capacity;
    long double vaccine_prob;
    pthread_mutex_t lock;
}Company;

typedef struct Zone {
    int id;
    pthread_t tid;
    int state;
    int num_vaccine,left_vaccine;
    int totalslots,leftslots,slots_used;
    struct Company** company;
    Company* mycomp;
    pthread_mutex_t lock;
} Zone;

typedef struct Student {
    int id;
    pthread_t tid;
    int time;
    int check;              // check for successful vaccination
    int num;                // number of times vaccinated
    struct Zone* myzone;
    struct Zone** zone;
    pthread_mutex_t lock;
} Student;

void init_comp(Company* a, int i, long double x);
void init_stud(Student* a, int i, Zone** zone);
void init_zone(Zone* a,int i,Company** comp);
void* comp_exec(void *inp);
void* stud_exec(void *inp);
void* zone_exec(void *inp);
void run_stud(Student* a);
int main()
{
    srand(time(0));
   
    pthread_mutex_init(&global_lock, NULL);
    printf("Enter Company, Zone, Students:\n");
    scanf("%d %d %d",&num_comp,&num_zones,&num_stud);
    if(num_comp==0 || num_zones==0)
    {
        printf("Simluation cannot run: Insufficient values.\n");
        return 0;
    }
    Company** companies = (Company**)malloc(num_comp*sizeof(Company*));
    Zone** zones = (Zone**)malloc(num_zones*sizeof(Zone*));
    Student** students = (Student**)malloc(num_stud*sizeof(Student*));
    printf("Enter the probabilities of the company vaccines:\n");
    for(int i=0;i<num_comp;i++)
    {
        long double x;
        scanf("%Lf",&x);
        companies[i] = (Company*)malloc(sizeof(Company));
        init_comp(companies[i],i,x);
        pthread_create(&(companies[i]->tid),NULL,comp_exec,(void *)(companies[i]));
        
    }
    for(int i=0;i<num_zones;i++)
    {
        zones[i] = (Zone*)malloc(sizeof(Zone));
        init_zone(zones[i],i,companies);
        pthread_create(&(zones[i]->tid),NULL,zone_exec,(void *)(zones[i]));
    }
     for(int i=0;i<num_stud;i++)
    {
        students[i] = (Student*)malloc(sizeof(Student));
        init_stud(students[i],i,zones);
        pthread_create(&(students[i]->tid),NULL,stud_exec,(void *)(students[i]));
    }

    for(int i=0;i<num_stud;i++)
    {
        pthread_join(students[i]->tid, NULL);
    }
    for(int i=0;i<num_zones;i++)
    {
        pthread_mutex_destroy(&(zones[i]->lock));
    }
    for(int i=0;i<num_comp;i++)
    {
        pthread_mutex_destroy(&(companies[i]->lock));
    }
    printf("Simulation over.\n");
    return 0;
}
// ------------------------------------------------------------------------
//                              COMPANY
// ------------------------------------------------------------------------
void init_comp(Company* a, int i,long double x)
{
    a->id=i;
    a->vaccine_prob=x;
    a->total_batch=rand()%5+1;              // num of batch between [1,5]
    a->prep_time=rand()%4+2;                // prep time between [2,5]
    a->batch_capacity=rand()%11+10;         // batch capacity between [10,20]
    pthread_mutex_init(&(a->lock), NULL);
}

void* comp_exec(void *inp)
{
    Company* c = (Company*) inp;
    
    while(1)
    {
        printf(CYAN "Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %Lf\n"RESET,c->id+1,c->total_batch,c->vaccine_prob);
        sleep(c->prep_time);
        printf(CYAN"Pharmaceutical Company %d has prepared %d batches of %d vaccines which have success probability %Lf. Waiting for all the vaccines to be used to resume production\n"RESET,c->id+1,c->total_batch,c->batch_capacity,c->vaccine_prob);
        c->batch_left=c->total_batch;
        c->batch_used=0;
        
        //busy wait till all batches of company are used
        while(c->total_batch>c->batch_used);
        c->batch_left=0;
        printf(CYAN"All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n"RESET,c->id+1);
    }
    return NULL;
}

// ------------------------------------------------------------------------
//                              ZONE
// ------------------------------------------------------------------------
void init_zone(Zone* a,int i,Company** comp)
{
    a->id=i;
    a->totalslots=0;
    a->leftslots=0;
    a->company=comp;
    pthread_mutex_init(&(a->lock), NULL);
}
void* zone_exec(void *inp)
{
    Zone* z = (Zone*)inp;
    while(1)
    {
        z->state=0;
        // GET BATCH FROM COMPANY
        while(1)        
        {
            int i=rand()%num_comp;                   
            Company* comp = z->company[i];
            pthread_mutex_lock(&(comp->lock));
            z->mycomp=comp;
            // printf("batches:%d,capacity:%d",comp->batch_left,comp->batch_capacity);
            if(comp->batch_left>0)
            {
                printf(RED"Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %Lf\n"RESET,comp->id+1,z->id+1,comp->vaccine_prob);
                comp->batch_left--;
                z->num_vaccine=comp->batch_capacity;
                z->left_vaccine=z->num_vaccine;
                pthread_mutex_unlock(&(comp->lock));
                break;
            }
            pthread_mutex_unlock(&(comp->lock));
        }

        printf(RED"Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n"RESET,z->mycomp->id+1,z->id+1);
        // VACCINATION PHASE
        sleep(2);
        printf(RED"Vaccination Zone %d entering Vaccination Phase\n"RESET,z->id+1);
        sleep(1);
        while(z->left_vaccine>0)
        {
            pthread_mutex_lock(&(z->lock));
            z->totalslots=8>z->left_vaccine?z->left_vaccine:8;
            z->totalslots=z->totalslots>wait_students?wait_students:z->totalslots;
            if(z->totalslots==0)
            {
                pthread_mutex_unlock(&(z->lock));
                continue;
            }
            z->totalslots = rand()%z->totalslots+1;
            z->leftslots = z->totalslots;
            z->left_vaccine = z->left_vaccine-z->totalslots;
            // z->mycomp->batch_capacity = z->mycomp->batch_capacity-z->totalslots; // remaining vaccines of batch
            pthread_mutex_unlock(&(z->lock));

            printf(RED"Vaccination Zone %d is ready to vaccinate with %d slots.\n"RESET,z->id+1,z->totalslots);
            // start vaccination phase
   
            z->state=1;
            z->slots_used=0;
            
            // busy wait till slots assigned have been used
            while(z->slots_used<z->totalslots);
            z->state=0;
            sleep(1);
        }
        z->left_vaccine=0;
        z->mycomp->batch_used++;
        printf(RED"Vaccination Zone %d has run out of vaccines\n"RESET,z->id+1);
    }
    return NULL;
}
// ------------------------------------------------------------------------
//                              STUDENTS
// ------------------------------------------------------------------------
void init_stud(Student* a, int i,Zone** zones)
{
    a->id=i;
    a->num=1;
    a->check=-1;
    a->zone=zones;
    pthread_mutex_init(&(a->lock),NULL);
}
int cksuccess(Zone* a)
{
    long double x = a->mycomp->vaccine_prob;
    long double ck = (long double) rand()/RAND_MAX;
    if(x-ck>0.00001)
    return 1;
    return 0;
}
void* stud_exec(void *inp)
{
    Student* st = (Student*) inp;
    run_stud(st);
    return NULL;
}

void run_stud(Student* a)
{
    pthread_mutex_lock(&(a->lock));
    a->time=rand()%10+1; //arrival time
    sleep(a->time);
    pthread_mutex_unlock(&(a->lock));

    pthread_mutex_lock(&global_lock);
    wait_students++;
    pthread_mutex_unlock(&global_lock);
    
    while(a->num<4 && a->check!=1)
    {
        char roundno[100];
        if(a->num==1)
        sprintf(roundno,"%dst",a->num);
        else if(a->num==2)
        sprintf(roundno,"%dnd",a->num);
        else
        sprintf(roundno,"%drd",a->num);
        printf(YELLOW"Student %d has arrived for his %s round of Vaccination\n"RESET,a->id+1,roundno);
        sleep(1);
        printf(YELLOW"Student %d is waiting to be allocated a slot on a Vaccination Zone\n"RESET,a->id+1);
        

        while(1) //allot students to zone
        {
            int i=rand()%num_zones;
            Zone* zone = a->zone[i];
            pthread_mutex_lock(&(zone->lock));
            a->myzone=zone;
            if(zone->state!=1 || zone->leftslots<=0)    // in phase or no vacancy
            {
                pthread_mutex_unlock(&(zone->lock));
                continue;
            }
          
            zone->leftslots--;
            printf(GREEN"Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n"RESET,a->id+1,zone->id+1);
        
            pthread_mutex_unlock(&(zone->lock));
            break;
        }
        sleep(2);
        // student in vaccination phase
        printf(GREEN"Student %d on Vaccination Zone %d has been vaccinated which has success probability %Lf\n"RESET,a->id+1,a->myzone->id+1,a->myzone->mycomp->vaccine_prob);
        if(a->check!=1)
        {
            pthread_mutex_lock(&(a->myzone->lock));
            a->myzone->slots_used++;
            pthread_mutex_unlock(&(a->myzone->lock));

            if(cksuccess(a->myzone))
            {
                printf(PURPLE"Student %d has tested positive for antibodies\n"RESET,a->id+1);
                a->check=1;
                pthread_mutex_lock(&global_lock);
                wait_students--;
                pthread_mutex_unlock(&global_lock);
                break;
            }
            else
            {
                printf(PURPLE"Student %d has tested negative for antibodies\n"RESET,a->id+1);
                a->num++;
            }
            if(a->num==4)
            {
                pthread_mutex_lock(&global_lock);
                wait_students--;
                pthread_mutex_unlock(&global_lock);
                break;
            }
        }
    }
}
