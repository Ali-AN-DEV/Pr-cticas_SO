#include <stdlib.h>
#include <string.h>
#include "comun.h"
#include "tabla.h"
#include "comandos.h"
#include "shell.h"

static void DecidirComando(char *tr[])
{
    if (tr[0] == NULL)
        return;
    if (!strcmp(tr[0], "fin")) exit(0);
    else if (!strcmp(tr[0], "quit")) exit(0);
    else if (!strcmp(tr[0], "exit")) exit(0);
    else if (!strcmp(tr[0], "bye")) exit(0);
    else if (!strcmp(tr[0], "authors")) Cmd_authors(tr + 1);
    else if (!strcmp(tr[0], "exec")) Cmd_exec(tr + 1);
    else if (!strcmp(tr[0], "pplano")) Cmd_pplano(tr + 1);
    else if (!strcmp(tr[0], "splano")) Cmd_splano(tr + 1);
    else if (!strcmp(tr[0], "chdir")) Cmd_chdir(tr[1]);
    else if (!strcmp(tr[0], "pwd")) Cmd_pwd();
    else if (!strcmp(tr[0], "pid")) Cmd_pid(tr[1]);
    else if (!strcmp(tr[0], "open")) Cmd_open(tr + 1);
    else if (!strcmp(tr[0], "listopen")) ListarFicherosAbiertos();
    else if (!strcmp(tr[0], "close")) Cmd_close(tr + 1);
    else if (!strcmp(tr[0], "dup")) Cmd_dup(tr + 1);
    else if (!strcmp(tr[0], "lseek")) Cmd_lseek(tr + 1);
    else if (!strcmp(tr[0], "readstr")) Cmd_readstr(tr + 1);
    else if (!strcmp(tr[0], "writestr")) Cmd_writestr(tr + 1);
    else if (!strcmp(tr[0], "makefile")) Cmd_makefile(tr + 1);
    else if (!strcmp(tr[0], "makedir")) Cmd_makedir(tr + 1);
    else if (!strcmp(tr[0], "date")) Cmd_date(tr + 1);
    else if (!strcmp(tr[0], "sysinfo")) Cmd_sysinfo(tr + 1);
    else if (!strcmp(tr[0], "listfile")) Cmd_listfile(tr + 1);
    else if (!strcmp(tr[0], "list")) Cmd_list(tr + 1);
    else if (!strcmp(tr[0], "delete")) Cmd_delete(tr + 1);
    else if (!strcmp(tr[0], "deltree")) Cmd_deltree(tr + 1);
    else if (!strcmp(tr[0], "help")) Cmd_help(tr + 1);
    else Cmd_pplano(tr);
}

static int TrocearCadena(char *cadena, char *trozos[])
{
    int i = 1;
    if ((trozos[0] = strtok(cadena, " \n\t")) == NULL)
        return 0;
    while ((trozos[i] = strtok(NULL, " \n\t")) != NULL)
        i++;
    return i;
}

void ProcesarEntrada(char *entrada)
{
    char *tr[MAXENTRADA / 2];
    if (TrocearCadena(entrada, tr) == 0)   /* no hay nada */
        return;
    DecidirComando(tr);
}
