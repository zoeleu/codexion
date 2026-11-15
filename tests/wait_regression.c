/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   wait_regression.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zleullie <zleullie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/11 00:00:00 by zleullie          #+#    #+#             */
/*   Updated: 2026/11/15 00:00:00 by zleullie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <assert.h>

typedef struct s_fixture
{
	t_simulation	sim;
	t_coder			coder;
	t_dongle		dongles[2];
}	t_fixture;

static pthread_mutex_t	g_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	g_ready_condition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t	*g_wait_condition;
static int				g_wait_ready;

int	__real_pthread_cond_timedwait(pthread_cond_t *condition,
		pthread_mutex_t *mutex, const struct timespec *deadline);

/* Notify while the worker still holds the state mutex. The test then
   acquires that mutex, proving the real wait has released it. */
int	__wrap_pthread_cond_timedwait(pthread_cond_t *condition,
		pthread_mutex_t *mutex, const struct timespec *deadline)
{
	assert(!pthread_mutex_lock(&g_ready_mutex));
	if (condition == g_wait_condition)
	{
		g_wait_ready = 1;
		assert(!pthread_cond_signal(&g_ready_condition));
	}
	assert(!pthread_mutex_unlock(&g_ready_mutex));
	return (__real_pthread_cond_timedwait(condition, mutex, deadline));
}

static void	arm_wait_readiness(t_simulation *sim)
{
	assert(!pthread_mutex_lock(&g_ready_mutex));
	g_wait_condition = &sim->phase_condition;
	g_wait_ready = 0;
	assert(!pthread_mutex_unlock(&g_ready_mutex));
}

static void	await_wait_readiness(t_simulation *sim)
{
	assert(!pthread_mutex_lock(&g_ready_mutex));
	while (!g_wait_ready)
		assert(!pthread_cond_wait(&g_ready_condition, &g_ready_mutex));
	g_wait_condition = NULL;
	assert(!pthread_mutex_unlock(&g_ready_mutex));
	assert(!pthread_mutex_lock(&sim->state_mutex));
}

static void	init_fixture(t_fixture *f, long burnout_ms)
{
	memset(f, 0, sizeof(*f));
	assert(!simulation_state_init(&f->sim));
	assert(!pthread_cond_init(&f->sim.log_condition, NULL));
	f->sim.start_time = time_now();
	f->coder.simulation = &f->sim;
	f->coder.burnout_time = time_add_ms(time_now(), burnout_ms);
	assert(!dongle_initialize(&f->dongles[0], SCHED_FIFO_POLICY));
	assert(!dongle_initialize(&f->dongles[1], SCHED_FIFO_POLICY));
}

static void	destroy_fixture(t_fixture *f)
{
	t_log_event	*event;

	while (f->sim.log_head != NULL)
	{
		event = f->sim.log_head;
		f->sim.log_head = event->next;
		free(event);
	}
	heap_destroy(&f->dongles[0].requests);
	heap_destroy(&f->dongles[1].requests);
	pthread_mutex_destroy(&f->dongles[0].dongle_taken);
	pthread_mutex_destroy(&f->dongles[1].dongle_taken);
	assert(!pthread_cond_destroy(&f->sim.log_condition));
	simulation_state_destroy(&f->sim);
}

/* A pending cooldown delays the pair grant until it expires. */
static void	check_pair_grant_waits_cooldown(void)
{
	t_fixture		f;
	struct timespec	begin;
	long long		waited;

	init_fixture(&f, 5000);
	f.dongles[0].cooldown_until = time_add_ms(time_now(), 50);
	begin = time_now();
	assert(dongle_acquire(&f.coder, &f.dongles[0], &f.dongles[1]));
	waited = time_elapsed_ms(begin, time_now());
	assert(waited >= 40 && waited < 1000);
	assert(f.coder.taken_dongles[0] == &f.dongles[0]);
	assert(f.coder.taken_dongles[1] == &f.dongles[1]);
	assert(f.dongles[0].requests.root == NULL);
	assert(f.dongles[1].requests.root == NULL);
	dongle_release_pair(&f.coder);
	assert(!f.dongles[0].in_use && !f.dongles[1].in_use);
	assert(f.coder.taken_dongles[0] == NULL);
	destroy_fixture(&f);
}

/* A lone coder holds its only dongle, then aborts at its deadline. */
static void	check_single_dongle_burnout(void)
{
	t_fixture		f;
	struct timespec	begin;

	init_fixture(&f, 100);
	begin = time_now();
	assert(!dongle_acquire(&f.coder, &f.dongles[0], &f.dongles[0]));
	assert(time_elapsed_ms(begin, time_now()) >= 90);
	assert(f.coder.taken_dongles[0] == &f.dongles[0]);
	assert(f.coder.taken_dongles[1] == NULL);
	dongle_release_pair(&f.coder);
	destroy_fixture(&f);
}

static void	*acquire_waiter(void *arg)
{
	t_fixture	*f;

	f = arg;
	assert(!dongle_acquire(&f->coder, &f->dongles[0], &f->dongles[1]));
	return (NULL);
}

/* Shutdown wakes a blocked acquisition; nothing is held afterwards. */
static void	check_stop_wakes_acquire(void)
{
	t_fixture	f;
	pthread_t	thread;

	init_fixture(&f, 20000);
	f.dongles[1].in_use = 1;
	arm_wait_readiness(&f.sim);
	assert(!pthread_create(&thread, NULL, acquire_waiter, &f));
	await_wait_readiness(&f.sim);
	assert(heap_peek(&f.dongles[0].requests) != NULL);
	assert(heap_peek(&f.dongles[0].requests)->coder == &f.coder);
	assert(heap_peek(&f.dongles[0].requests)
		== heap_peek(&f.dongles[1].requests));
	assert(!pthread_mutex_unlock(&f.sim.state_mutex));
	simulation_stop(&f.sim, SIM_SUCCESS);
	pthread_join(thread, NULL);
	assert(f.coder.taken_dongles[0] == NULL);
	assert(f.coder.taken_dongles[1] == NULL);
	assert(f.dongles[0].requests.root == NULL);
	assert(f.dongles[1].requests.root == NULL);
	destroy_fixture(&f);
}

static void	*phase_waiter(void *arg)
{
	assert(!simulation_wait_phase(arg, 10000));
	return (NULL);
}

static void	check_stop_wakes_phase(void)
{
	t_simulation	sim;
	t_coder			coder;
	pthread_t		thread;

	memset(&sim, 0, sizeof(sim));
	memset(&coder, 0, sizeof(coder));
	assert(!simulation_state_init(&sim));
	coder.simulation = &sim;
	coder.burnout_time = time_add_ms(time_now(), 20000);
	assert(simulation_wait_phase(&coder, 1));
	arm_wait_readiness(&sim);
	assert(!pthread_create(&thread, NULL, phase_waiter, &coder));
	await_wait_readiness(&sim);
	assert(!pthread_mutex_unlock(&sim.state_mutex));
	simulation_stop(&sim, SIM_SUCCESS);
	pthread_join(thread, NULL);
	simulation_state_destroy(&sim);
}

int	main(void)
{
	check_pair_grant_waits_cooldown();
	check_single_dongle_burnout();
	check_stop_wakes_acquire();
	check_stop_wakes_phase();
	return (0);
}
