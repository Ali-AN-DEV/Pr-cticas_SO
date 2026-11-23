/* Comandos de memoria: allocate, deallocate, i-o, memory, read, write,
   memfill, memdump, recurse */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <ctype.h>

#include "comun.h"
#include "tabla_p1.h"
#include "memoria_p2.h"
#include "cmd_general_p1.h"
#include "cmd_ficheros_p1.h"
#include "cmd_memoria_p2.h"

#define BYTES_POR_LINEA 20

static void Recursiva (int n)
{
  char automatico[MAXENTRADA];
  static char estatico[MAXENTRADA];

  printf ("parametro:%3d(%p) array %p, arr estatico %p\n",n,&n,automatico, estatico);

  if (n>0)
    Recursiva(n-1);
}

void Cmd_recurse(char *tr[])
{
    /* 1. si falta el argumento, ¿qué debe pasar? (mira el shell de referencia) */
    if (tr[0] == NULL){
        return; 
    }

    /* 2. pasar la cadena tr[0] a int (como en Cmd_close) */
    int n = atoi(tr[0]); 

    /* 3. llamar a Recursiva con ese valor */
    Recursiva(n);
}


//allocate y auxiliares:

/* Obtiene (o crea, si tam != 0) un bloque de memoria compartida, lo
   engancha al proceso y lo anade a la lista. NULL y errno si falla. */
static void *ObtenerMemoriaShmget(key_t clave, size_t tam)
{
    void *p;
    int aux, id, flags = 0777;   /* los 9 bits menos significativos: permisos */
    struct shmid_ds s;
    tBloque b;
 
    if (tam)                     /* tam != 0 indica crear */
        flags |= IPC_CREAT | IPC_EXCL;
    if (clave == IPC_PRIVATE) {  /* no nos vale */
        errno = EINVAL;
        return NULL;
    }
    if ((id = shmget(clave, tam, flags)) == -1)
        return NULL;
    if ((p = shmat(id, NULL, 0)) == (void *) -1) {
        aux = errno;
        if (tam)
            shmctl(id, IPC_RMID, NULL);   /* no dejamos basura en el sistema */
        errno = aux;
        return NULL;
    }
 
    /* Si no es crear, el tamano real esta en shm_segsz */
    if (shmctl(id, IPC_STAT, &s) == -1) {
        aux = errno;
        shmdt(p);
        if (tam)
            shmctl(id, IPC_RMID, NULL);
        errno = aux;
        return NULL;
    }
 
    memset(&b, 0, sizeof(b));
    b.direccion = p;
    b.tamano = s.shm_segsz;
    b.tipo = SHARED;
    b.info.clave = clave;
    if (!insertarBloque(&b)) {
        shmdt(p);
        if (tam)
            shmctl(id, IPC_RMID, NULL);
        errno = ENOMEM;
        return NULL;
    }
    return p;
}
 
/* Mapea el fichero entero, lo anade a la lista de bloques y su descriptor
   a la tabla de ficheros abiertos. NULL y errno si falla. */
static void *MapearFichero(char *fichero, int protection)
{
    int df = -1, aux, modo = O_RDONLY;
    struct stat s;
    void *p;
    tBloque b;
    char nombreTabla[MAXNOMBRE];
 
    if (protection & PROT_WRITE)
        modo = O_RDWR;
    if (stat(fichero, &s) == -1 || (df = open(fichero, modo)) == -1)
        return NULL;
    if ((p = mmap(NULL, s.st_size, protection, MAP_PRIVATE, df, 0)) == MAP_FAILED) {
        aux = errno;
        close(df);               /* si no, el descriptor se queda abierto */
        errno = aux;
        return NULL;
    }
 
    memset(&b, 0, sizeof(b));
    b.direccion = p;
    b.tamano = s.st_size;
    b.tipo = MMAP;
    b.info.archivo.nombre = fichero;   /* insertarBloque guarda su propia copia */
    b.info.archivo.descriptor = df;
    if (!insertarBloque(&b)) {
        munmap(p, s.st_size);
        close(df);
        errno = ENOMEM;
        return NULL;
    }
 
    snprintf(nombreTabla, sizeof(nombreTabla), "Mapeo de %s", fichero);
    AnadirFichero(df, nombreTabla, modo);
    return p;
}
 
