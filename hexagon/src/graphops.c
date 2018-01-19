
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

/*
 * 
 * Now that that's out of the way, let's get to the good stuff.
 * 
 * This contains operations on the graph
 */

#include <nn_graph.h>

static inline void log_causality(
	struct nn_graph *nn, 
	struct nn_node *tmp, 
	struct nn_node *producer)
{
	logmsg(nn,0,
		"CAUSALITY VIOLATION: "
		"node %p (id=0x%x) referenced output of node %p (id=0x%x) "
		"before instantiated in the graph",
		tmp,
		tmp->node_id,
		producer,
		producer->node_id);
}
/* Returns the last node in the graph to reference the input. */
/* If no node references the input, producer is returned.  */

struct nn_node* find_last_consumer(
	struct nn_graph *nn, 
	struct nn_node *producer, 
	int out_idx)
{
	struct nn_node *tmp;
	struct nn_node *last_node = producer;
	struct input *in;
	int i;
	int seen_producer = 0;
	uint32_t prod_id = producer->node_id;
	for (tmp = nn->head; tmp != NULL; tmp = tmp->next) {
		for (i = 0; i < tmp->n_inputs; i++) {
			in = &tmp->input_refs[i];
			if (in->src_id != prod_id) continue;
			if (in->output_idx != out_idx) continue;
			if (!seen_producer) {
				log_causality(nn,tmp,producer);
			} else {
				last_node = tmp;
			}
		}
		if (tmp == producer) seen_producer = 1;
	}
	return last_node;
}

/* Returns the last node in the graph to reference the input. */
/* If no node references the input, producer is returned.  */

struct nn_node* find_first_consumer(
	struct nn_graph *nn, 
	struct nn_node *producer, 
	int out_idx)
{
	struct nn_node *tmp;
	struct input *in;
	int i;
	int seen_producer = 0;
	uint32_t prod_id = producer->node_id;
	for (tmp = nn->head; tmp != NULL; tmp = tmp->next) {
		for (i = 0; i < tmp->n_inputs; i++) {
			in = &tmp->input_refs[i];
			if (in->src_id != prod_id) continue;
			if (in->output_idx != out_idx) continue;
			if (!seen_producer) {
				log_causality(nn,tmp,producer);
			} else {
				return tmp;
			}
		}
		if (tmp == producer) seen_producer = 1;
	}
	return producer;
}
/* Returns the *only* node in the graph to reference the input. */
/* If no node references the input, or if there's more than  */
/* one reference (including from the same node), producer is returned.  */

struct nn_node* find_unique_consumer(
	struct nn_graph *nn, 
	struct nn_node *producer, 
	int out_idx)
{
	struct nn_node *tmp;
	struct nn_node *result = producer;
	struct input *in;
	int i;
	int seen_producer = 0;
    int count = 0;
	uint32_t prod_id = producer->node_id;
	for (tmp = nn->head; tmp != NULL; tmp = tmp->next) {
		for (i = 0; i < tmp->n_inputs; i++) {
			in = &tmp->input_refs[i];
			if (in->src_id != prod_id) continue;
			if (in->output_idx != out_idx) continue;
			if (!seen_producer) {
				log_causality(nn,tmp,producer);
			} else {
                count++;
                if (count > 1) return producer;
                result = tmp;
			}
		}
		if (tmp == producer) seen_producer = 1;
	}
	return result;
}
//
// returns true iff the output
//  (producer, out_idx) 
//  goes only to a single input, and that
//  input is on 'consumer'
//
int check_single_consumer(
	struct nn_graph *nn, 
	struct nn_node *producer, 
	int out_idx,
	struct nn_node *consumer)
{
	struct nn_node *tmp;
	struct input *in;
	int i;
	int seen_producer = 0;
    int count = 0;
	uint32_t prod_id = producer->node_id;

	for (tmp = nn->head; tmp != NULL; tmp = tmp->next) {
		for (i = 0; i < tmp->n_inputs; i++) {
			in = &tmp->input_refs[i];
			if (in->src_id != prod_id) continue;
			if (in->output_idx != out_idx) continue;
			if (!seen_producer) {
				log_causality(nn,tmp,producer);
			} else {
                count ++;
                if( tmp != consumer || count > 1) return 0;
			}
		}
		if (tmp == producer) seen_producer = 1;
	}
	return count == 1;
}

//
// this is given a list of one or more node items
//    rmnodes[0..n_remove-1]
//  which must exist in the list in the order given, possibly
//  with intervening items. 'anchor' is an upstream anchor for these
//   (i.e. *anchor = rmnodes[0]; or (*anchor)->next = rmnodes[0], etc)
//
// All of those are removed from the list, and the first one rmnodes[0] is replaced by new_node.
// it will return -1 if it can't find the removed nodes; this should not
// occur if the assumptions are met. If a non-zero return occurs, it may
// be that some of the items were not deleted. If the first item could not be found,
// the new item is not inserted.
//
// if anchor is NULL, &nn->head is used.
// if new_node is NULL, only the removal of nodes in rmnodes is done.
// if any of the rmnodes[1..n_remove-1] is NULL, it is taken as a list
//  terminator (but there must be at least one item).
//

int replace_node_sequence (
		struct nn_graph *nn,
		struct nn_node ** anchor,	// list anchor, at items[0] or upstream
		struct nn_node * new_node,	// node to replace (may be null)
    	struct nn_node * const * rmnodes,
        int n_remove)
{
	if( anchor == NULL) anchor = &nn->head;
	// find the first one
    struct nn_node * searchfor;
    if( n_remove < 1 || ( searchfor = rmnodes[0]) == NULL )
    	return -1;
    struct nn_node * current = *anchor;
    while( current != searchfor){
    	if( current == NULL)
    		return -1;		// couldn't find the first one...
    	anchor = &current->next;
    	current = *anchor;
    }
    // OK, the anchor points to the first node in rmnodes[].
    if( new_node != NULL){
    	// insert node before the one we're about to delete
    	*anchor = new_node;
    	anchor = &new_node->next;
    	*anchor = current;
    }
    // remove the first item
    current = current->next;
    *anchor = current;

    // remove the rest
    for( int i = 1; i < n_remove; i++){
    	searchfor = rmnodes[i];
    	if (searchfor == NULL) break;	// all done
        while( current != searchfor){
        	if( current == NULL)
        		return -1;		// couldn't find
        	anchor = &current->next;
        	current = *anchor;
        }
        // remove it
        current = current->next;
        *anchor = current;
    }
    return 0;
}


