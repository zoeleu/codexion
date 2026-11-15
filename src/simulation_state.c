/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation_state.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	simulation_state_init(t_simulation *sim)
{
	if (pthread_mutex_init(&sim->state_mutex, NULL))
		return (1);
	if (pthread_cond_init(&sim->start_condition, NULL))
	{
		pthread_mutex_destroy(&sim->state_mutex);
		return (1);
	}
	if (pthread_cond_init(&sim->phase_condition, NULL))
	{
		pthread_cond_destroy(&sim->start_condition);
		pthread_mutex_destroy(&sim->state_mutex);
		return (1);
	}
	return (0);
}

void	simulation_state_destroy(t_simulation *sim)
{
	pthread_cond_destroy(&sim->phase_condition);
	pthread_cond_destroy(&sim->start_condition);
	pthread_mutex_destroy(&sim->state_mutex);
}

/* Let alternating seats acquire disjoint pairs before their neighbors. */
void	simulation_start_coder(t_coder *coder)
{
	simulation_wait_start(coder->simulation);
	if (coder->index % 2 != 0)
		simulation_wait_phase(coder,
			coder->simulation->config.time_to_compile / 2);
}

/* Release state_mutex while waiting; shutdown wakes all phase waits. */
int	simulation_wait_phase(t_coder *coder, long milliseconds)
{
	t_simulation	*sim;
	struct timespec	end;
	int				error;
	int				active;

	sim = coder->simulation;
	end = time_add_ms(time_now(), milliseconds);
	pthread_mutex_lock(&sim->state_mutex);
	if (time_reached(end, coder->burnout_time))
		end = coder->burnout_time;
	while (sim->result == SIM_RUNNING && !time_reached(time_now(), end))
	{
		error = pthread_cond_timedwait(&sim->phase_condition,
				&sim->state_mutex, &end);
		if (error != 0 && error != ETIMEDOUT)
		{
			sim->result = SIM_ERROR;
			break ;
		}
	}
	active = (sim->result == SIM_RUNNING
			&& !time_reached(time_now(), coder->burnout_time));
	pthread_mutex_unlock(&sim->state_mutex);
	return (active);
}
