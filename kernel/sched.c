/*
 * Copyright (C) 2009 Renê de Souza Pinto
 * Tempos - Tempos is an Educational and multi purpose Operating System
 *
 * File: sched.c
 * Desc: The TempOS scheduler
 *
 * This file is part of TempOS.
 *
 * TempOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * TempOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <tempos/sched.h>
#include <tempos/kernel.h>
#include <tempos/timer.h>
#include <tempos/jiffies.h>

/** Scheduler Quantum */
static uint32_t scheduler_quantum = (HZ / 100); /* 10 ms */

/** Scheduler time counter */
static size_t sched_cnt;

/** Linked list of all tasks */
c_llist *tasks = NULL;

/** Element of list that points to current task */
c_llist *cur_task = NULL;

/**
 * Initialize the scheduler
 */
void init_scheduler(void (*start_routine)(void*))
{
	/* Create circular linked list */
	c_llist_create(&tasks);

	/* Start scheduler time counter */
	sched_cnt = jiffies + scheduler_quantum;

	/* Architecture dependent */
	arch_init_scheduler(start_routine);
}


/**
 * Decides what task to run and make the task switch.
 *
 * TempOS scheduler uses a round robin policy.
 *
 * \param p Scheduler Quantum (in ticks).
 */
void schedule(pt_regs *regs)
{
	task_t *current_task;
	c_llist *next;

	/* Check if the quantum has exceeded */
	if( !time_after(jiffies, sched_cnt) ) {
		return;
	} else {
		sched_cnt = jiffies + scheduler_quantum;
	}

	/* do schedule */
	if (cur_task == NULL) {
		return;
	}

	next = cur_task->next;
	if (next != NULL) {
		if (next != cur_task) {
			current_task = GET_TASK(next);
			if (current_task->state == TASK_RUNNING || 
					current_task->state == TASK_READY_TO_RUN) {
				switch_to(regs, next);
			}
		}
	}
}

