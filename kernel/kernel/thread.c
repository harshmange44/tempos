/*
 * Copyright (C) 2009 Renê de Souza Pinto
 * Tempos - Tempos is an Educational and multi purpose Operating System
 *
 * File: thread.c
 * Desc: Implements functions to create/handle threads.
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
#include <tempos/wait.h>
#include <tempos/mm.h>
#include <arch/io.h>
#include <linkedl.h>
#include <string.h>

void force_kthread_exit(void)
{
	kernel_thread_exit(0);
}

task_t *kernel_thread_create(int priority, void (*start_routine)(void *), void *arg)
{
	task_t *newth = NULL;
	char *new_kstack = NULL;
	void *udata;

	/* Alloc memory for task structure */
	newth = (task_t*)kmalloc(sizeof(task_t), GFP_NORMAL_Z);
	if (newth == NULL) {
		return NULL;
	}

	/* Alloc memory for process kernel stack */
	new_kstack = (char*)kmalloc(PROCESS_STACK_SIZE, GFP_NORMAL_Z);
	if (new_kstack == NULL) {
		kfree(newth);
		return NULL;
	}

	newth->state = TASK_READY_TO_RUN;
	newth->priority = priority;
	newth->pid = KERNEL_PID;
	newth->return_code = 0;
	newth->wait_queue = 0;
	newth->stack_base = new_kstack;
	newth->kstack = (char*)((void*)new_kstack + PROCESS_STACK_SIZE);

	/* "push" start_routine argument */
	udata = arg;
	push_into_stack(newth->kstack, udata);

	/* "push" return address */
	udata = force_kthread_exit; 
	push_into_stack(newth->kstack, udata);

	/* Architecture specific */
	setup_task(newth, start_routine);

	/* Add to task queue */
	cli();
	c_llist_add(&tasks, newth);
	sti();

	/* Return */
	return newth;
}

void kernel_thread_exit(int return_code)
{
	task_t *current_task;

	cli();
	current_task = GET_TASK(cur_task);
	current_task->state = TASK_ZOMBIE;
	current_task->return_code = return_code;
	wakeup(WAIT_KERNEL_THREAD);
	sti();
	schedule();
}

int kernel_thread_wait(task_t *th)
{
	int ret;

	if (th == NULL) {
		return -1;
	}

	while(th->state != TASK_ZOMBIE)
		sleep_on(WAIT_KERNEL_THREAD);

	cli();
	ret = th->return_code;
	c_llist_remove(&tasks, th);
	kfree(th->stack_base);
	kfree(th);
	sti();

	return ret;
}

