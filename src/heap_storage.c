/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_storage.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:36:41 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_init(t_request_heap *heap, t_scheduler scheduler)
{
	if (heap == NULL)
		return (1);
	memset(heap, 0, sizeof(*heap));
	if (scheduler != SCHED_FIFO_POLICY && scheduler != SCHED_EDF_POLICY)
		return (1);
	heap->scheduler = scheduler;
	return (0);
}

void	heap_destroy(t_request_heap *heap)
{
	if (heap == NULL)
		return ;
	while (heap->root != NULL)
		heap_pop(heap);
	memset(heap, 0, sizeof(*heap));
}

t_heap_node	*heap_new_node(t_request_heap *heap, t_request *request)
{
	t_heap_node	*node;

	node = malloc(sizeof(*node));
	if (node == NULL)
		return (NULL);
	memset(node, 0, sizeof(*node));
	node->entry = (t_heap_entry){request, request->deadline,
		heap->next_arrival++};
	return (node);
}

/* Positions start at 1: root=1, left child=2*p, right child=2*p+1. */
t_heap_node	*heap_node_at(t_request_heap *heap, size_t position)
{
	t_heap_node	*parent;

	if (heap == NULL || position == 0)
		return (NULL);
	if (position == 1)
		return (heap->root);
	parent = heap_node_at(heap, position / 2);
	if (parent == NULL)
		return (NULL);
	if (position % 2 == 0)
		return (parent->left);
	return (parent->right);
}

/* Preorder traversal: node, left subtree, then right subtree. */
t_heap_node	*heap_next_node(t_heap_node *node)
{
	if (node == NULL)
		return (NULL);
	if (node->left != NULL)
		return (node->left);
	if (node->right != NULL)
		return (node->right);
	while (node->parent != NULL)
	{
		if (node == node->parent->left && node->parent->right != NULL)
			return (node->parent->right);
		node = node->parent;
	}
	return (NULL);
}
