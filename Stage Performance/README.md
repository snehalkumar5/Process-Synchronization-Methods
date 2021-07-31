## ASSIGNMENT - 4
## Q3 - Musical Mayhem
### SNEHAL KUMAR
### 2019101003
The stages(acoustic/electric), performers(musician/singer) and coordinators are represented as threads which process concurrently. The program ends when all performers are done/exit.
There is global semaphore to signal coordinator when a performer is ready to collect tshirt after performing on stage:
`sem_t  gocollect`
### STAGE
Acoustic and electric stages are both 'stages' which type variable differentiating them as the process remains same. 
Each stage has states:
1. empty
2. solo musician
3. solo singer
4. duo
 
When stage is empty it looks for available performer and assigns it that stage and changes the state. 
When stage is in 'state 2' it looks for available singer to join the musician. If found, it assigns the singer the stage and changes the states of the performers (musician time gets extended by 2sec). It then waits for end of musician and singer using the semaphore and changes stage state to 'empty'
When in 'state 3' it waits for singer's performance to end before changing stage state to 'empty' and starting the process again.
 
### PERFORMER (MUSICIAN/SINGER)
Each performer after arriving, changes state to 'WAITING TO PERFORM' and waits for a conditional signal to be broadcasted within the wait_time interval of the performer. If it fails to receive a signal, it gets impatient and leaves.
Else, it gets assigned a stage (stage thread) according to the instrument and performs.
Code snippet `music_exec()`:
```c
while(m->state==MUSICIAN_WAITING_TO_PERFORM)
{
	if(pthread_cond_timedwait(&(m->condlock), &(m->lock), &settime) == ETIMEDOUT)
	{
		m->state = MUSICIAN_EXITED;
		sem_post(&(m->performed));
		printf(RED"Musician %s with instrument %c left due to impatience\n"RESET, m->name,m->instrument);
		pthread_mutex_unlock(&(m->lock));
		return  NULL;
	}
}
```
Special case : If a singer joins a musician, the musician's time gets extended (using state) by 2sec and it signals the singer after performing using semaphore `sem_t dual` after which both leave stage.
After performing, it signals the coordinator to collect tshirt using global semaphore and then waits for signal by coordinator to confirm receiving the tshirt before exiting.
Code snippet `music_exec()`:
```c
if(m->type == IS_SINGER)
{
	if(m->state == SINGER_PERFORMING_WITH_MUSICIAN)
	{
		sem_wait(&(m->dual));
		sem_post(&(m->performed)); //performance ended
	}
	else
	{
		sleep(m->perform_time);
		sem_post(&(m->performed)); //performance ended
	}
}
else
{
	sleep(m->perform_time);
	if(m->state == MUSICIAN_PERFORMING_WITH_SINGER)
	{
		sleep(2);
		sem_post(&(m->mystage->mysinger_b->dual));
		sem_post(&(m->performed)); //performance ended
	}
	else
	{
		sem_post(&(m->performed)); //performance ended
	}
}
.
.
.
//signal coordinator to collect tshirt
sem_post(&gocollect);
sem_wait(&(m->collected));
// collected tshirt and now will exit
return NULL;
```
### COORDINATOR
Each coordinator waits for signal to give tshirt to any such available performer (global semaphore). Once received, it looks for the performer who is available and gives the tshirt after which it sends signal to the performer to indicate completion and waits again for another performer.
Code snippet `coord_exec()`:
```c
sem_wait(&gocollect);
coord->state = COORDINATOR_BUSY;
.
.
// look for perfomer waiting to collect
if(mus->state == MUSICIAN_WAITING_FOR_SHIRT)
{
	mus->state = MUSICIAN_COLLECTING_SHIRT;
	coord->mymusician = mus;
}
.
.
// signal performer
sem_post(&(coord->mymusician->collected));
coord->state = COORDINATOR_FREE;
```
#### BONUS
1. Singer also collects t-shirt (treated as performer)
2. Keep track of stage number.

This program uses mutex locks and semaphores for synchronization. The additional use of semaphores prevent busy waiting and save CPU cycles.

