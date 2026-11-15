/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static const char	*log_message(t_log_kind kind)
{
	if (kind == LOG_DONGLE)
		return ("has taken a dongle");
	if (kind == LOG_COMPILE)
		return ("is compiling");
	if (kind == LOG_DEBUG)
		return ("is debugging");
	if (kind == LOG_REFACTOR)
		return ("is refactoring");
	return ("burned out");
}

static size_t	log_number(char *line, unsigned long long number)
{
	char	digits[20];
	size_t	count;
	size_t	length;

	count = 0;
	while (1)
	{
		digits[count++] = '0' + number % 10;
		number /= 10;
		if (!number)
			break ;
	}
	length = count;
	while (count)
		*line++ = digits[--count];
	return (length);
}

/* Unbuffered output keeps burnout messages visible even when piped. */
static int	log_print(t_log_event *event)
{
	char		line[96];
	const char	*message;
	size_t		length;
	size_t		sent;
	ssize_t		written;

	length = log_number(line, event->milliseconds);
	line[length++] = ' ';
	length += log_number(line + length, event->coder_number);
	line[length++] = ' ';
	message = log_message(event->kind);
	while (*message)
		line[length++] = *message++;
	line[length++] = '\n';
	sent = 0;
	while (sent < length)
	{
		written = write(STDOUT_FILENO, line + sent, length - sent);
		if (written <= 0)
			return (1);
		sent += written;
	}
	return (0);
}

void	*log_worker(void *arg)
{
	t_simulation	*sim;
	t_log_event		*event;
	int				error;

	sim = arg;
	pthread_mutex_lock(&sim->state_mutex);
	while (1)
	{
		while (sim->log_head == NULL && !sim->logs_closed)
			pthread_cond_wait(&sim->log_condition, &sim->state_mutex);
		if (sim->log_head == NULL && sim->logs_closed)
			break ;
		event = sim->log_head;
		sim->log_head = event->next;
		if (sim->log_head == NULL)
			sim->log_tail = NULL;
		pthread_mutex_unlock(&sim->state_mutex);
		error = log_print(event);
		free(event);
		pthread_mutex_lock(&sim->state_mutex);
		if (error)
			sim->result = SIM_ERROR;
	}
	pthread_mutex_unlock(&sim->state_mutex);
	return (NULL);
}
