
/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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
/*
 * 
 * Now that that's out of the way, let's get to the good stuff.
 * 
 * This contains implementations for quantized max pooling node
 */

#include <nn_graph.h>
#include <string.h>

static inline uint8_t __attribute__((unused)) uint8_t_max(uint8_t a, uint8_t b)
{
	if (a > b) return a;
	else return b;
}

#if 0
static inline void l2fetch(void *ptr, uint32_t stride, uint32_t width, uint32_t height)
{
	union {
		unsigned long long int raw;
		struct {
			unsigned short height;
			unsigned short width;
			unsigned int stride;
		};
	} x;
	x.stride = stride;
	x.height = height;
	x.width = width;
	asm volatile ("l2fetch(%0,%1)"::"r"(ptr),"r"(x.raw));
}
#endif

struct tdata {
	struct nn_node *self;
	int whoami;
	nn_sem_t donesem;
};

static void maxpool_execute_slice(struct nn_graph *nn, void *vinfo)
{
	struct tdata *info = vinfo;
	struct nn_node *self = info->self;
	int whoami = info->whoami;
	const struct tensor *in_tensor = self->inputs[0];
	const struct tensor *window_tensor = self->inputs[3];
	const struct tensor *stride_tensor = self->inputs[4];
	struct tensor *out_tensor = self->outputs[0];

	int32_t in_batches = in_tensor->shape.batches;
	int32_t in_width = in_tensor->shape.width;
	int32_t in_height = in_tensor->shape.height;
	int32_t in_depth = in_tensor->shape.depth;

	int32_t stride_width = stride_tensor->shape.width;
	int32_t stride_height = stride_tensor->shape.height;

	int32_t window_height = window_tensor->shape.height;
	int32_t window_width = window_tensor->shape.width;

	int32_t out_batches = in_batches;
	int32_t out_width = nn_pad_compute_outsize(in_width,window_width,stride_width,self->padding);
	int32_t out_height = nn_pad_compute_outsize(in_height,window_height,stride_height,self->padding);
	int32_t out_depth = in_depth;

	int32_t batch;
	int32_t out_x;
	int32_t out_y;
	//int32_t out_z;
	//int32_t in_x;
	//int32_t in_y;
	//int32_t in_z;

	int32_t start_x;
	int32_t start_y;
	int32_t end_x;
	int32_t end_y;
	int32_t next_y;

	uint8_t *in = in_tensor->data;
	uint8_t *out = out_tensor->data;

	int32_t adj_x = ((out_width-1) * stride_width + window_width - in_width) / 2;
	int32_t adj_y = ((out_height-1) * stride_height + window_height - in_height) / 2;

	//uint32_t maxval;

	/* Yes, indentation inconsistent, but depth gets big. */
	/* Would be good to refactor here... */
	/* foreach out batch */
	for (batch = 0; batch < out_batches; batch++) {
	  /* foreach out y */
	  for (out_y = whoami; out_y < out_height; out_y+=2) {
	    start_y = out_y * stride_height - adj_y;
	    next_y = (out_y + 2) * stride_height - adj_y;
	    end_y = start_y + window_height;
	    if (start_y < 0) start_y = 0;
	    if (end_y > in_height) end_y = in_height;
	    l2fetch(&in[in_depth*in_width*next_y],
		in_width*in_depth,
		in_width*in_depth,
		stride_height);
	    /* foreach out x */
	    for (out_x = 0; out_x < out_width; out_x++) {
	      start_x = out_x * stride_width - adj_x;
	      end_x = start_x + window_width;
	      if (start_x < 0) start_x = 0;
	      if (end_x > in_width) end_x = in_width;
	      /* foreach out z */
	      /*
	       * Since Z is consecutive, 
	       * should be easy to vectorize along Z
	       */

#ifndef __hexagon__
	      for (out_z = 0; out_z < out_depth; out_z++) {
	        maxval = 0;
		in_z = out_z;
	        /* foreach window y */
	        for (in_y = start_y; in_y < end_y; in_y++) {
	          /* foreach window x */
	          for (in_x = start_x; in_x < end_x; in_x++) {
	            uint32_t data = in[in_z 
	                      + in_depth * (in_x 
	                        + in_width * ( in_y 
	                          + in_height * (batch)))];
	            if (data > maxval) maxval = data;
	          }
	        }
		out[out_z
	            + out_depth * (out_x
	              + out_width * (out_y
	                + out_height * (batch)))] = maxval;
	      }
#else
             uint8_t * out0=&out[out_depth* out_width* out_height* batch + out_depth* out_x + out_depth * out_width * out_y];
             uint8_t * in0 = &in[in_depth * in_width * in_height * batch + start_y * in_depth * in_width + start_x * in_depth];
              if((in_depth % 128)==0)
                maxpool_aligned_hvx(out0, in0, in_depth, end_x-start_x, end_y-start_y, in_width);
              else
                maxpool_nonaligned_hvx(out0, in0, in_depth, end_x-start_x, end_y-start_y, in_width);
#endif
	    }
	  }
	}

#ifdef __hexagon__
        //SIM_RELEASE_HVX;
#endif
	nn_sem_post(&info->donesem);
}

