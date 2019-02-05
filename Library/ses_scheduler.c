/*INCLUDES ************************************************************/
#include "ses_timer.h"
#include "ses_scheduler.h"
#include <util/atomic.h>

/* PRIVATE VARIABLES **************************************************/
/** list of scheduled tasks */
static taskDescriptor* taskList = NULL;
static systemTime_t millisecs = 0;

/*FUNCTION DEFINITION *************************************************/
static void scheduler_update(void) {
	/*increased by 1 for every 1 ms*/
	millisecs++;

	taskDescriptor* local = taskList;		//begin of the list

	while(local != NULL ){
		if(local->expire != 0)
		{
			local->expire-- ; 			//update every 1ms
		}
		else
		{
			local->execute = 1;		//set execution if time expired
			local->expire = local->period; //repeat the task
		}

		local = local->next;			//Point to next task in list
	}
}

void scheduler_init() {
	timer2_start();
	timer2_setCallback(scheduler_update);
}

void scheduler_run() {
	if(taskList  == NULL)
	{
		return;
	}

	taskDescriptor *local = NULL;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	local = taskList;
	}

	while(local != NULL){
		if(local->execute == 1)
		{

			local->task(local->param); 	//execute the task
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
			local->execute = 0;			//clear execution after executing
			}
			if(local->period == 0)		//remove from scheduler if no period
			{
				scheduler_remove(local);
			}
		}

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		local = local->next;
		}
	}
}

bool scheduler_add(taskDescriptor * toAdd) {
	taskDescriptor *local = NULL;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	local = taskList;
	}

	/*check if there is task*/
	if(toAdd == NULL)
	{
		return 0;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	while(local != NULL){
		/*check if the task is in the list*/
		if(local == toAdd)
		{
			return 0;
		}
		local = local->next;
		}
	}

		/*task is added at the beginning of the list*/
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		local = taskList;
		taskList = toAdd;
		taskList->next = local;
		}
	return 1;
}

void scheduler_remove(taskDescriptor * toRemove) {
	/*if there is no task in the list*/
	if(taskList  == NULL)
	{
		return;
	}

	taskDescriptor *local 		= NULL;
	taskDescriptor *local_next 	= NULL;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	local = taskList;
	local_next 	= taskList->next;
	}

	/*if the task is at the beginning of the list*/
	if(local == toRemove)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
		taskList = taskList->next;
		}
		return;
	}

	/*if the task is at somewhere else*/
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	while(local_next != NULL){
		if(local_next == toRemove)
		{
			local = local_next;
			local->next = local_next->next;		//Skip to next address
			return;

		}
		else
		{
			local_next = local_next->next;
		}
	}
	}
}

systemTime_t scheduler_getTime()
{
	systemTime_t temp = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	temp = millisecs;
	}
	return temp;
}

void scheduler_setTime(systemTime_t time)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
	millisecs = time;
	}
}
