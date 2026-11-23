#ifndef CMD_GENERAL_P1_H
#define CMD_GENERAL_P1_H

void MostrarDirActual(void);

void Cmd_authors(char *tr[]);
void Cmd_pid(char *arg);
void Cmd_chdir(char *dir);
void Cmd_pwd(void);
void Cmd_date(char *arg[]);
void Cmd_sysinfo(char *arg[]);
void Cmd_help(char *tr[]);

#endif
