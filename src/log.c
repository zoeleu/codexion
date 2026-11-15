/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Caller holds sim->mutex, which also keeps lines from interleaving.
   Nothing is printed once the simulation has stopped. */
void	coder_log(t_coder *coder, const char *message)
{
	t_simulation	*sim;
	long long		timestamp;

	sim = coder->sim;
	if (sim->result != SIM_RUNNING)
		return ;
	timestamp = time_elapsed_ms(sim->start_time, time_now());
	printf("%lld %d %s\n", timestamp, coder->index + 1, message);
}
