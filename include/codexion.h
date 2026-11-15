/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:42:41 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <limits.h>
# include <errno.h>
# include <pthread.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/time.h>
# include <time.h>
# include <unistd.h>

typedef enum e_scheduler
{
	SCHED_FIFO_POLICY,
	SCHED_EDF_POLICY
}	t_scheduler;

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

/* Requests keep a stable stack address until removed from both heaps. */
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

/* Complete binary min-heap, protected by the simulation state mutex. */
typedef struct s_request_heap
{
	t_heap_node			*root;
	size_t				size;
	unsigned long long	next_arrival;
	t_scheduler			scheduler;
}	t_request_heap;

/* dongle_taken guards only in_use and cooldown_until (inner lock). */
typedef struct s_dongle
{
	pthread_mutex_t		dongle_taken;
	t_request_heap		requests;
	int					in_use;
	struct timespec		cooldown_until;
}	t_dongle;

typedef enum e_coder_state
{
	WAITING = 0,
	COMPILING = 1,
	DEBUGGING = 2,
	REFACTORING = 3,
	BURNED_OUT = 4
}	t_coder_state;

typedef struct s_coder
{
	struct s_simulation	*simulation;
	int					index;
	t_dongle			*taken_dongles[2];
	struct timespec		burnout_time;
	unsigned int		number_of_compiles;
	t_coder_state		state;
}	t_coder;

/* Dongles points to the shared array; index is the coder's zero-based seat. */
typedef struct s_coder_worker
{
	t_coder		*coder;
	t_config	*config;
	t_dongle	*dongles;
	int			index;
}	t_coder_worker;

typedef enum e_simulation_result
{
	SIM_RUNNING,
	SIM_SUCCESS,
	SIM_BURNOUT,
	SIM_ERROR
}	t_simulation_result;

/* Phase values match t_coder_state; WAITING has no output event. */
typedef enum e_log_kind
{
	LOG_DONGLE,
	LOG_COMPILE,
	LOG_DEBUG,
	LOG_REFACTOR,
	LOG_BURNOUT
}	t_log_kind;

typedef struct s_log_event
{
	long long			milliseconds;
	int					coder_number;
	t_log_kind			kind;
	struct s_log_event	*next;
}	t_log_event;

/* State mutex protects result, started, coder state/deadlines/counts,
   the log queue, and every dongle's request heap. */
typedef struct s_simulation
{
	t_config			config;
	t_coder				*coders;
	t_dongle			*dongles;
	t_coder_worker		*workers;
	pthread_t			*threads;
	pthread_t			monitor;
	pthread_t			logger;
	pthread_cond_t		log_condition;
	t_log_event			*log_head;
	t_log_event			*log_tail;
	int					logger_started;
	int					logs_closed;
	pthread_mutex_t		state_mutex;
	pthread_cond_t		start_condition;
	pthread_cond_t		phase_condition;
	struct timespec		start_time;
	t_simulation_result	result;
	int					started;
	int					initialized_dongles;
	int					created_threads;
	int					burned_out_index;
}	t_simulation;

int				simulation_run(const t_config *config);
int				simulation_state_init(t_simulation *simulation);
void			simulation_state_destroy(t_simulation *simulation);
int				simulation_wait_phase(t_coder *coder, long milliseconds);
void			simulation_start_coder(t_coder *coder);
int				simulation_stopped(t_simulation *simulation);
void			simulation_wait_start(t_simulation *simulation);
void			simulation_stop(t_simulation *simulation,
					t_simulation_result result);
void			*monitor_worker(void *arg);

int				log_initialize(t_simulation *simulation);
void			log_finish(t_simulation *simulation);
/* Caller holds state_mutex. Returns 1 and sets SIM_ERROR on malloc failure. */
int				log_event_locked(t_simulation *simulation, int index,
					t_log_kind kind);
void			*log_worker(void *arg);

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

/* Realtime helpers match the default pthread condition-variable clock. */
struct timespec	time_now(void);
struct timespec	time_add_ms(struct timespec time, long milliseconds);
long long		time_elapsed_ms(struct timespec start, struct timespec end);
int				time_reached(struct timespec now, struct timespec deadline);

void			*coder_worker(void *arg);
int				parse_args(t_config *config, int argc, char **argv);
int				dongle_initialize(t_dongle *dongle, t_scheduler scheduler);
int				dongle_acquire(t_coder *coder, t_dongle *a, t_dongle *b);
void			dongle_release_pair(t_coder *coder);
int				dongle_enqueue_pair(t_dongle *a, t_dongle *b,
					t_request *request);

#endif
