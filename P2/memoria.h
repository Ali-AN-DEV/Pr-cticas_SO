#ifndef MEMORIA_H
#define MEMORIA_H

#include <stddef.h>
#include <time.h>
#include <sys/types.h>

typedef enum {
    MALLOC,
    SHARED,
    MMAP
} tTipoBloque;

/* Que bloques muestra mostrarBloques */
typedef enum {
    LISTAR_TODO,
    LISTAR_MALLOC,
    LISTAR_SHARED,
    LISTAR_MMAP
} tModoListado;

typedef struct {
    void *direccion;
    size_t tamano;
    time_t hora;            /* la rellena insertarBloque */
    tTipoBloque tipo;
    union {
        key_t clave;        /* SHARED */
        struct {            /* MMAP */
            char *nombre;   /* insertarBloque guarda una copia propia */
            int descriptor;
        } archivo;
    } info;
} tBloque;

/* Operaciones de la lista */

void inicializarLista(void);

/* Copia *bloque al final de la lista y pone la hora actual.
   Devuelve 1 si OK, 0 si error. */
int insertarBloque(const tBloque *bloque);

/* Los buscar* devuelven un puntero DENTRO del nodo (NULL si no existe).
   Copia lo que necesites (direccion, tamano...) ANTES de llamar a
   eliminarBloque: despues el puntero queda invalido. */
const tBloque *buscarMallocPorTamano(size_t tamano);
const tBloque *buscarSharedPorClave(key_t clave);
const tBloque *buscarMmapPorNombre(const char *nombre);
const tBloque *buscarBloquePorDireccion(const void *direccion);

/* Quita SOLO el registro de la lista (y libera el nombre si es mmap).
   NO libera/desmapea/cierra: eso lo hace antes el comando.
   Devuelve 1 si lo encontro, 0 si no. */
int eliminarBloque(const void *direccion);

void mostrarBloques(tModoListado modo);

/* Libera TODO lo que quede (memoria, shmdt, munmap, close) y la lista.
   Pensada para atexit. */
void liberarLista(void);

#endif