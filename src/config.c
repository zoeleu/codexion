/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/11 20:52:14 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	parse_number(const char *arg, long limit, long *value)
{
	long	number;
	int		digit;

	if (!*arg)
		return (-1);
	number = 0;
	while (*arg)
	{
		if (*arg < '0' || *arg > '9')
			return (-1);
		digit = *arg - '0';
		if (number > (limit - digit) / 10)
			return (-1);
		number = number * 10 + digit;
		arg++;
	}
	*value = number;
	return (0);
}

static int	parse_numbers(char **argv, long *values)
{
	long	limit;
	int		i;

	i = 0;
	while (i < 7)
	{
		limit = LONG_MAX;
		if (i == 0 || i == 5)
			limit = INT_MAX;
		if (parse_number(argv[i + 1], limit, &values[i]) != 0)
			return (-1);
		if (values[i] == 0 && i != 6)
			return (-1);
		i++;
	}
	return (0);
}

static void	fill_config(t_config *config, long *values, char *scheduler)
{
	config->number_of_coders = (int)values[0];
	config->time_to_burnout = values[1];
	config->time_to_compile = values[2];
	config->time_to_debug = values[3];
	config->time_to_refactor = values[4];
	config->number_of_compiles_required = (int)values[5];
	config->dongle_cooldown = values[6];
	config->scheduler = SCHED_FIFO_POLICY;
	if (strcmp(scheduler, "edf") == 0)
		config->scheduler = SCHED_EDF_POLICY;
}

int	parse_args(t_config *config, int argc, char **argv)
{
	long	values[7];

	if (argc != 9)
	{
		fprintf(stderr, "codexion: expected eight arguments\n");
		return (-1);
	}
	if (parse_numbers(argv, values) != 0)
	{
		fprintf(stderr, "codexion: invalid numeric argument\n");
		return (-1);
	}
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
	{
		fprintf(stderr, "codexion: scheduler must be fifo or edf\n");
		return (-1);
	}
	fill_config(config, values, argv[8]);
	return (0);
}
