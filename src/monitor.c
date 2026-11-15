/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Caller holds sim->mutex. Wakes every waiting thread so it can exit. */
void	simulation_stop(t_simulation *sim, t_result result)
{
	if (sim->result == SIM_RUNNING)
		sim->result = result;
	pthread_cond_broadcast(&sim->cond);
}

/* Caller holds sim->mutex. Coders that met their quota have retired,
   so only the others can burn out. */
static t_result	monitor_check(t_simulation *sim)
{
	struct timespec	now;
	int				completed;
	int				i;

	if (sim->result != SIM_RUNNING)
		return (sim->result);
	now = time_now();
	completed = 0;
	i = -1;
	while (++i < sim->config.number_of_coders)
	{
		if (sim->coders[i].compiles >= sim->config.number_of_compiles_required)
			completed++;
		else if (time_reached(now, sim->coders[i].burnout_time))
		{
			coder_log(&sim->coders[i], "burned out");
			return (SIM_BURNOUT);
		}
	}
	if (completed == sim->config.number_of_coders)
		return (SIM_SUCCESS);
	return (SIM_RUNNING);
}

/* Checks every millisecond. The burnout line and the stop happen under
   the same lock, so no other message can be printed after it. */
void	*monitor_routine(void *arg)
{
	t_simulation	*sim;
	t_result		result;

	sim = arg;
	result = SIM_RUNNING;
	while (result == SIM_RUNNING)
	{
		usleep(1000);
		pthread_mutex_lock(&sim->mutex);
		result = monitor_check(sim);
		if (result != SIM_RUNNING)
			simulation_stop(sim, result);
		pthread_mutex_unlock(&sim->mutex);
	}
	return (NULL);
}
