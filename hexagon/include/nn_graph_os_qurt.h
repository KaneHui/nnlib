
/*
 * Copyright (c) 2016-2017, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef NN_GRAPH_OS_QURT_H
#define NN_GRAPH_OS_QURT_H 1

#include <qurt.h>
#include <stdlib.h>
struct nn_graph;
typedef qurt_pipe_t nn_pipe_t;
typedef qurt_sem_t nn_sem_t;
static inline void nn_sem_init(nn_sem_t *sem, int val) { qurt_sem_init_val(sem,val); }
static inline void nn_sem_post(nn_sem_t *sem) { qurt_sem_up(sem); }
static inline void nn_sem_wait(nn_sem_t *sem) { qurt_sem_down(sem); }
static inline void nn_pipe_send(nn_pipe_t *pipe, unsigned long long int val) { qurt_pipe_send(pipe,val); }
static inline nn_pipe_t *nn_pipe_alloc(struct nn_graph *nn, uint32_t pipe_elements)
{
	qurt_pipe_attr_t pattr;
	nn_pipe_t *ret;
	qurt_pipe_attr_init(&pattr);
	const unsigned int PIPESIZE_ELEMENTS = 4;
	const unsigned int PIPESIZE_BYTES = PIPESIZE_ELEMENTS * 8;
	qurt_pipe_attr_set_buffer(&pattr,malloc(PIPESIZE_BYTES));
	qurt_pipe_attr_set_elements(&pattr,PIPESIZE_ELEMENTS);
	qurt_pipe_create(&ret,&pattr);
	return ret;
}
static inline unsigned long long int nn_pipe_recv(nn_pipe_t *pipe) { return qurt_pipe_receive(pipe); }
static inline uint64_t nn_os_get_cycles(struct nn_graph *nn) { return qurt_get_core_pcycles(); }
int nn_os_vector_acquire();
void nn_os_vector_release(int idx);
static inline void nn_os_vector_init() {};
void nn_os_hvx_power_on(struct nn_graph *nn);
void nn_os_hvx_power_off(struct nn_graph *nn);
uint64_t nn_os_get_perfcount(struct nn_graph *nn);

int nn_os_workers_spawn(struct nn_graph *nn);
void nn_os_workers_kill(struct nn_graph *nn);
void nn_os_work_for_vector(struct nn_graph *nn, void (*f)(struct nn_graph *, void *),void *arg);
void nn_os_work_for_scalar(struct nn_graph *nn, void (*f)(struct nn_graph *, void *),void *arg);

static inline uint64_t nn_os_get_usecs(struct nn_graph *nn)
{
	return qurt_timer_timetick_to_us(qurt_sysclock_get_hw_ticks());
}

#endif // NN_GRAPH_OS_H
