/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

struct timespec	time_now(void)
{
	struct timespec	now;

	clock_gettime(CLOCK_REALTIME, &now);
	return (now);
}

/* Milliseconds must be nonnegative and the result must fit in time_t. */
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

int	time_reached(struct timespec now, struct timespec deadline)
{
	return (now.tv_sec > deadline.tv_sec
		|| (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec));
}

long long	time_elapsed_ms(struct timespec start, struct timespec end)
{
	long long	seconds;
	long		nanoseconds;

	if (!time_reached(end, start))
		return (0);
	seconds = end.tv_sec - start.tv_sec;
	nanoseconds = end.tv_nsec - start.tv_nsec;
	if (nanoseconds < 0)
	{
		seconds--;
		nanoseconds += 1000000000L;
	}
	return (seconds * 1000 + nanoseconds / 1000000L);
}
