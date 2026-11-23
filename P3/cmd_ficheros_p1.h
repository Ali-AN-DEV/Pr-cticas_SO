#ifndef CMD_FICHEROS_P1_H
#define CMD_FICHEROS_P1_H

void Cmd_open(char *tr[]);
void Cmd_close(char *tr[]);
void Cmd_dup(char *tr[]);
void Cmd_lseek(char *tr[]);
void Cmd_readstr(char *tr[]);
void Cmd_writestr(char *tr[]);
void Cmd_makefile(char *tr[]);
void Cmd_makedir(char *tr[]);
void Cmd_listfile(char *tr[]);
void Cmd_list(char *tr[]);
void Cmd_delete(char *tr[]);
void Cmd_deltree(char *tr[]);

#endif
