/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_queue.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	dongle_initialize(t_dongle *dongle, t_scheduler scheduler)
{
	memset(dongle, 0, sizeof(*dongle));
	if (pthread_mutex_init(&dongle->dongle_taken, NULL))
		return (1);
	if (heap_init(&dongle->requests, scheduler))
	{
		pthread_mutex_destroy(&dongle->dongle_taken);
		return (1);
	}
	return (0);
}

/* Caller holds state_mutex. One critical section enqueues the request in
   both heaps, keeping every dongle's arrival order globally consistent. */
int	dongle_enqueue_pair(t_dongle *a, t_dongle *b, t_request *request)
{
	t_simulation	*sim;

	sim = request->coder->simulation;
	request->deadline = request->coder->burnout_time;
	if (sim->result != SIM_RUNNING
		|| time_reached(time_now(), request->deadline))
		return (0);
	if (heap_push(&a->requests, request)
		|| (a != b && heap_push(&b->requests, request)))
	{
		heap_remove(&a->requests, request);
		heap_remove(&b->requests, request);
		sim->result = SIM_ERROR;
		return (0);
	}
	return (1);
}

static void	dongle_put(t_dongle *dongle, long cooldown)
{
	pthread_mutex_lock(&dongle->dongle_taken);
	dongle->in_use = 0;
	dongle->cooldown_until = time_add_ms(time_now(), cooldown);
	pthread_mutex_unlock(&dongle->dongle_taken);
}

/* Releasing flips the dongles first, then takes the scheduling lock only
   for the single wakeup broadcast; waiters recheck and retime their wait. */
void	dongle_release_pair(t_coder *coder)
{
	t_simulation	*sim;
	int				i;

	sim = coder->simulation;
	i = 0;
	while (i < 2)
	{
		if (coder->taken_dongles[i] != NULL)
			dongle_put(coder->taken_dongles[i],
				sim->config.dongle_cooldown);
		coder->taken_dongles[i] = NULL;
		i++;
	}
	pthread_mutex_lock(&sim->state_mutex);
	pthread_cond_broadcast(&sim->phase_condition);
	pthread_mutex_unlock(&sim->state_mutex);
}
