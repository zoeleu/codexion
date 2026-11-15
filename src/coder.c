/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Sleeps until the phase ends, the coder burns out or the run stops.
   Returns 1 while the coder may go on. */
int	coder_wait(t_coder *coder, long milliseconds)
{
	t_simulation	*sim;
	struct timespec	end;
	int				active;

	sim = coder->sim;
	pthread_mutex_lock(&sim->mutex);
	end = time_add_ms(time_now(), milliseconds);
	if (time_reached(end, coder->burnout_time))
		end = coder->burnout_time;
	while (sim->result == SIM_RUNNING && !time_reached(time_now(), end))
		pthread_cond_timedwait(&sim->cond, &sim->mutex, &end);
	active = (sim->result == SIM_RUNNING
			&& !time_reached(time_now(), coder->burnout_time));
	pthread_mutex_unlock(&sim->mutex);
	return (active);
}

/* Logs the new phase, then waits for it to end. */
static int	coder_phase(t_coder *coder, const char *message, long duration)
{
	pthread_mutex_lock(&coder->sim->mutex);
	if (!time_reached(time_now(), coder->burnout_time))
		coder_log(coder, message);
	pthread_mutex_unlock(&coder->sim->mutex);
	return (coder_wait(coder, duration));
}

/* Takes both dongles, compiles, then puts them back. "is debugging" is
   logged before the release, so no neighbour can be seen compiling first.
   Returns 1 when the compile finished. */
static int	coder_compile(t_coder *coder)
{
	int	finished;

	finished = 0;
	if (dongle_acquire(coder)
		&& coder_wait(coder, coder->sim->config.time_to_compile))
		finished = coder_phase(coder, "is debugging", 0);
	dongle_release(coder);
	return (finished);
}

/* Counts one finished compile. Returns 1 while more are required. */
static int	coder_needs_more(t_coder *coder)
{
	int	more;

	pthread_mutex_lock(&coder->sim->mutex);
	coder->compiles++;
	more = (coder->compiles < coder->sim->config.number_of_compiles_required);
	pthread_mutex_unlock(&coder->sim->mutex);
	return (more);
}

/* Odd seats start half a compile late so neighbours take disjoint pairs.
   A coder that met its quota retires without debugging or refactoring. */
void	*coder_routine(void *arg)
{
	t_coder		*coder;
	t_config	*config;

	coder = arg;
	config = &coder->sim->config;
	if (coder->index % 2 == 1)
		coder_wait(coder, config->time_to_compile / 2);
	while (coder_compile(coder))
	{
		if (!coder_needs_more(coder))
			break ;
		if (!coder_wait(coder, config->time_to_debug))
			break ;
		if (!coder_phase(coder, "is refactoring", config->time_to_refactor))
			break ;
	}
	return (NULL);
}
