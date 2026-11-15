/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_order.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:36:41 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	heap_precedes(t_request_heap *heap,
		t_heap_entry a, t_heap_entry b)
{
	if (heap->scheduler == SCHED_EDF_POLICY
		&& (a.deadline.tv_sec != b.deadline.tv_sec
			|| a.deadline.tv_nsec != b.deadline.tv_nsec))
		return (!time_reached(a.deadline, b.deadline));
	return (a.arrival < b.arrival);
}

static void	heap_sift_up(t_request_heap *heap, t_heap_node *node)
{
	t_heap_entry	entry;

	entry = node->entry;
	while (node->parent != NULL
		&& heap_precedes(heap, entry, node->parent->entry))
	{
		node->entry = node->parent->entry;
		node = node->parent;
	}
	node->entry = entry;
}

static void	heap_sift_down(t_request_heap *heap, t_heap_node *node)
{
	t_heap_entry	entry;
	t_heap_node		*child;

	entry = node->entry;
	while (node->left != NULL)
	{
		child = node->left;
		if (node->right != NULL
			&& heap_precedes(heap, node->right->entry, child->entry))
			child = node->right;
		if (!heap_precedes(heap, child->entry, entry))
			break ;
		node->entry = child->entry;
		node = child;
	}
	node->entry = entry;
}

void	heap_repair(t_request_heap *heap, t_heap_node *node)
{
	if (node->parent != NULL
		&& heap_precedes(heap, node->entry, node->parent->entry))
		heap_sift_up(heap, node);
	else
		heap_sift_down(heap, node);
}
