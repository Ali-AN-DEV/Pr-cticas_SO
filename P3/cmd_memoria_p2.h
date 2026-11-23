#ifndef CMD_MEMORIA_P2_H
#define CMD_MEMORIA_P2_H

void Cmd_recurse(char *tr[]);
void Cmd_allocate(char *tr[]);
void Cmd_deallocate(char *tr[]);
void Cmd_io(char *tr[]);
void Cmd_memory(char *tr[]);
void Cmd_read(char *tr[]);
void Cmd_write(char *tr[]);
void Cmd_memfill(char *tr[]);
void Cmd_memdump(char *tr[]);

#endif
