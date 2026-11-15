/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log_queue.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	log_initialize(t_simulation *sim)
{
	if (pthread_cond_init(&sim->log_condition, NULL))
		return (1);
	if (pthread_create(&sim->logger, NULL, log_worker, sim))
	{
		pthread_cond_destroy(&sim->log_condition);
		return (1);
	}
	sim->logger_started = 1;
	return (0);
}

/* This is called after all producers (workers and monitor) have joined. */
void	log_finish(t_simulation *sim)
{
	if (!sim->logger_started)
		return ;
	pthread_mutex_lock(&sim->state_mutex);
	sim->logs_closed = 1;
	pthread_cond_signal(&sim->log_condition);
	pthread_mutex_unlock(&sim->state_mutex);
	pthread_join(sim->logger, NULL);
	pthread_cond_destroy(&sim->log_condition);
	sim->logger_started = 0;
}

int	log_event_locked(t_simulation *sim, int index, t_log_kind kind)
{
	t_log_event	*event;

	if (sim->result != SIM_RUNNING || sim->logs_closed)
		return (0);
	event = malloc(sizeof(*event));
	if (event == NULL)
	{
		sim->result = SIM_ERROR;
		return (1);
	}
	event->milliseconds = time_elapsed_ms(sim->start_time, time_now());
	event->coder_number = index + 1;
	event->kind = kind;
	event->next = NULL;
	if (sim->log_tail != NULL)
		sim->log_tail->next = event;
	else
		sim->log_head = event;
	sim->log_tail = event;
	pthread_cond_signal(&sim->log_condition);
	return (0);
}
