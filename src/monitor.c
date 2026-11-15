/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	simulation_wait_start(t_simulation *sim)
{
	pthread_mutex_lock(&sim->state_mutex);
	while (!sim->started && sim->result == SIM_RUNNING)
		pthread_cond_wait(&sim->start_condition, &sim->state_mutex);
	pthread_mutex_unlock(&sim->state_mutex);
}

int	simulation_stopped(t_simulation *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->state_mutex);
	stopped = (sim->result != SIM_RUNNING);
	pthread_mutex_unlock(&sim->state_mutex);
	return (stopped);
}

/* Phase and dongle waits share phase_condition; one broadcast wakes all. */
void	simulation_stop(t_simulation *sim, t_simulation_result result)
{
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->result == SIM_RUNNING)
		sim->result = result;
	pthread_cond_broadcast(&sim->start_condition);
	pthread_cond_broadcast(&sim->phase_condition);
	pthread_mutex_unlock(&sim->state_mutex);
}

/* Called under state_mutex; completed coders have retired from the cycle. */
static t_simulation_result	monitor_check(t_simulation *sim)
{
	int				i;
	int				completed;
	struct timespec	now;

	i = 0;
	completed = 0;
	now = time_now();
	while (i < sim->config.number_of_coders)
	{
		if (sim->coders[i].number_of_compiles
			>= (unsigned int)sim->config.number_of_compiles_required)
			completed++;
		else if (time_reached(now, sim->coders[i].burnout_time))
		{
			sim->coders[i].state = BURNED_OUT;
			sim->burned_out_index = i;
			if (log_event_locked(sim, i, LOG_BURNOUT))
				return (SIM_ERROR);
			return (SIM_BURNOUT);
		}
		i++;
	}
	if (completed == sim->config.number_of_coders)
		return (SIM_SUCCESS);
	return (SIM_RUNNING);
}

void	*monitor_worker(void *arg)
{
	t_simulation		*sim;
	t_simulation_result	result;

	sim = arg;
	simulation_wait_start(sim);
	pthread_mutex_lock(&sim->state_mutex);
	while (sim->result == SIM_RUNNING)
	{
		sim->result = monitor_check(sim);
		if (sim->result != SIM_RUNNING)
			break ;
		pthread_mutex_unlock(&sim->state_mutex);
		usleep(1000);
		pthread_mutex_lock(&sim->state_mutex);
	}
	result = sim->result;
	pthread_mutex_unlock(&sim->state_mutex);
	simulation_stop(sim, result);
	return (NULL);
}
