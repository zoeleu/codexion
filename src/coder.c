/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 03:55:18 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	coder_phase(t_coder *coder, t_coder_state state, long duration)
{
	t_simulation	*sim;

	sim = coder->simulation;
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->result == SIM_RUNNING
		&& !time_reached(time_now(), coder->burnout_time))
	{
		if (state == COMPILING)
			coder->burnout_time = time_add_ms(time_now(),
					sim->config.time_to_burnout);
		coder->state = state;
		if (state != WAITING)
			log_event_locked(sim, coder->index, (t_log_kind)state);
	}
	pthread_mutex_unlock(&sim->state_mutex);
	return (simulation_wait_phase(coder, duration));
}

static int	coder_compile(t_coder_worker *worker)
{
	t_coder	*coder;
	int		next;
	int		active;

	coder = worker->coder;
	next = (worker->index + 1) % worker->config->number_of_coders;
	coder_phase(coder, WAITING, 0);
	active = dongle_acquire(coder, &worker->dongles[worker->index],
			&worker->dongles[next])
		&& coder_phase(coder, COMPILING, worker->config->time_to_compile);
	if (active)
		coder_phase(coder, DEBUGGING, 0);
	dongle_release_pair(coder);
	return (active);
}

void	*coder_worker(void *arg)
{
	t_coder_worker	*worker;
	t_simulation	*sim;
	unsigned int	completed;

	worker = arg;
	sim = worker->coder->simulation;
	simulation_start_coder(worker->coder);
	completed = 0;
	while (completed < (unsigned int)sim->config.number_of_compiles_required)
	{
		if (!coder_compile(worker))
			break ;
		pthread_mutex_lock(&sim->state_mutex);
		worker->coder->number_of_compiles = ++completed;
		pthread_mutex_unlock(&sim->state_mutex);
		if (completed < (unsigned int)sim->config.number_of_compiles_required
			&& (!simulation_wait_phase(worker->coder,
					sim->config.time_to_debug)
				|| !coder_phase(worker->coder, REFACTORING,
					sim->config.time_to_refactor)))
			break ;
	}
	return (NULL);
}
