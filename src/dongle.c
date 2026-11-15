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

/* Lowers wake to a pending cooldown so expiry needs no wakeup signal. */
static int	dongle_busy(t_dongle *dongle, struct timespec now,
		struct timespec *wake)
{
	struct timespec	cooldown;
	int				busy;

	pthread_mutex_lock(&dongle->dongle_taken);
	busy = dongle->in_use;
	cooldown = dongle->cooldown_until;
	pthread_mutex_unlock(&dongle->dongle_taken);
	if (!busy && !time_reached(now, cooldown))
	{
		busy = 1;
		if (!time_reached(cooldown, *wake))
			*wake = cooldown;
	}
	return (busy);
}

/* Caller holds state_mutex and the coder's request heads this heap. */
static void	dongle_take(t_dongle *dongle, t_coder *coder, int slot)
{
	pthread_mutex_lock(&dongle->dongle_taken);
	dongle->in_use = 1;
	pthread_mutex_unlock(&dongle->dongle_taken);
	heap_pop(&dongle->requests);
	coder->taken_dongles[slot] = dongle;
	log_event_locked(coder->simulation, coder->index, LOG_DONGLE);
}

/* Returns 1 granted, 0 keep waiting, -1 abort; wake bounds the wait.
   A lone coder takes its only dongle, then waits for a second forever. */
static int	dongle_try_grant(t_dongle *a, t_dongle *b, t_request *request,
		struct timespec *wake)
{
	struct timespec	now;
	t_coder			*coder;
	int				busy;

	coder = request->coder;
	now = time_now();
	*wake = request->deadline;
	if (coder->simulation->result != SIM_RUNNING
		|| time_reached(now, request->deadline))
		return (-1);
	if (heap_peek(&a->requests) != request
		|| heap_peek(&b->requests) != request)
		return (0);
	busy = dongle_busy(a, now, wake);
	if (dongle_busy(b, now, wake) || busy)
		return (0);
	dongle_take(a, coder, 0);
	if (a == b)
		return (0);
	dongle_take(b, coder, 1);
	return (1);
}

/* Caller holds state_mutex. Returns 1 on wait error (stops the run). */
static int	dongle_sleep(t_simulation *sim, const struct timespec *wake)
{
	int	error;

	error = pthread_cond_timedwait(&sim->phase_condition,
			&sim->state_mutex, wake);
	if (error == 0 || error == ETIMEDOUT)
		return (0);
	sim->result = SIM_ERROR;
	return (1);
}

/* Takes both dongles atomically: nothing is held while waiting, so the
   earliest-deadline waiter always heads both heaps and cannot starve.
   Returns 1 when both dongles are held, 0 on burnout or shutdown. */
int	dongle_acquire(t_coder *coder, t_dongle *a, t_dongle *b)
{
	t_simulation	*sim;
	t_request		request;
	struct timespec	wake;
	int				status;

	sim = coder->simulation;
	request.coder = coder;
	status = -1;
	pthread_mutex_lock(&sim->state_mutex);
	if (dongle_enqueue_pair(a, b, &request))
	{
		status = dongle_try_grant(a, b, &request, &wake);
		while (status == 0 && !dongle_sleep(sim, &wake))
			status = dongle_try_grant(a, b, &request, &wake);
		heap_remove(&a->requests, &request);
		heap_remove(&b->requests, &request);
		pthread_cond_broadcast(&sim->phase_condition);
	}
	pthread_mutex_unlock(&sim->state_mutex);
	return (status == 1);
}
