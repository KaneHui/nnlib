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

#ifndef NN_OP_NON_LIN_COMMON_H
#define NN_OP_NON_LIN_COMMON_H

#define ALIGN_SIZE (128)
#define MAXPAD     (2*ALIGN_SIZE)
static inline void *pad_and_align(void *ptr, unsigned long minsize)
{
	uintptr_t ptrval = (uintptr_t)(ptr);
	ptrval += minsize + (MAXPAD-1);
	ptrval &= ~(MAXPAD-1);
	return (void *)ptrval;
}

static inline void requant_u8u8_inplace(uint8_t *data, int numbytes, float in_min, float in_max, float out_min, float out_max)
{
	#define QUANTMAX_8BITVAR    (0x000000FFL)
	#define QUANTSHIFT_16BITVAR (15)
	float out_level_recip;
	float in_level;
	float limgain;
	short gains;
	int gain;
	int offset;
	int rval;
	int lval;
	int i;
	
	// compute with appropriate bitshift the gain and offset for requantization from input min-max to output min-max on data
	out_level_recip = ((float)(QUANTMAX_8BITVAR) / (out_max - out_min));
	in_level = ((in_max - in_min) / (float)(QUANTMAX_8BITVAR));
	offset = (int)((in_min - out_min) / in_level);
	limgain = (float)(powf(2.0, QUANTSHIFT_16BITVAR));
	gain = (int)(out_level_recip * in_level * limgain);
	if (gain > (int)(limgain - 1)) {
		gains = (short)(limgain - 1); 
	} else {
		gains = (short)(gain);
	}
	
	// apply with appropriate bitshift the gain and offset for requantization from input min-max to output min-max on data
	for (i = 0; i < numbytes; i++) {
		rval = ((int)data[i] & QUANTMAX_8BITVAR);
		lval = (((rval + offset) * gains) + ((int)(limgain) / 2)) >> QUANTSHIFT_16BITVAR;
		if (lval > QUANTMAX_8BITVAR) {
			lval = QUANTMAX_8BITVAR; 
		} else if (lval < 0) {
			lval = 0;
		}
		data[i] = (uint8_t)(lval & QUANTMAX_8BITVAR);
	}
}

static inline void dequant_u8(float *out, const uint8_t *in, int numbytes, float in_min, float in_max)
{
	#define QUANTMAX_8BITVAR    (0x000000FFL)
	float range = fmaxf(0.0001f,(in_max - in_min));
	float stepsize = (range / (float)(QUANTMAX_8BITVAR));
	int i;
	
	// apply dequantization
	for (i = 0; i < numbytes; i++) {
		out[i] = (in[i] * stepsize) + in_min;
	}
}

static inline void quant_u8(uint8_t *out, const float *in, int numbytes, float out_min, float out_max)
{
	#define QUANTMAX_8BITVAR    (0x000000FFL)
	float range = fmaxf(0.0001f,(out_max - out_min));
	float steprecip = ((float)(QUANTMAX_8BITVAR) / range);
	int lval;
	int i;
	
	// apply quantization
	for (i = 0; i < numbytes; i++) {
		lval = roundf((in[i] - out_min) * steprecip);
		if (lval > QUANTMAX_8BITVAR) {
			lval = QUANTMAX_8BITVAR; 
		} else if (lval < 0) {
			lval = 0;
		}		
		out[i] = (uint8_t)(lval & QUANTMAX_8BITVAR);
	}
}

#endif
