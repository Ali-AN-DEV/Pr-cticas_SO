/* Comandos de procesos (P3): getuid, setuid, prio, path, showenv, showvar,
   changevar, fork, ejec, ejecpri, pplano, pplanopri, splano, splanopri,
   jobs, deljobs.
   De momento estan las versiones de exec/pplano/splano de la P1:
   hay que rehacerlas segun el enunciado de la P3 (progspec + execve). */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "comun.h"
#include "cmd_procesos_p3.h"

static int ComprobarSegundoPlano(char *tr[])
{
    int i;
    for (i = 0; tr[i] != NULL; i++)
        if (!strcmp(tr[i], "&")) {  /* & indica segundo plano */
            tr[i] = NULL;           /* es el ultimo argumento */
            return i;               /* si solo hay un & no se ejecuta nada en pplano */
        }
    return 0;
}

void Cmd_exec(char *tr[])
{
    if (execvp(tr[0], tr) == -1)
        perror("Imposible ejecutar");
}

static void Proceso(char *tr[], int splano)
{
    pid_t pid;
    int background = splano || ComprobarSegundoPlano(tr);

    if ((pid = fork()) == -1) {
        perror("Imposible crear proceso");
        return;
    }
    if (pid == 0) {  /* proceso hijo */
        Cmd_exec(tr);
        exit(255);   /* por si falla exec */
    }
    if (!background)
        waitpid(pid, NULL, 0);
}

void Cmd_splano(char *arg[])
{
    Proceso(arg, 1);
}

void Cmd_pplano(char *arg[])
{
    Proceso(arg, 0);
}

