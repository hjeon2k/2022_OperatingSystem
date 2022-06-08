/********************************************************
 * Filename: core/sync.c
 * 
 * Author: wsyoo, RTOSLab. SNU.
 * 
 * Description: semaphore, condition variable management.
 ********************************************************/
#include <core/eos.h>

#define READY 1
#define RUNNING 2
#define WAITING 3

void eos_init_semaphore(eos_semaphore_t *sem, int32u_t initial_count, int8u_t queue_type) {
	/* initialization */
	sem->count = initial_count; //eos.h int
	sem->queue_type = queue_type; //eos.h bool
	sem->wait_queue = NULL;
}

int32u_t eos_acquire_semaphore(eos_semaphore_t *sem, int32s_t timeout) {
	int32u_t flag = eos_disable_interrupt();
  eos_tcb_t *task = eos_get_current_task(); 
  
 //while(1){
	  if (sem->count > 0){
      sem->count --;
      //PRINT("Sem ACK, count : %d\n", sem->count);
      eos_restore_interrupt(flag);
		  return 1;
	  }

	  else{
		  if (timeout == -1){
			  eos_restore_interrupt(flag);
			  return 0;
		  }

		  else if (timeout == 0){
        if (sem->wait_queue != NULL){ return 0;
          //if (sem->wait_queue->ptr_data != NULL){
          //  if (sem->wait_queue->ptr_data == task) return 0;
          //}
        }
        if (sem->queue_type == FIFO) _os_add_node_tail(&(sem->wait_queue), task->semb);
				else _os_add_node_priority(&(sem->wait_queue), task->semb);
        //PRINT("Add to Q\n");

        eos_restore_interrupt(flag);
        /*eos_sleep(0);
        
        if (sem->wait_queue != NULL) _os_remove_node(&(sem->wait_queue), task->semb);
        flag = eos_disable_interrupt();
        if (sem->count > 0){
          sem->count --;
          //PRINT("Sem ACK, count : %d\n", sem->count);
          eos_restore_interrupt(flag);
		      return 1;
	      }
        else{
          eos_restore_interrupt(flag);
          return 0;
        }*/
      }

		  else { // timeout > 0
        if (sem->queue_type == FIFO) _os_add_node_tail(&(sem->wait_queue), task->semb);
				else _os_add_node_priority(&(sem->wait_queue), task->semb);
        
        eos_restore_interrupt(flag);
		  	eos_sleep(timeout);
		  	eos_acquire_semaphore(sem, -1);
		  }
	  }
  //}
}

void eos_release_semaphore(eos_semaphore_t *sem) {
  int32u_t flag = eos_disable_interrupt();
  sem->count ++;
	
  if (sem->wait_queue != NULL){
    if (sem->wait_queue->ptr_data != NULL){
		eos_tcb_t *task = sem->wait_queue->ptr_data;
		eos_alarm_t* alarm = task->alarm;
    
		_os_remove_node(&(sem->wait_queue), task->semb);
    //PRINT("Remove from Q\n");
    //if (task->schb->priority == 50 && sender_wakeup_flag == 0) sender_wakeup_flag = 1;
    alarm->handler(task);
    }
	}
  //PRINT("Sem Realse now, count :  %d\n", sem->count);
  eos_restore_interrupt(flag);
}


void eos_init_condition(eos_condition_t *cond, int32u_t queue_type) {
	/* initialization */
	cond->wait_queue = NULL;
	cond->queue_type = queue_type;
}

void eos_wait_condition(eos_condition_t *cond, eos_semaphore_t *mutex) {
	/* release acquired semaphore */
	eos_release_semaphore(mutex);
	/* wait on condition's wait_queue */
	_os_wait(&cond->wait_queue);
	/* acquire semaphore before return */
	eos_acquire_semaphore(mutex, 0);
}

void eos_notify_condition(eos_condition_t *cond) {
	/* select a task that is waiting on this wait_queue */
	_os_wakeup_single(&cond->wait_queue, cond->queue_type);
}
