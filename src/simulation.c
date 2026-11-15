/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Only successfully initialized primitives are destroyed on failure. */
static void	simulation_destroy(t_simulation *sim)
{
	int	i;

	i = -1;
	while (++i < sim->created_threads)
		pthread_join(sim->threads[i], NULL);
	log_finish(sim);
	i = 0;
	while (i < sim->initialized_dongles)
	{
		heap_destroy(&sim->dongles[i].requests);
		pthread_mutex_destroy(&sim->dongles[i].dongle_taken);
		i++;
	}
	free(sim->threads);
	free(sim->workers);
	free(sim->dongles);
	free(sim->coders);
	simulation_state_destroy(sim);
}

static int	simulation_allocate(t_simulation *sim)
{
	size_t	n;

	n = sim->config.number_of_coders;
	if (n > (size_t)-1 / sizeof(t_coder)
		|| n > (size_t)-1 / sizeof(t_dongle)
		|| n > (size_t)-1 / sizeof(t_coder_worker)
		|| n > (size_t)-1 / sizeof(pthread_t))
		return (1);
	sim->coders = malloc(n * sizeof(t_coder));
	sim->dongles = malloc(n * sizeof(t_dongle));
	sim->workers = malloc(n * sizeof(t_coder_worker));
	sim->threads = malloc(n * sizeof(pthread_t));
	if (!sim->coders || !sim->dongles || !sim->workers || !sim->threads)
		return (1);
	memset(sim->coders, 0, n * sizeof(t_coder));
	memset(sim->dongles, 0, n * sizeof(t_dongle));
	return (0);
}

static int	simulation_initialize(t_simulation *sim)
{
	int	i;

	if (simulation_allocate(sim))
		return (1);
	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (dongle_initialize(&sim->dongles[i], sim->config.scheduler))
			return (1);
		sim->initialized_dongles++;
		sim->coders[i].simulation = sim;
		sim->coders[i].index = i;
		sim->workers[i] = (t_coder_worker){&sim->coders[i],
			&sim->config, sim->dongles, i};
		i++;
	}
	return (0);
}

/* Workers remain behind the start gate until all threads exist. */
static int	simulation_launch(t_simulation *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->threads[i], NULL,
				coder_worker, &sim->workers[i]))
			return (1);
		sim->created_threads++;
		i++;
	}
	if (pthread_create(&sim->monitor, NULL, monitor_worker, sim))
		return (1);
	pthread_mutex_lock(&sim->state_mutex);
	sim->start_time = time_now();
	i = -1;
	while (++i < sim->config.number_of_coders)
		sim->coders[i].burnout_time = time_add_ms(sim->start_time,
				sim->config.time_to_burnout);
	sim->started = 1;
	pthread_cond_broadcast(&sim->start_condition);
	pthread_mutex_unlock(&sim->state_mutex);
	pthread_join(sim->monitor, NULL);
	return (0);
}

int	simulation_run(const t_config *config)
{
	t_simulation	sim;
	int				error;

	if (config->number_of_compiles_required == 0)
		return (0);
	memset(&sim, 0, sizeof(sim));
	sim.config = *config;
	sim.burned_out_index = -1;
	if (simulation_state_init(&sim))
		return (1);
	error = simulation_initialize(&sim);
	if (!error)
		error = log_initialize(&sim);
	if (!error)
		error = simulation_launch(&sim);
	if (error)
		simulation_stop(&sim, SIM_ERROR);
	simulation_destroy(&sim);
	error = (error || sim.result == SIM_ERROR);
	if (error)
		fprintf(stderr, "codexion: fatal error\n");
	return (error);
}
