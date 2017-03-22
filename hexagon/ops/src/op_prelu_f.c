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


#include <nn_graph.h>
#include <string.h>
#include <quantize.h>

static int prelu_execute(struct nn_node *self, struct nn_graph *nn)
{
	const struct tensor *in_tensor = self->inputs[0];
	const struct tensor *in_alpha_tensor = self->inputs[1];
	struct tensor *out_tensor = self->outputs[0];
	size_t elements = in_tensor->shape.batches 
		* in_tensor->shape.height
		* in_tensor->shape.width
		* in_tensor->shape.depth;
	size_t bytes = elements * sizeof(float);
	const float alpha = tensor_get_float(in_alpha_tensor,0);
	const float *in_data = in_tensor->data;
	float *out_data = out_tensor->data;
	uint32_t i;

	logmsg(nn,2,"prelu execute. self=%p ",self);
	if (alpha < 0.0f) return errlog(nn,"negative alpha");
	if (bytes > out_tensor->max_size) return errlog(nn,"out too small");
	out_tensor->shape = in_tensor->shape;
	out_tensor->data_size = bytes;

	for (i = 0; i < elements; i++) {
		out_data[i] = fmaxf(in_data[i],0.0f) + fminf(in_data[i] * alpha,0.0f);
	}

	logmsg(nn,2,"prelu %p done",self);
	return 0;
}

static int prelu_check(struct nn_node *self, struct nn_graph *nn)
{
	logmsg(nn,2,"Checking prelu node %p",self);
	if (self->n_inputs != 2) return errlog(nn,"wrong # inputs");
	if (self->n_outputs != 1) return errlog(nn,"wrong # outputs");
	logmsg(nn,2,"prelu node %p check OK",self);
	return 0;
}

struct nn_node_ops nn_ops_for_PRelu_f = {
	.execute = prelu_execute,
	.check = prelu_check,
	.ctor = node_alloc_common,
	.dtor = node_free_common,
};

