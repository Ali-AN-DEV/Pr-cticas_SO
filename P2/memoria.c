#include "memoria.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/shm.h>

/* Nodo de la lista, privado de memoria.c */
typedef struct tNodo {
    tBloque bloque;
    struct tNodo *siguiente;
} tNodo;

static tNodo *lista = NULL;
static tNodo *ultimo = NULL;

void inicializarLista(void) {
    lista = NULL;
    ultimo = NULL;
}

int insertarBloque(const tBloque *bloque) {
    tNodo *nuevo;

    if (bloque == NULL)
        return 0;
    if (bloque->tipo == MMAP && bloque->info.archivo.nombre == NULL)
        return 0;

    nuevo = malloc(sizeof(*nuevo));
    if (nuevo == NULL)
        return 0;

    nuevo->bloque = *bloque;
    nuevo->bloque.hora = time(NULL);
    nuevo->siguiente = NULL;

    if (bloque->tipo == MMAP) {   /* copia propia del nombre */
        nuevo->bloque.info.archivo.nombre = strdup(bloque->info.archivo.nombre);
        if (nuevo->bloque.info.archivo.nombre == NULL) {
            free(nuevo);
            return 0;
        }
    } 

    if (lista == NULL)
        lista = nuevo;
    else
        ultimo->siguiente = nuevo;
    ultimo = nuevo;

    return 1;
}

const tBloque *buscarMallocPorTamano(size_t tamano) {
    tNodo *actual;
    for (actual = lista; actual != NULL; actual = actual->siguiente)
        if (actual->bloque.tipo == MALLOC && actual->bloque.tamano == tamano)
            return &actual->bloque;
    return NULL;
}

const tBloque *buscarSharedPorClave(key_t clave) {
    tNodo *actual;
    for (actual = lista; actual != NULL; actual = actual->siguiente)
        if (actual->bloque.tipo == SHARED && actual->bloque.info.clave == clave)
            return &actual->bloque;
    return NULL;
}

const tBloque *buscarMmapPorNombre(const char *nombre) {
    tNodo *actual;
    if (nombre == NULL)
        return NULL;
    for (actual = lista; actual != NULL; actual = actual->siguiente)
        if (actual->bloque.tipo == MMAP &&
            strcmp(actual->bloque.info.archivo.nombre, nombre) == 0)
            return &actual->bloque;
    return NULL;
}

const tBloque *buscarBloquePorDireccion(const void *direccion) {
    tNodo *actual;
    for (actual = lista; actual != NULL; actual = actual->siguiente)
        if (actual->bloque.direccion == direccion)
            return &actual->bloque;
    return NULL;
}

int eliminarBloque(const void *direccion) {
    tNodo *actual = lista;
    tNodo *anterior = NULL;

    while (actual != NULL) {
        if (actual->bloque.direccion == direccion) {
            if (anterior == NULL)
                lista = actual->siguiente;
            else
                anterior->siguiente = actual->siguiente;
            if (actual == ultimo)
                ultimo = anterior;

            if (actual->bloque.tipo == MMAP)
                free(actual->bloque.info.archivo.nombre);
            free(actual);
            return 1;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    return 0;
}

static int coincideModo(tTipoBloque tipo, tModoListado modo) {
    switch (modo) {
        case LISTAR_MALLOC: return tipo == MALLOC;
        case LISTAR_SHARED: return tipo == SHARED;
        case LISTAR_MMAP:   return tipo == MMAP;
        default:            return 1;
    }
}

void mostrarBloques(tModoListado modo) {
    tNodo *actual;
    struct tm *tiempo;
    char fecha[32];

    /* OJO: comprueba cabeceras y anchos contra el shell de referencia */
    switch (modo) {
        case LISTAR_MALLOC:
            printf("******Lista de bloques asignados malloc para el proceso %ld\n", (long)getpid());
            break;
        case LISTAR_SHARED:
            printf("******Lista de bloques asignados shared para el proceso %ld\n", (long)getpid());
            break;
        case LISTAR_MMAP:
            printf("******Lista de bloques asignados mmap para el proceso %ld\n", (long)getpid());
            break;
        default:
            printf("******Lista de bloques asignados para el proceso %ld\n", (long)getpid());
            break;
    }

    for (actual = lista; actual != NULL; actual = actual->siguiente) {
        if (!coincideModo(actual->bloque.tipo, modo))
            continue;

        tiempo = localtime(&actual->bloque.hora);
        if (tiempo == NULL || strftime(fecha, sizeof(fecha), "%b %d %H:%M", tiempo) == 0)
            snprintf(fecha, sizeof(fecha), "fecha desconocida");

        printf("%20p %16zu %s ", actual->bloque.direccion, actual->bloque.tamano, fecha);

        switch (actual->bloque.tipo) {
            case MALLOC:
                printf("malloc\n");
                break;
            case SHARED:
                printf("shared (key %ld)\n", (long)actual->bloque.info.clave);
                break;
            case MMAP:
                printf("%s  (descriptor %d)\n", actual->bloque.info.archivo.nombre,
                       actual->bloque.info.archivo.descriptor);
                break;
        }
    }
}

void liberarLista(void) {
    tNodo *actual = lista;
    tNodo *siguiente;

    while (actual != NULL) {
        siguiente = actual->siguiente;

        switch (actual->bloque.tipo) {
            case MALLOC:
                free(actual->bloque.direccion);
                break;
            case SHARED:
                if (shmdt(actual->bloque.direccion) == -1)
                    perror("Error en shmdt");
                break;
            case MMAP:
                if (munmap(actual->bloque.direccion, actual->bloque.tamano) == -1)
                    perror("Error en munmap");
                if (actual->bloque.info.archivo.descriptor >= 0 &&
                    close(actual->bloque.info.archivo.descriptor) == -1)
                    perror("Error en close");
                free(actual->bloque.info.archivo.nombre);
                break;
        }
        free(actual);
        actual = siguiente;
    }
    lista = NULL;
    ultimo = NULL;
}