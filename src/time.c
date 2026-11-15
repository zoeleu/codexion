/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

struct timespec	time_now(void)
{
	struct timespec	now;

	clock_gettime(CLOCK_REALTIME, &now);
	return (now);
}

struct timespec	time_add_ms(struct timespec time, long milliseconds)
{
	time.tv_sec += milliseconds / 1000;
	time.tv_nsec += (milliseconds % 1000) * 1000000L;
	if (time.tv_nsec >= 1000000000L)
	{
		time.tv_sec++;
		time.tv_nsec -= 1000000000L;
	}
	return (time);
}

/* Returns 1 once now is at or after deadline. */
int	time_reached(struct timespec now, struct timespec deadline)
{
	if (now.tv_sec != deadline.tv_sec)
		return (now.tv_sec > deadline.tv_sec);
	return (now.tv_nsec >= deadline.tv_nsec);
}

long long	time_elapsed_ms(struct timespec start, struct timespec end)
{
	long long	start_ms;
	long long	end_ms;

	start_ms = start.tv_sec * 1000LL + start.tv_nsec / 1000000;
	end_ms = end.tv_sec * 1000LL + end.tv_nsec / 1000000;
	return (end_ms - start_ms);
}
