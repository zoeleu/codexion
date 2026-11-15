/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Creates the dongles and seats coder i between dongles i and i + 1.
   With a single coder, left and right are the same dongle. */
static int	simulation_init(t_simulation *sim)
{
	int	n;
	int	i;

	n = sim->config.number_of_coders;
	sim->coders = malloc(n * sizeof(t_coder));
	sim->dongles = malloc(n * sizeof(t_dongle));
	if (sim->coders == NULL || sim->dongles == NULL)
		return (1);
	memset(sim->coders, 0, n * sizeof(t_coder));
	memset(sim->dongles, 0, n * sizeof(t_dongle));
	i = -1;
	while (++i < n)
	{
		if (heap_init(&sim->dongles[i].requests, sim->config.scheduler)
			|| pthread_mutex_init(&sim->dongles[i].mutex, NULL))
			return (1);
		sim->ready_dongles++;
		sim->coders[i].sim = sim;
		sim->coders[i].index = i;
		sim->coders[i].left = &sim->dongles[i];
		sim->coders[i].right = &sim->dongles[(i + 1) % n];
	}
	return (0);
}

/* Caller holds sim->mutex. Every coder starts with a full burnout delay. */
static void	simulation_start_clock(t_simulation *sim)
{
	int	i;

	sim->start_time = time_now();
	i = -1;
	while (++i < sim->config.number_of_coders)
		sim->coders[i].burnout_time = time_add_ms(sim->start_time,
				sim->config.time_to_burnout);
}

/* sim->mutex is held while the threads are created, so they all block on
   it until the clock starts. That lock is the whole start gate. */
static int	simulation_launch(t_simulation *sim)
{
	pthread_t	monitor;
	int			created;
	int			error;

	pthread_mutex_lock(&sim->mutex);
	created = 0;
	while (created < sim->config.number_of_coders
		&& pthread_create(&sim->coders[created].thread, NULL,
			coder_routine, &sim->coders[created]) == 0)
		created++;
	error = (created < sim->config.number_of_coders
			|| pthread_create(&monitor, NULL, monitor_routine, sim) != 0);
	simulation_start_clock(sim);
	if (error)
		simulation_stop(sim, SIM_ERROR);
	pthread_mutex_unlock(&sim->mutex);
	if (!error)
		pthread_join(monitor, NULL);
	while (created > 0)
		pthread_join(sim->coders[--created].thread, NULL);
	return (error);
}

static void	simulation_cleanup(t_simulation *sim)
{
	int	i;

	i = -1;
	while (++i < sim->ready_dongles)
	{
		heap_destroy(&sim->dongles[i].requests);
		pthread_mutex_destroy(&sim->dongles[i].mutex);
	}
	free(sim->dongles);
	free(sim->coders);
	pthread_cond_destroy(&sim->cond);
	pthread_mutex_destroy(&sim->mutex);
}

/* A quota of zero is already met: nothing runs and nothing is printed. */
int	simulation_run(const t_config *config)
{
	t_simulation	sim;
	int				error;

	if (config->number_of_compiles_required == 0)
		return (0);
	memset(&sim, 0, sizeof(sim));
	sim.config = *config;
	if (pthread_mutex_init(&sim.mutex, NULL) != 0)
		return (1);
	if (pthread_cond_init(&sim.cond, NULL) != 0)
	{
		pthread_mutex_destroy(&sim.mutex);
		return (1);
	}
	error = simulation_init(&sim);
	if (!error)
		error = simulation_launch(&sim);
	simulation_cleanup(&sim);
	if (error || sim.result == SIM_ERROR)
	{
		fprintf(stderr, "codexion: fatal error\n");
		return (1);
	}
	return (0);
}
