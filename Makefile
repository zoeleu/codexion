NAME = codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
CPPFLAGS = -Iinclude

SRC = src/main.c \
	src/simulation.c \
	src/monitor.c \
	src/coder.c \
	src/dongle.c \
	src/heap.c \
	src/heap_order.c \
	src/heap_storage.c \
	src/log.c \
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

-include $(DEP)

.PHONY: all clean fclean re norm