static int maxpool_execute(struct nn_node *self, struct nn_graph *nn)
{
	const struct tensor *in_tensor = self->inputs[0];
	const struct tensor *in_min_tensor = self->inputs[1];
	const struct tensor *in_max_tensor = self->inputs[2];
	const struct tensor *window_tensor = self->inputs[3];
	const struct tensor *stride_tensor = self->inputs[4];
	struct tensor *out_tensor = self->outputs[0];
	struct tensor *out_min_tensor = self->outputs[1];
	struct tensor *out_max_tensor = self->outputs[2];

	int32_t in_batches = in_tensor->shape.batches;
	int32_t in_width = in_tensor->shape.width;
	int32_t in_height = in_tensor->shape.height;
	int32_t in_depth = in_tensor->shape.depth;

	int32_t stride_width = stride_tensor->shape.width;
	int32_t stride_height = stride_tensor->shape.height;

	int32_t window_height = window_tensor->shape.height;
	int32_t window_width = window_tensor->shape.width;

	int32_t out_batches = in_batches;
	int32_t out_width = nn_pad_compute_outsize(in_width,window_width,stride_width,self->padding);
	int32_t out_height = nn_pad_compute_outsize(in_height,window_height,stride_height,self->padding);
	int32_t out_depth = in_depth;

	size_t bytes = out_batches * out_width * out_height * out_depth;

	struct tdata my_info = {
		.self = self,
		.whoami = 0,
	};
	struct tdata worker_info = {
		.self = self,
		.whoami = 1,
	};
	nn_sem_init(&worker_info.donesem,0);

#ifdef __hexagon__
        //SIM_ACQUIRE_HVX;
        //SIM_SET_HVX_DOUBLE_MODE;
#endif
	/* Assert min and max are size 1,1,1,1 ? */

	/* check size of output */

	logmsg(nn,2,"maxpool execute. self=%p ",self);
	if ((window_tensor->shape.batches != 1)
		|| (window_tensor->shape.depth != 1)
		|| (stride_tensor->shape.batches != 1)
		|| (stride_tensor->shape.depth != 1)) {
		return errlog(nn,"bad window/stride shape");
	}
	if (bytes > out_tensor->max_size) return errlog(nn,"out too small");
	if (self->padding == NN_PAD_NA) return errlog(nn,"This op might pad");
	tensor_set_shape(out_tensor,out_batches,out_height,out_width,out_depth);
	out_tensor->data_size = bytes;

	nn_os_work_for_vector(nn,maxpool_execute_slice,&worker_info);
	//maxpool_execute_slice(nn,&worker_info);
	maxpool_execute_slice(nn,&my_info);
	nn_sem_wait(&worker_info.donesem);

	tensor_copy(out_min_tensor,in_min_tensor);
	tensor_copy(out_max_tensor,in_max_tensor);

	logmsg(nn,2,"maxpool %p done",self);
	return 0;
}

static inline void logmsg_input(
	struct nn_graph *nn,
	int logval,
	int index,
	const struct tensor *tens)
{
	logmsg(nn,logval,"input %d: BHWD=%d,%d,%d,%d data %d bytes @ %p",
		index,
		tens->shape.batches,
		tens->shape.height,
		tens->shape.width,
		tens->shape.depth,
		tens->data_size,
		tens->data);
}

static int maxpool_check(struct nn_node *self, struct nn_graph *nn)
{
	int i;
	logmsg(nn,2,"Checking maxpool node %p",self);
	if (self->n_inputs != 5) return errlog(nn,"maxpool wrong # inputs");
	if (self->n_outputs != 3) return errlog(nn,"maxpool wrong # outs");
	logmsg(nn,3,"conv2d node %p inputs: "
		"[in, min_in, max_in, window, stride]:",
		self);
	for (i = 0; i < self->n_inputs; i++) {
		if (self->inputs[i] == NULL) {
			return errlog(nn,"maxpool NULL input %d",i);
		}
		logmsg_input(nn,3,i,self->inputs[i]);
	}
	for (i = 0; i < self->n_inputs; i++) {
		if (self->inputs[i] == NULL) {
			return errlog(nn,"maxpool NULL output %d",i);
		}
	}
	logmsg(nn,2,"maxpool node %p check OK",self);
	return 0;
}

struct nn_node_ops nn_ops_for_QuantizedMaxPool_8 = {
	.execute = maxpool_execute,
	.check = maxpool_check,
	.ctor = node_alloc_common,
	.dtor = node_free_common,
};

struct nn_node_ops nn_ops_for_QuantizedMaxPool_8_ref = {
	.execute = maxpool_execute,
	.check = maxpool_check,
	.ctor = node_alloc_common,
	.dtor = node_free_common,
};


