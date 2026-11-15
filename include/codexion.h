/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <limits.h>
# include <pthread.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>

typedef enum e_scheduler
{
	SCHED_FIFO_POLICY,
	SCHED_EDF_POLICY
}	t_scheduler;

typedef enum e_result
{
	SIM_RUNNING,
	SIM_SUCCESS,
	SIM_BURNOUT,
	SIM_ERROR
}	t_result;

typedef struct s_config
{
	int				number_of_coders;
	long			time_to_burnout;
	long			time_to_compile;
	long			time_to_debug;
	long			time_to_refactor;
	int				number_of_compiles_required;
	long			dongle_cooldown;
	t_scheduler		scheduler;
}	t_config;

/* Lives on the requesting coder's stack while queued in both heaps. */
typedef struct s_dongle_request
{
	struct s_coder	*coder;
	struct timespec	deadline;
}	t_request;

typedef struct s_heap_entry
{
	t_request			*request;
	struct timespec		deadline;
	unsigned long long	arrival;
}	t_heap_entry;

typedef struct s_heap_node
{
	t_heap_entry		entry;
	struct s_heap_node	*parent;
	struct s_heap_node	*left;
	struct s_heap_node	*right;
}	t_heap_node;

/* Complete binary min-heap, protected by the simulation mutex. */
typedef struct s_request_heap
{
	t_heap_node			*root;
	size_t				size;
	unsigned long long	next_arrival;
	t_scheduler			scheduler;
}	t_request_heap;

/* mutex guards in_use and cooldown_until; requests uses the sim mutex. */
typedef struct s_dongle
{
	pthread_mutex_t		mutex;
	int					in_use;
	struct timespec		cooldown_until;
	t_request_heap		requests;
}	t_dongle;

typedef struct s_coder
{
	struct s_simulation	*sim;
	pthread_t			thread;
	int					index;
	int					compiles;
	struct timespec		burnout_time;
	t_dongle			*left;
	t_dongle			*right;
	t_dongle			*taken[2];
}	t_coder;

/* mutex protects everything shared except dongle availability, and also
   serializes output. cond wakes every phase and dongle wait. */
typedef struct s_simulation
{
	t_config			config;
	t_coder				*coders;
	t_dongle			*dongles;
	int					ready_dongles;
	pthread_mutex_t		mutex;
	pthread_cond_t		cond;
	struct timespec		start_time;
	t_result			result;
}	t_simulation;

int				parse_args(t_config *config, int argc, char **argv);
int				simulation_run(const t_config *config);
/* Caller holds sim->mutex. */
void			simulation_stop(t_simulation *sim, t_result result);
void			*monitor_routine(void *arg);

void			*coder_routine(void *arg);
int				coder_wait(t_coder *coder, long milliseconds);
void			coder_log(t_coder *coder, const char *message);

int				dongle_acquire(t_coder *coder);
void			dongle_release(t_coder *coder);

/* Init on a fresh queue. Init/push return 0 on success, 1 on failure. */
int				heap_init(t_request_heap *heap, t_scheduler scheduler);
void			heap_destroy(t_request_heap *heap);
t_heap_node		*heap_new_node(t_request_heap *heap, t_request *request);
t_heap_node		*heap_node_at(t_request_heap *heap, size_t position);
t_heap_node		*heap_next_node(t_heap_node *node);
int				heap_push(t_request_heap *heap, t_request *request);
t_request		*heap_peek(const t_request_heap *heap);
t_request		*heap_pop(t_request_heap *heap);
void			heap_remove(t_request_heap *heap, t_request *request);
void			heap_repair(t_request_heap *heap, t_heap_node *node);

/* CLOCK_REALTIME matches the default pthread condition-variable clock. */
struct timespec	time_now(void);
struct timespec	time_add_ms(struct timespec time, long milliseconds);
int				time_reached(struct timespec now, struct timespec deadline);
long long		time_elapsed_ms(struct timespec start, struct timespec end);

#endif
