/********************************************************
 * Filename: core/timer.c
 *
 * Author: wsyoo, RTOSLab. SNU.
 * 
 * Description: 
 ********************************************************/
#include <core/eos.h>

static eos_counter_t system_timer;

int8u_t eos_init_counter(eos_counter_t *counter, int32u_t init_value) {
	counter->tick = init_value;
	counter->alarm_queue = NULL;
	return 0;
}

void eos_set_alarm(eos_counter_t* counter, eos_alarm_t* alarm, int32u_t timeout, void (*entry)(void *arg), void *arg) {
	_os_remove_node(&(counter->alarm_queue), &(alarm->alarm_queue_node)); // 1. 카운터의 알람큐에서 알람을 제거
	if (timeout == 0 || entry == NULL) return; // 2. Timeout 이 0 이거나 entry 가 NULL 일 경우 리턴
	alarm->timeout = timeout; // 3. parameter로 알람 구조체의 각 variable 재정의
	alarm->handler = entry; 	
	alarm->arg = arg;
	alarm->alarm_queue_node.priority = timeout; // closest time has the highest priority
  //PRINT("alarm set on %d\n", timeout);
	_os_add_node_priority(&(counter->alarm_queue), &(alarm->alarm_queue_node)); // 4. 카운터의 알람큐의 적절한 위치에 알람 추가
	// automatically ordered by priority sorting!!
	//PRINT("[%d] Will be timed out in %d\n", counter->tick, timeout);
}

eos_counter_t* eos_get_system_timer() {
	return &system_timer;
}

void eos_trigger_counter(eos_counter_t* counter) { // for each real system clock's (SIGALRM) even tick
	PRINT("tick %d\n", counter->tick); // Changed PRINT to prinf for the better visualization
	
	counter->tick ++; // increase counter's tick by 1

	if (counter->alarm_queue->priority > counter->tick) eos_schedule(); // no timeout (idle task scheduled)
	else{
		_os_node_t *_os_ring_queue[64]; // queue to reorder the timeout tasks in real priority
		int count=0;
		while(counter->alarm_queue->priority == counter->tick){
			_os_node_t *sch_alarm_n = counter->alarm_queue;	
			eos_tcb_t *sch_task = sch_alarm_n->ptr_data;
			eos_alarm_t *sch_alarm = sch_task->alarm;
		
			_os_add_node_priority(&_os_ring_queue[0], sch_task->schb);
			count ++;

			_os_remove_node(&(counter->alarm_queue), &(sch_alarm->alarm_queue_node));
			if (counter->alarm_queue == NULL) break;
		}
		//PRINT("Alarms ring : %d\n", count);
		for (int k = 0; k<count; k++){
			_os_node_t *sch_schb = _os_ring_queue[0];
			eos_tcb_t *sch_task = sch_schb->ptr_data;
			eos_alarm_t *sch_alarm = sch_task->alarm;

			_os_add_node_priority(&(counter->alarm_queue), &(sch_alarm->alarm_queue_node));
			_os_remove_node(&_os_ring_queue[0], sch_schb);
		}

		while(counter->alarm_queue->priority == counter->tick){
			_os_node_t* sch_alarm_n = counter->alarm_queue;
			eos_tcb_t* sch_task = sch_alarm_n->ptr_data;
			eos_alarm_t* sch_alarm = sch_task->alarm;
			
			_os_remove_node(&(counter->alarm_queue), &(sch_alarm->alarm_queue_node));
			sch_alarm->handler(sch_task);
			if (counter->alarm_queue == NULL) break;
		}
  eos_schedule();
  //PRINT("alarm count : %d\n", count);
	}
}

/* Timer interrupt handler */
static void timer_interrupt_handler(int8s_t irqnum, void *arg) { // controlled by SIGALRM (real system clock!)
	/* trigger alarms */
	eos_trigger_counter(&system_timer);
}

void _os_init_timer() {
	eos_init_counter(&system_timer, 0);

	/* register timer interrupt handler */
	eos_set_interrupt_handler(IRQ_INTERVAL_TIMER0, timer_interrupt_handler, NULL);
}
