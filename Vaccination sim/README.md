## ASSIGNMENT - 3
## Q2 - Back to College
### SNEHAL KUMAR
### 2019101003

Company, Zone, Students are threads which concurrently process their individual functions. The program runs till all student threads have finished their process (pthread_join(&students,NULL)). 
There is a global lock to keep track of the number of waiting students:
``pthread_mutex_t  global_lock;``
### COMPANY
Each company produces batches of vaccine with some probability. After preparing the batches, it signals the zone for delivering a batch. Once there are no more zones waiting for delivery, the company waits for all its batches to be used until it can start production again.
Code snippet `comp_exec()`:
```c
while(1)
{
printf(CYAN  "Pharmaceutical Company %d is preparing %d batches of vaccines which have success probability %Lf\n"RESET,c->id,c->total_batch,c->vaccine_prob);
sleep(c->prep_time);
printf(CYAN"Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %Lf. Waiting for all the vaccines to be used to resume production\n"RESET,c->id,c->total_batch,c->vaccine_prob);
c->batch_left=c->total_batch;
c->batch_used=0;
//busy wait till all batches of company are used
while(c->total_batch>c->batch_used);
c->batch_left=0;
printf(CYAN"All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n"RESET,c->id);
}
return  NULL;
```
### ZONE
Each zone first waits for a batch from a company:  
Code snippet:`zone_exec()`:
```c
if(comp->batch_left>0)
{
	printf(RED"Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %Lf\n"RESET,comp->id,z->id,comp->vaccine_prob);
	comp->batch_left--;
	z->num_vaccine=comp->batch_capacity;
	z->left_vaccine=z->num_vaccine;
	pthread_mutex_unlock(&(comp->lock));
	break;
}
```
Once delivered, it then generates slots for the vaccination phase. Then, the zone proceeds to the vaccination phase where it waits for the students who get vaccinated (student thread). 

```c
printf(RED"Vaccination Zone %d entering Vaccination Phase\n"RESET,z->id);
sleep(1);
while(z->left_vaccine>0)
{
	//generate slots
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
	pthread_mutex_unlock(&(z->lock));
	printf(RED"Vaccination Zone %d is ready to vaccinate with %d slots.\n"RESET,z->id,z->totalslots);
	// start vaccination phase
	z->state=1;
	z->slots_used=0;
	
	// busy wait till slots assigned have been used
	while(z->slots_used<z->totalslots);
	z->state=0;
	sleep(1);
}
```
Now, in the phase, the zone busy waits for all the slots to be used by the students and changes the state of zone which prevents students to be allotted while in the vaccination phase. 
If the vaccines in the zone are finished, it sends signal to company to deliver another batch. 
### STUDENT
After arriving at random times, the students proceed to make themselves available for vaccination and search for slots available in any zone. Once allotted a slot, the student goes into vaccination phase and gets vaccinated and sends signal of using the slot to the zone after which he is tested for antibodies by the probability of the vaccine of the zone given by a company. If successful, it exits else if unsuccessful thrice, it exits.
Code snippet:`run_stud()`:
```c
while(a->num<4 && a->check!=1)
{
	a->time=rand()%3+1; //arrival time
	sleep(a->time);
	printf(YELLOW"Student %d has arrived for his %s round of Vaccination\n"RESET,a->id,roundno);
	sleep(1);
	printf(YELLOW"Student %d is waiting to be allocated a slot on a Vaccination Zone\n"RESET,a->id);
	.
	// check for slots
	.
	.
// student in vaccination phase

	printf(GREEN"Student %d on Vaccination Zone %d has been vaccinated which has success probability %Lf\n"RESET,a->id,a->myzone->id,a->myzone->mycomp->vaccine_prob);
	if(a->check!=1)
	{
		pthread_mutex_lock(&(a->myzone->lock));
		a->myzone->slots_used++;
		pthread_mutex_unlock(&(a->myzone->lock));
		if(cksuccess(a->myzone))
		{
			printf(PURPLE"Student %d has tested positive for antibodies\n"RESET,a->id);
			a->check=1;
			pthread_mutex_lock(&global_lock);
			wait_students--;
			pthread_mutex_unlock(&global_lock);
			break;
		}
		else
		{
			printf(PURPLE"Student %d has tested negative for antibodies\n"RESET,a->id);
			a->num++;
		}
	}
}
```
The program uses mutex locks for synchronisation and prevents potential deadlocks when multiple threads access shared/global data.
