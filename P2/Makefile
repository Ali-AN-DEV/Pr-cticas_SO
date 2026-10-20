CC     = gcc
CFLAGS = -Wall -Wextra -g -MMD
OBJS   = p2.o shell.o comandos.o tabla.o
BIN    = p2

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(BIN)

.PHONY: clean

-include $(OBJS:.o=.d)