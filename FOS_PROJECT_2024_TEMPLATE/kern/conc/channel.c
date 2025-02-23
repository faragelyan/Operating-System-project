/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct spinlock* lk)
{
	//TODO: [PROJECT'24.MS1 - #10] [4] LOCKS - sleep
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("sleep is not implemented yet");
    struct Env* sofa_proc = get_cpu_proc();

    release_spinlock(lk);//imporrrrrtaaaant to ensure if any one hold this lock i should realese it

    // Protection
    acquire_spinlock(&ProcessQueues.qlock);  // Ensure qlock is helded
    enqueue(&chan->queue, sofa_proc);
    sofa_proc->env_status = ENV_BLOCKED;
    release_spinlock(&ProcessQueues.qlock);

    acquire_spinlock(&ProcessQueues.qlock);  // Ensure qlock is helded
    sched();
    release_spinlock(&ProcessQueues.qlock);

    acquire_spinlock(lk);//back the lock to original
}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	//TODO: [PROJECT'24.MS1 - #11] [4] LOCKS - wakeup_one
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	acquire_spinlock(&ProcessQueues.qlock);
/*	   while (true)
	            {
	                // Per some operations

	                it++;

	                if (it >= 10)
	                {
	                    break;
	                }

	            }*/
	    if (queue_size(&chan->queue) > 0) {
	        struct Env *process = dequeue(&chan->queue);
	        process->env_status = ENV_READY;
	        sched_insert_ready0(process);
	    }

	    release_spinlock(&ProcessQueues.qlock);
/*        while (false)
        {

        }*/
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
	//TODO: [PROJECT'24.MS1 - #12] [4] LOCKS - wakeup_all
	int x =0;
	 acquire_spinlock(&ProcessQueues.qlock);

	    while (queue_size(&chan->queue) > 0) {
	        struct Env *prso = dequeue(&chan->queue);
	        prso->env_status = ENV_READY;
	        sched_insert_ready0(prso);
	    }

	    release_spinlock(&ProcessQueues.qlock);
}