static void AllocateMalloc(char *tr[])
{
    size_t tam;
    void *p;
    tBloque b;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_MALLOC);
        return;
    }
    tam = (size_t) strtoul(tr[0], NULL, 10);
    if (tam == 0) {
        printf("No se asignan bloques de 0 bytes\n");
        return;
    }
    if ((p = malloc(tam)) == NULL) {
        perror("Imposible obtener memoria con malloc");
        return;
    }
 
    memset(&b, 0, sizeof(b));
    b.direccion = p;
    b.tamano = tam;
    b.tipo = MALLOC;
    if (!insertarBloque(&b)) {
        free(p);                 /* sin registro en la lista no hay quien lo libere */
        fprintf(stderr, "Imposible anadir el bloque a la lista\n");
        return;
    }
    printf("Asignados %lu bytes en %p\n", (unsigned long) tam, p);
}
 
static void AllocateCreateShared(char *tr[])
{
    key_t cl;
    size_t tam;
    void *p;
 
    if (tr[0] == NULL || tr[1] == NULL) {
        mostrarBloques(LISTAR_SHARED);
        return;
    }
    cl = (key_t) strtoul(tr[0], NULL, 10);
    tam = (size_t) strtoul(tr[1], NULL, 10);
    if (tam == 0) {
        printf("No se asignan bloques de 0 bytes\n");
        return;
    }
    if ((p = ObtenerMemoriaShmget(cl, tam)) != NULL)
        printf("Asignados %lu bytes en %p\n", (unsigned long) tam, p);
    else
        printf("Imposible asignar memoria compartida clave %lu: %s\n",
               (unsigned long) cl, strerror(errno));
}
 
static void AllocateShared(char *tr[])
{
    key_t cl;
    void *p;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_SHARED);
        return;
    }
    cl = (key_t) strtoul(tr[0], NULL, 10);
    if ((p = ObtenerMemoriaShmget(cl, 0)) != NULL)
        printf("Memoria compartida de clave %lu  en %p\n", (unsigned long) cl, p);
    else
        printf("Imposible asignar memoria compartida clave %lu: %s\n",
               (unsigned long) cl, strerror(errno));
}
 
static void AllocateMmap(char *tr[])
{
    char *perm;
    void *p;
    int protection = 0;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_MMAP);
        return;
    }
    if ((perm = tr[1]) != NULL && strlen(perm) < 4) {
        if (strchr(perm, 'r') != NULL) protection |= PROT_READ;
        if (strchr(perm, 'w') != NULL) protection |= PROT_WRITE;
        if (strchr(perm, 'x') != NULL) protection |= PROT_EXEC;
    }
    if ((p = MapearFichero(tr[0], protection)) == NULL)
        perror("Imposible mapear fichero");
    else
        printf("fichero %s mapeado en %p\n", tr[0], p);
}
 
void Cmd_allocate(char *tr[])
{
    // 1) si tr[0] == NULL, es decir un simple allcoate, listar todos los bloques
    // 2) tr[0] es "-malloc"
    if (tr[0] == NULL)
        mostrarBloques(LISTAR_TODO);
    else if (!strcmp(tr[0], "-malloc"))       AllocateMalloc(tr + 1);
    else if (!strcmp(tr[0], "-createshared")) AllocateCreateShared(tr + 1);
    else if (!strcmp(tr[0], "-shared"))       AllocateShared(tr + 1);
    else if (!strcmp(tr[0], "-mmap"))         AllocateMmap(tr + 1);
    else
        printf("uso: allocate [-malloc|-shared|-createshared|-mmap] ....\n");
}


