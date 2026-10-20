/*
 * Sistemas Operativos - P2
 * Autores:
 *   Ruben Varela Tarrio - ruben.varela.tarrio@udc.es
 *   Ali Abu-afash Nayef - ali.nayef@udc.es
 */

#include <stdio.h>
#include <stdlib.h>
#include "comun.h"
#include "tabla.h"
#include "shell.h"

int main(void)
{
    char entrada[MAXENTRADA];

    InicializarTabla();
    atexit(LiberarTabla);

    while (1) {
        printf("-> ");
        /* Con ctrl+D fgets devuelve NULL: si no lo comprobamos, el bucle
           reprocesaria el buffer anterior infinitamente. */
        if (fgets(entrada, MAXENTRADA, stdin) == NULL) {
            printf("\n");
            break;
        }
        ProcesarEntrada(entrada);
    }
    return 0;
}
