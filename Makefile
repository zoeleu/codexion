NAME = codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
CPPFLAGS = -Iinclude

SRC = src/main.c \
	src/config.c \
	src/simulation.c \
	src/simulation_state.c \
	src/coder.c \
	src/dongle.c \
	src/dongle_queue.c \
	src/heap.c \
	src/heap_order.c \
	src/heap_storage.c \
	src/monitor.c \
	src/log.c \
	src/log_queue.c \
	src/time.c
OBJ = $(SRC:src/%.c=build/%.o)
DEP = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

fclean: clean
	rm -f $(NAME)

re: fclean
	$(MAKE) all

norm:
	norminette include src

build/wait_regression: tests/wait_regression.c $(filter-out build/main.o,$(OBJ)) include/codexion.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(filter-out build/main.o,$(OBJ)) -Wl,--wrap=pthread_cond_timedwait -o $@

test: $(NAME) build/wait_regression
	timeout 2 ./build/wait_regression
	bash tests/regression.sh

-include $(DEP)

.PHONY: all clean fclean re norm test