//Deallocate y auxiliares:

/* Libera el recurso del bloque segun su tipo y lo quita de la lista.
   Recibe un puntero DENTRO de la lista, asi que copia lo que necesita
   antes de que eliminarBloque lo deje invalido. */


static void LiberarBloque(const tBloque *b)
{
    tBloque c = *b;
 
    switch (c.tipo) {
        case MALLOC:
            free(c.direccion);
            break;
        case SHARED:
            if (shmdt(c.direccion) == -1) {
                perror("Imposible desasignar memoria compartida");
                return;          /* no seguimos: el bloque sigue enganchado */
            }
            break;
        case MMAP:
            if (munmap(c.direccion, c.tamano) == -1) {
                perror("Imposible desmapear fichero");
                return;
            }
            if (close(c.info.archivo.descriptor) == -1)
                perror("Imposible cerrar descriptor");
            MarcarCerrado(c.info.archivo.descriptor);   /* fuera de la tabla de abiertos */
            break;
    }
    eliminarBloque(c.direccion);
}

static void DeallocateMalloc(char *tr[])
{
    size_t tam;
    const tBloque *b;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_MALLOC);
        return;
    }
    tam = (size_t) strtoul(tr[0], NULL, 10);
    if (tam == 0) {
        printf("No se asignan bloques de 0 bytes\n");
        return;
    }
    if ((b = buscarMallocPorTamano(tam)) == NULL)
        printf("No hay bloque de ese tamano asignado con malloc\n");
    else
        LiberarBloque(b);
}

static void DeallocateShared(char *tr[])
{
    key_t cl;
    const tBloque *b;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_SHARED);
        return;
    }
    cl = (key_t) strtoul(tr[0], NULL, 10);
    if ((b = buscarSharedPorClave(cl)) == NULL)
        printf("No hay bloque de esa clave mapeado en el proceso\n");
    else
        LiberarBloque(b);
}

static void DeallocateMmap(char *tr[])
{
    const tBloque *b;
 
    if (tr[0] == NULL) {
        mostrarBloques(LISTAR_MMAP);
        return;
    }
    if ((b = buscarMmapPorNombre(tr[0])) == NULL)
        printf("Fichero %s no mapeado\n", tr[0]);
    else
        LiberarBloque(b);
}

static void SharedDelkey(char *tr[])
{
    key_t clave = IPC_PRIVATE;
    int id;
 
    if (tr[0] == NULL || (clave = (key_t) strtoul(tr[0], NULL, 10)) == IPC_PRIVATE) {
        printf("      delkey necesita clave_valida\n");
        return;
    }
    if ((id = shmget(clave, 0, 0666)) == -1) {
        perror("shmget: imposible obtener memoria compartida");
        return;
    }
    if (shmctl(id, IPC_RMID, NULL) == -1)
        perror("shmctl: imposible eliminar memoria compartida");
}

static void DeallocateDireccion(char *cadena)
{
    void *p = NULL;              /* si sscanf falla, NO queremos basura */
    const tBloque *b;
 
    if (sscanf(cadena, "%p", &p) != 1)
        p = NULL;
    if (p == NULL) {             /* como el shell de referencia: 0x0 o texto no valido -> lista */
        mostrarBloques(LISTAR_TODO);
        return;
    }
    if ((b = buscarBloquePorDireccion(p)) == NULL)
        printf("Direccion %p no asignada con malloc, shared o mmap\n", p);
    else
        LiberarBloque(b);
}

void Cmd_deallocate(char *tr[])
{
    // 1) si tr[0] == NULL listar todos
    // 2) tr"-amlloc" con un -malloc tam localizar un bloque de ese tamalo 
    if (tr[0] == NULL)
        mostrarBloques(LISTAR_TODO);
    else if (!strcmp(tr[0], "-malloc"))  DeallocateMalloc(tr + 1);
    else if (!strcmp(tr[0], "-shared"))  DeallocateShared(tr + 1);
    else if (!strcmp(tr[0], "-delkey"))  SharedDelkey(tr + 1);
    else if (!strcmp(tr[0], "-mmap"))    DeallocateMmap(tr + 1);
    else
        DeallocateDireccion(tr[0]);
}

