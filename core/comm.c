/********************************************************
 * Filename: core/comm.c
 *  
 * Author: jtlim, RTOSLab. SNU.
 * 
 * Description: message queue management. 
 ********************************************************/
#include <core/eos.h>

void eos_init_mqueue(eos_mqueue_t *mq, void *queue_start, int16u_t queue_size, int8u_t msg_size, int8u_t queue_type) {
	mq->queue_size = queue_size;
	mq->msg_size = msg_size;

	mq->queue_start = queue_start;
	mq->front = queue_start;
	mq->rear = queue_start;
	mq->queue_type = queue_type;

	eos_init_semaphore(&(mq->putsem), queue_size, queue_type);
	eos_init_semaphore(&(mq->getsem), 0, queue_type);
}

int8u_t eos_send_message(eos_mqueue_t *mq, void *message, int32s_t timeout) {
	if (eos_acquire_semaphore(&(mq->putsem), timeout) == 0) return 0;
	else {
		char *c_message = (char *)message; // message is 1B char
		for (int k=0; k<mq->msg_size; k++){
      *(char *)(mq->rear) = *(c_message + k);
			if (mq->rear < mq->queue_start + (mq->msg_size)*(mq->queue_size)) mq->rear ++;
			else mq->rear = mq->queue_start;
		}
		eos_release_semaphore(&(mq->getsem));
	}
}

int8u_t eos_receive_message(eos_mqueue_t *mq, void *message, int32s_t timeout) {
	if (eos_acquire_semaphore(&(mq->getsem), timeout) == 0) return 0;
	else {
		char *c_message = (char *)message;
		for (int k=0; k<mq->msg_size; k++){
      *(c_message + k) = *(char *)(mq->front);
			if (mq->front < mq->queue_start + (mq->msg_size)*(mq->queue_size)) mq->front ++;
			else mq->front = mq->queue_start;
    }
		eos_release_semaphore(&(mq->putsem));
	}
}
