/********************************************************
 * Filename: core/task.c
 * 
 * Author: parkjy, RTOSLab. SNU.
 * 
 * Description: task management.
 ********************************************************/
#include <core/eos.h>

#define READY		1
#define RUNNING		2
#define WAITING		3

/*
 * Queue (list) of tasks that are ready to run.
 */
//extern static _os_node_t *_os_ready_queue[LOWEST_PRIORITY + 1];

/*
 * Pointer to TCB of running task
 */
static eos_tcb_t *_os_current_task;

int32u_t eos_create_task(eos_tcb_t *task, addr_t sblock_start, size_t sblock_size, void (*entry)(void *arg), void *arg, int32u_t priority) {
	//PRINT("task: 0x%x, priority: %d\n", (int32u_t)task, priority);

	task->state = READY;
	task->sp = _os_create_context(sblock_start, sblock_size, entry, arg);
	task->period = 0; // initial task is always running (not 1, min granuality)

	task->schb = (_os_node_t *)malloc(sizeof(_os_node_t));	
	task->schb->ptr_data = task;
	task->schb->priority = priority;

	task->alarm = (eos_alarm_t *)malloc(sizeof(eos_alarm_t));
	task->alarm->timeout = 0; // timeout, hanlder, arg will be set by eos_sleep right after 1st execution
	task->alarm->alarm_queue_node.ptr_data = task;
	task->alarm->alarm_queue_node.priority = 0;
	
  task->semb = (_os_node_t *)malloc(sizeof(_os_node_t));
  task->semb->ptr_data = task;
  task->semb->priority = priority;

	_os_add_node_tail(&_os_ready_queue[priority], task->schb); //still, task is selected FIFO inside the queue
	_os_set_ready(priority);

	//PRINT("created sp : 0x%x\n", task->sp);
	return 0;
}

int32u_t eos_destroy_task(eos_tcb_t *task) {
}

void eos_schedule() {
	eos_tcb_t *task = eos_get_current_task();
	if (task){
		//PRINT("cur process is 0x%x\n", task->sp);
		addr_t task_sp = _os_save_context();
		if (task_sp){
			task->sp = task_sp;
			
			if (task->period == 0){ //for the initial task(always running)
				task->state = READY;
				_os_add_node_tail(&_os_ready_queue[task->schb->priority], task->schb);
				_os_set_ready(task->schb->priority);
			}
		}
		else return;
	}

	int32u_t sch_priority = _os_get_highest_priority();
	_os_node_t *sch_schb = _os_ready_queue[sch_priority];
	if (sch_schb){
		_os_current_task = sch_schb->ptr_data;
		_os_current_task->state = RUNNING;
		
		//PRINT("Next Task P : %d\n", _os_current_task->schb->priority);
		if (_os_current_task->schb->priority == 50){
      if (sender_wakeup_flag == 1){sender_wakeup_flag = 0;}
      else if (sender_inq_flag == 1) {sender_inq_flag = 0;}
      else _os_remove_node(&_os_ready_queue[sch_priority], sch_schb);
    }
    else _os_remove_node(&_os_ready_queue[sch_priority], sch_schb);
		
    if (!_os_ready_queue[sch_priority]) _os_unset_ready(sch_priority); // get schb rid of the ready queue
		_os_restore_context(_os_current_task->sp);
	}
	else{
    return;
  }
}

eos_tcb_t *eos_get_current_task() {
	return _os_current_task;
}

void eos_change_priority(eos_tcb_t *task, int32u_t priority) {
}

int32u_t eos_get_priority(eos_tcb_t *task) {
}

void eos_set_period(eos_tcb_t *task, int32u_t period){
	task->period = period; // called after create_task
	task->alarm->timeout = period;
	task->alarm->alarm_queue_node.priority = period;
}

int32u_t eos_get_period(eos_tcb_t *task) {
}

int32u_t eos_suspend_task(eos_tcb_t *task) {
}

int32u_t eos_resume_task(eos_tcb_t *task) {
}

void eos_sleep(int32u_t tick) {
  eos_tcb_t *task = eos_get_current_task();
	eos_counter_t *sys_timer = eos_get_system_timer();
	
	task->state = WAITING;
	if (tick == 0) eos_set_alarm(sys_timer, task->alarm, task->period + sys_timer->tick, &_os_wakeup_sleeping_task, task);
	else if (tick > 0) eos_set_alarm(sys_timer, task->alarm, tick + sys_timer->tick, &_os_wakeup_sleeping_task, task);
	
	if (!_os_ready_queue[task->schb->priority]) _os_unset_ready(task->schb->priority);
  eos_schedule();
}

void _os_init_task() {
	PRINT("initializing task module.\n");

	/* init current_task */
	_os_current_task = NULL;

	/* init multi-level ready_queue */
	int32u_t i;
	for (i = 0; i < LOWEST_PRIORITY; i++) {
		_os_ready_queue[i] = NULL;
	}
}

void _os_wait(_os_node_t **wait_queue) {
}

void _os_wakeup_single(_os_node_t **wait_queue, int32u_t queue_type) {
}

void _os_wakeup_all(_os_node_t **wait_queue, int32u_t queue_type) {
}

void _os_wakeup_sleeping_task(void *arg) { // [CHECK] void *arg (also in eos.h)
	eos_tcb_t *task = arg;
	task->state = READY;
	_os_add_node_tail(&_os_ready_queue[task->schb->priority], task->schb);
	
	_os_set_ready(task->schb->priority);
	//eos_schedule();
}