/* Convierte "0x7ffd..." en puntero. Devuelve 1 si OK, 0 si el texto no es una direccion. */
static int ParsearDireccion(const char *s, void **dir)
{
    void *p = NULL;

    if (s == NULL || sscanf(s, "%p", &p) != 1)
        return 0;
    *dir = p;
    return 1;
}

//i-o and memory 

#define CONT_TODO ((size_t) -1)   /* "leer el fichero entero" */

/* Lee de f a p. Devuelve bytes leidos, o -1 con errno. */
static ssize_t LeerFichero(const char *f, void *p, size_t cont)
{
    struct stat s;
    ssize_t n;
    int df, aux;

    if ((df = open(f, O_RDONLY)) == -1)
        return -1;
    if (cont == CONT_TODO) {
        if (fstat(df, &s) == -1) {
            aux = errno;
            close(df);
            errno = aux;
            return -1;
        }
        cont = (size_t) s.st_size;
    }
    if ((n = read(df, p, cont)) == -1) {
        aux = errno;               /* close puede cambiar errno */
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

/* Escribe cont bytes de p en f. Devuelve bytes escritos, o -1 con errno. */
static ssize_t EscribirFichero(const char *f, const void *p, size_t cont, int sobrescribir)
{
    ssize_t n;
    int df, aux, flags = O_WRONLY | O_CREAT;

    flags |= sobrescribir ? O_TRUNC : O_EXCL;
    if ((df = open(f, flags, 0666)) == -1)
        return -1;
    if ((n = write(df, p, cont)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return n;
}

static void IORead(char *tr[])
{
    void *dir;
    size_t cont = CONT_TODO;       /* sin cnt, se lee entero (como hace ayudaP2) */
    ssize_t n;

    if (tr[0] == NULL || tr[1] == NULL) {
        printf("uso: i-o read f addr [cnt]\n");
        return;
    }
    if (!ParsearDireccion(tr[1], &dir)) {
        printf("Direccion invalida: %s\n", tr[1]);
        return;
    }
    if (tr[2] != NULL)
        cont = (size_t) strtoul(tr[2], NULL, 10);

    if ((n = LeerFichero(tr[0], dir, cont)) == -1)
        printf("Imposible leer %s: %s\n", tr[0], strerror(errno));
    else
        printf("leidos %zd bytes de %s en %p\n", n, tr[0], dir);
}

static void IOWrite(char *tr[])
{
    void *dir;
    size_t cont;
    ssize_t n;
    int sobrescribir = 0;

    if (tr[0] != NULL && !strcmp(tr[0], "-o")) {
        sobrescribir = 1;
        tr++;                      /* nos saltamos la opcion */
    }
    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("uso: i-o write [-o] f addr cnt\n");
        return;
    }
    if (!ParsearDireccion(tr[1], &dir)) {
        printf("Direccion invalida: %s\n", tr[1]);
        return;
    }
    cont = (size_t) strtoul(tr[2], NULL, 10);

    if ((n = EscribirFichero(tr[0], dir, cont, sobrescribir)) == -1)
        printf("Imposible escribir %s: %s\n", tr[0], strerror(errno));
    else
        printf("escritos %zd bytes en %s desde %p\n", n, tr[0], dir);
}

void Cmd_io(char *tr[])
{
    if (tr[0] == NULL)
        printf("uso: i-o [read|write] [-o] f addr cnt\n");
    else if (!strcmp(tr[0], "read"))  IORead(tr + 1);
    else if (!strcmp(tr[0], "write")) IOWrite(tr + 1);
    else
        printf("uso: i-o [read|write] [-o] f addr cnt\n");
}

/* Variables externas (globales sin static): .data las inicializadas, .bss la otra */
int ext_ini1 = 1;
int ext_ini2 = 2;
int ext_ini3 = 3;
int ext_noini1;
int ext_noini2;
int ext_noini3;

static void MemoriaFuncs(void)
{
    /* El cast a void * de un puntero a funcion no es ISO C estricto, pero
       gcc lo acepta sin avisos con -Wall -Wextra (solo -Wpedantic se queja) */
    printf("%-20s%18p,%18p,%18p\n", "Funciones programa",
           (void *) Cmd_pid, (void *) Cmd_date, (void *) Cmd_list);
    printf("%-20s%18p,%18p,%18p\n", "Funciones libreria",
           (void *) printf, (void *) malloc, (void *) strcmp);
}

static void MemoriaVars(void)
{
    int aut_ini1 = 1, aut_ini2 = 2;
    int aut_noini;                      /* solo usamos su direccion, no su valor */
    static int est_ini1 = 1, est_ini2 = 2, est_ini3 = 3;
    static int est_noini1, est_noini2, est_noini3;

    printf("%-20s%18p,%18p,%18p\n", "Variables locales",
           (void *) &aut_ini1, (void *) &aut_ini2, (void *) &aut_noini);
    printf("%-20s%18p,%18p,%18p\n", "Variables globales",
           (void *) &ext_ini1, (void *) &ext_ini2, (void *) &ext_ini3);
    printf("%-20s%18p,%18p,%18p\n", "Var (N.I.)globales",
           (void *) &ext_noini1, (void *) &ext_noini2, (void *) &ext_noini3);
    printf("%-20s%18p,%18p,%18p\n", "Variables estaticas",
           (void *) &est_ini1, (void *) &est_ini2, (void *) &est_ini3);
    printf("%-20s%18p,%18p,%18p\n", "Var (N.I.)estaticas",
           (void *) &est_noini1, (void *) &est_noini2, (void *) &est_noini3);
}

static void MemoriaPmap(void)
{
    pid_t pid;
    char elpid[32];

    snprintf(elpid, sizeof(elpid), "%d", (int) getpid());
    fflush(stdout);                     /* si no, el hijo hereda el buffer y lo imprime otra vez */
    if ((pid = fork()) == -1) {
        perror("Imposible crear proceso");
        return;
    }
    if (pid == 0) {
        execlp("pmap", "pmap", elpid, (char *) NULL);
        perror("Imposible ejecutar pmap");   /* solo llegamos aqui si exec falla */
        exit(255);
    }
    waitpid(pid, NULL, 0);
}

void Cmd_memory(char *tr[])
{
    int i, bloques = 0, funcs = 0, vars = 0, pmap = 0;

    if (tr[0] == NULL)                  /* sin argumentos: comprueba que hace el de referencia */
        bloques = funcs = vars = 1;
    for (i = 0; tr[i] != NULL; i++) {
        if      (!strcmp(tr[i], "-blocks")) bloques = 1;
        else if (!strcmp(tr[i], "-funcs"))  funcs = 1;
        else if (!strcmp(tr[i], "-vars"))   vars = 1;
        else if (!strcmp(tr[i], "-all"))    bloques = funcs = vars = 1;
        else if (!strcmp(tr[i], "-pmap"))   pmap = 1;
        else {
            printf("uso: memory [-blocks|-funcs|-vars|-all|-pmap]\n");
            return;
        }
    }
    if (vars)    MemoriaVars();
    if (funcs)   MemoriaFuncs();
    if (bloques) mostrarBloques(LISTAR_TODO);
    if (pmap)    MemoriaPmap();
}

//read and write
void Cmd_read(char *tr[])
{
    void *dir;
    int df;
    size_t cont;
    ssize_t n;

    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("uso: read df addr cont\n");
        return;
    }
    df = atoi(tr[0]);
    if (!ParsearDireccion(tr[1], &dir)) {
        printf("Direccion invalida: %s\n", tr[1]);
        return;
    }
    cont = (size_t) strtoul(tr[2], NULL, 10);

    n = read(df, dir, cont);
    if (n == -1)
        printf("Error al leer del descriptor %d a la direccion %p: %s\n",
               df, dir, strerror(errno));
    else
        printf("Leidos %zd bytes del descriptor %d en la direccion %p\n", n, df, dir);
}

void Cmd_write(char *tr[])
{
    void *dir;
    int df;
    size_t cont;
    ssize_t n;

    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        printf("uso: write df addr cont\n");
        return;
    }
    df = atoi(tr[0]);
    if (!ParsearDireccion(tr[1], &dir)) {
        printf("Direccion invalida: %s\n", tr[1]);
        return;
    }
    cont = (size_t) strtoul(tr[2], NULL, 10);

    n = write(df, dir, cont);
    if (n == -1)
        printf("Error al escribir en el descriptor %d desde la direccion %p: %s\n",
               df, dir, strerror(errno));
    else
        printf("Escritos %zd bytes en el descriptor %d desde la direccion %p\n", n, df, dir);
}

//memfill + memdump

void Cmd_memfill(char *tr[])
{
    void *dir;
    unsigned char *p;
    unsigned long cont = 128, valor = 'A';
    size_t i;
    char *fin;

    if (tr[0] == NULL) {
        printf("uso: memfill addr [cont] [hcode]\n");
        return;
    }
    if (!ParsearDireccion(tr[0], &dir)) {
        printf("Direccion invalida: %s\n", tr[0]);
        return;
    }
    if (tr[1] != NULL)
        cont = strtoul(tr[1], NULL, 10);
    if (tr[1] != NULL && tr[2] != NULL) {
        if (strlen(tr[2]) == 3 && tr[2][0] == '\'' && tr[2][2] == '\'')
            valor = (unsigned char) tr[2][1];
        else {
            valor = strtoul(tr[2], &fin, 16);
            if (*fin != '\0' || valor > 0xFF) {
                printf("hcode debe estar entre 0x00 y 0xff\n");
                return;
            }
        }
    }
    printf("Llenando %lu bytes de memoria con el byte %2c(%02lx) a partir de la direccion %p\n",
           cont, isprint((int) valor) ? (int) valor : ' ', valor, dir);
    //El la funcion de LlenarMemoria de ayudaP2
    p = (unsigned char *) dir;
    for (i = 0; i < cont; i++)
        p[i] = (unsigned char) valor;
}

void Cmd_memdump(char *tr[])
{
    void *dir;
    const unsigned char *p;
    size_t cont = 20, i, j;

    if (tr[0] == NULL) {
        printf("uso: memdump addr [cont]\n");
        return;
    }
    if (!ParsearDireccion(tr[0], &dir)) {
        printf("Direccion invalida: %s\n", tr[0]);
        return;
    }
    if (tr[1] != NULL)
        cont = (size_t) strtoul(tr[1], NULL, 10);
    p = (const unsigned char *) dir;

    printf("Volcando %zu bytes desde la direccion %p\n", cont, dir);
    for (i = 0; i < cont; i += BYTES_POR_LINEA) {
        printf("%p-> ", (void *) (p + i));
        for (j = i; j < i + BYTES_POR_LINEA && j < cont; j++) {
            if      (p[j] == '\n') printf(" \\n");
            else if (p[j] == '\t') printf(" \\t");
            else if (p[j] == '\r') printf(" \\r");
            else printf("  %c", isprint(p[j]) ? p[j] : ' ');
        }
        printf("\n%p-> ", (void *) (p + i));
        for (j = i; j < i + BYTES_POR_LINEA && j < cont; j++)
            printf(" %02x", p[j]);
        printf("\n");
    }
}