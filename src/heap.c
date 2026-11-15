/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:36:41 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_push(t_request_heap *heap, t_request *request)
{
	t_heap_node	*node;

	if (heap == NULL || request == NULL || heap->size == (size_t)-1)
		return (1);
	if (heap->size == 0)
		heap->next_arrival = 0;
	if (heap->next_arrival == ULLONG_MAX)
		return (1);
	node = heap_new_node(heap, request);
	if (node == NULL)
		return (1);
	if (heap->root == NULL)
		heap->root = node;
	else
	{
		node->parent = heap_node_at(heap, (heap->size + 1) / 2);
		if (node->parent->left == NULL)
			node->parent->left = node;
		else
			node->parent->right = node;
	}
	heap->size++;
	heap_repair(heap, node);
	return (0);
}

t_request	*heap_peek(const t_request_heap *heap)
{
	if (heap == NULL || heap->root == NULL)
		return (NULL);
	return (heap->root->entry.request);
}

static void	heap_delete_node(t_request_heap *heap, t_heap_node *node)
{
	t_heap_node	*last;

	last = heap_node_at(heap, heap->size);
	if (last->parent == NULL)
		heap->root = NULL;
	else if (last->parent->left == last)
		last->parent->left = NULL;
	else
		last->parent->right = NULL;
	heap->size--;
	if (node != last)
	{
		node->entry = last->entry;
		heap_repair(heap, node);
	}
	free(last);
}

t_request	*heap_pop(t_request_heap *heap)
{
	t_request	*request;

	request = heap_peek(heap);
	if (request != NULL)
		heap_delete_node(heap, heap->root);
	return (request);
}

void	heap_remove(t_request_heap *heap, t_request *request)
{
	t_heap_node	*node;

	if (heap == NULL)
		return ;
	node = heap->root;
	while (node != NULL && node->entry.request != request)
		node = heap_next_node(node);
	if (node != NULL)
		heap_delete_node(heap, node);
}
