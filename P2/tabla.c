#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comun.h"
#include "tabla.h"

typedef struct tFichero {
    int descriptor;
    char nombre[MAXNOMBRE];
    int modo;
    int abierto;
} tFichero;

/* static: solo visibles dentro de este archivo */
static tFichero *tabla = NULL;
static int numEntradas = 0;  /* tamaño actual del array */

void InicializarTabla(void) {
    tabla = malloc(3 * sizeof(tFichero));
    numEntradas = 3;

    tabla[0].descriptor = 0;
    strcpy(tabla[0].nombre, "entrada estandar");
    tabla[0].abierto = 1;

    tabla[1].descriptor = 1;
    strcpy(tabla[1].nombre, "salida estandar");
    tabla[1].abierto = 1;

    tabla[2].descriptor = 2;
    strcpy(tabla[2].nombre, "error estandar");
    tabla[2].abierto = 1;
}

void AnadirFichero(int descriptor, const char *nombre, int modo) {
    int i;
    tFichero *aux;

    if (descriptor >= numEntradas) {
        aux = realloc(tabla, (descriptor + 1) * sizeof(tFichero));
        if (aux == NULL) {                      /* si falla, tabla sigue siendo valida */
            perror("Imposible ampliar tabla");
            return;
        }
        tabla = aux;
        for (i = numEntradas; i <= descriptor; i++)   /* huecos nuevos: marcados como libres */
            tabla[i].abierto = 0;
        numEntradas = descriptor + 1;
    }
    tabla[descriptor].descriptor = descriptor;
    strncpy(tabla[descriptor].nombre, nombre, MAXNOMBRE - 1);
    tabla[descriptor].nombre[MAXNOMBRE - 1] = '\0';   /* strncpy no garantiza el \0 */
    tabla[descriptor].modo = modo;
    tabla[descriptor].abierto = 1;
}

void LiberarTabla(void) {
    free(tabla);
    tabla = NULL;
    numEntradas = 0;
}

void ListarFicherosAbiertos(void) {
    int i;
    for (i = 0; i < numEntradas; i++) {
        if (tabla[i].abierto) {
            printf("Descriptor: %d, fichero: %s\n", tabla[i].descriptor, tabla[i].nombre);
        }
    }
}

int DescriptorAbierto(int df) {
    return df >= 0 && df < numEntradas && tabla[df].abierto;
}

const char *NombreDescriptor(int df) {
    return DescriptorAbierto(df) ? tabla[df].nombre : "?";
}

void MarcarCerrado(int df) {
    if (df >= 0 && df < numEntradas)
        tabla[df].abierto = 0;   /* liberamos la entrada, no la borramos del array */
}
