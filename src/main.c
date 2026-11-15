/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Decimal digits only, at most limit. */
static int	parse_number(const char *arg, long limit, long *value)
{
	long	number;

	if (!*arg)
		return (1);
	number = 0;
	while (*arg >= '0' && *arg <= '9')
	{
		if (number > (limit - (*arg - '0')) / 10)
			return (1);
		number = number * 10 + (*arg++ - '0');
	}
	*value = number;
	return (*arg != '\0');
}

/* Counts fit in an int; the quota (5) and the cooldown (6) may be zero. */
static int	parse_numbers(char **argv, long *values)
{
	long	limit;
	int		i;

	i = -1;
	while (++i < 7)
	{
		limit = LONG_MAX;
		if (i == 0 || i == 5)
			limit = INT_MAX;
		if (parse_number(argv[i + 1], limit, &values[i]))
			return (1);
		if (values[i] == 0 && i != 5 && i != 6)
			return (1);
	}
	return (0);
}

static int	parse_error(const char *message)
{
	fprintf(stderr, "codexion: %s\n", message);
	return (1);
}

int	parse_args(t_config *config, int argc, char **argv)
{
	long	v[7];

	if (argc != 9)
		return (parse_error("expected eight arguments"));
	if (parse_numbers(argv, v))
		return (parse_error("invalid numeric argument"));
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
		return (parse_error("scheduler must be fifo or edf"));
	*config = (t_config){(int)v[0], v[1], v[2], v[3], v[4], (int)v[5], v[6],
		SCHED_FIFO_POLICY};
	if (strcmp(argv[8], "edf") == 0)
		config->scheduler = SCHED_EDF_POLICY;
	return (0);
}

int	main(int argc, char **argv)
{
	t_config	config;

	if (parse_args(&config, argc, argv))
		return (1);
	return (simulation_run(&config));
}
