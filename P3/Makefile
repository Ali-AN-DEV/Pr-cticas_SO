CC     = gcc
CFLAGS = -Wall -Wextra -g -MMD
OBJS   = p3.o shell.o tabla_p1.o memoria_p2.o \
         cmd_general_p1.o cmd_ficheros_p1.o cmd_memoria_p2.o cmd_procesos_p3.o
BIN    = p3
 
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
 
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
 
clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(BIN)
 
.PHONY: clean
 
-include $(OBJS:.o=.d)