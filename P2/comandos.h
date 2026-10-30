#ifndef COMANDOS_H
#define COMANDOS_H

void Cmd_exec(char *tr[]);
void Cmd_pplano(char *arg[]);
void Cmd_splano(char *arg[]);
void Cmd_pid(char *arg);
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
void Cmd_authors(char *tr[]);
void Cmd_chdir(char *dir);
void Cmd_pwd(void);
void Cmd_date(char *arg[]);
void Cmd_sysinfo(char *arg[]);
void Cmd_help(char *tr[]);
void Cmd_recurse(char *tr[]); 
void Cmd_allocate(char *tr[]);
void Cmd_deallocate(char *tr[]);

#endif
