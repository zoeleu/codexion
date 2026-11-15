/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* A free dongle still cooling down counts as busy; its cooldown end
   shortens the wake time so expiry needs no wakeup signal. */
static int	dongle_busy(t_dongle *dongle, struct timespec now,
		struct timespec *wake)
{
	struct timespec	cooldown;
	int				busy;

	pthread_mutex_lock(&dongle->mutex);
	busy = dongle->in_use;
	cooldown = dongle->cooldown_until;
	pthread_mutex_unlock(&dongle->mutex);
	if (!busy && !time_reached(now, cooldown))
	{
		busy = 1;
		if (!time_reached(cooldown, *wake))
			*wake = cooldown;
	}
	return (busy);
}

static void	dongle_take(t_coder *coder, t_dongle *dongle, int slot)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 1;
	pthread_mutex_unlock(&dongle->mutex);
	coder->taken[slot] = dongle;
	coder_log(coder, "has taken a dongle");
}

/* Caller holds sim->mutex. Returns 1 when compiling starts, 0 to wait until
   *wake, -1 on burnout or shutdown. A lone coder takes its only dongle,
   then waits for a second one until it burns out. */
static int	dongle_try_grant(t_coder *coder, t_request *request,
		struct timespec *wake)
{
	struct timespec	now;
	int				busy;

	now = time_now();
	*wake = request->deadline;
	if (coder->sim->result != SIM_RUNNING
		|| time_reached(now, request->deadline))
		return (-1);
	if (heap_peek(&coder->left->requests) != request
		|| heap_peek(&coder->right->requests) != request)
		return (0);
	busy = dongle_busy(coder->left, now, wake);
	if (dongle_busy(coder->right, now, wake) || busy)
		return (0);
	dongle_take(coder, coder->left, 0);
	if (coder->left == coder->right)
		return (0);
	dongle_take(coder, coder->right, 1);
	coder->burnout_time = time_add_ms(now, coder->sim->config.time_to_burnout);
	coder_log(coder, "is compiling");
	return (1);
}

/* Queues one request in both heaps and takes both dongles at once when it
   heads both. Nothing is held while waiting, so there is no hold-and-wait.
   Returns 1 once compiling, 0 on burnout or shutdown. */
int	dongle_acquire(t_coder *coder)
{
	t_simulation	*sim;
	t_request		request;
	struct timespec	wake;
	int				status;

	sim = coder->sim;
	pthread_mutex_lock(&sim->mutex);
	request = (t_request){coder, coder->burnout_time};
	status = -1;
	if (heap_push(&coder->left->requests, &request)
		|| (coder->right != coder->left
			&& heap_push(&coder->right->requests, &request)))
		simulation_stop(sim, SIM_ERROR);
	else
		status = dongle_try_grant(coder, &request, &wake);
	while (status == 0)
	{
		pthread_cond_timedwait(&sim->cond, &sim->mutex, &wake);
		status = dongle_try_grant(coder, &request, &wake);
	}
	heap_remove(&coder->left->requests, &request);
	heap_remove(&coder->right->requests, &request);
	pthread_cond_broadcast(&sim->cond);
	pthread_mutex_unlock(&sim->mutex);
	return (status == 1);
}

/* Never holds a dongle mutex while taking sim->mutex. */
void	dongle_release(t_coder *coder)
{
	t_dongle	*dongle;
	int			i;

	i = -1;
	while (++i < 2)
	{
		dongle = coder->taken[i];
		if (dongle == NULL)
			continue ;
		pthread_mutex_lock(&dongle->mutex);
		dongle->in_use = 0;
		dongle->cooldown_until = time_add_ms(time_now(),
				coder->sim->config.dongle_cooldown);
		pthread_mutex_unlock(&dongle->mutex);
		coder->taken[i] = NULL;
	}
	pthread_mutex_lock(&coder->sim->mutex);
	pthread_cond_broadcast(&coder->sim->cond);
	pthread_mutex_unlock(&coder->sim->mutex);
}
