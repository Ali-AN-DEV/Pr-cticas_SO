/* Comandos generales del shell: authors, pid, chdir, pwd, date, sysinfo, help */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>

#include "comun.h"
#include "cmd_general_p1.h"

/* No es static: tambien la usan delete/deltree (cmd_ficheros_p1.c) */
void MostrarDirActual(void)
{
    char dir[MAXNOMBRE];

    if (getcwd(dir, MAXNOMBRE) == NULL)
        perror("Imposible obtener directorio");
    else
        printf("%s\n", dir);
}

void Cmd_pid(char *arg)
{
    if (arg == NULL)
        printf("El pid del proceso es %d\n", (int) getpid());
    else if (!strcmp(arg, "-p"))
        printf("El pid del proceso padre es %d\n", (int) getppid());
}

typedef struct {
    const char *nombre;
    const char *uso;
    const char *descripcion;
} tHelp;

void Cmd_authors(char *tr[])
{
    const char *nombres[] = { "Ruben Varela Tarrio", "Ali Abu-afash Nayef" };
    const char *logins[]  = { "ruben.varela.tarrio@udc.es", "ali.nayef@udc.es" };
    int i;

    if (tr[0] != NULL && !strcmp(tr[0], "-l")) {
        for (i = 0; i < 2; i++) printf("%s\n", logins[i]);
    } else if (tr[0] != NULL && !strcmp(tr[0], "-n")) {
        for (i = 0; i < 2; i++) printf("%s\n", nombres[i]);
    } else {
        for (i = 0; i < 2; i++) printf("%s : %s\n", nombres[i], logins[i]);
    }
}

void Cmd_chdir(char *dir)
{
    if (dir == NULL)
        MostrarDirActual();
    else if (chdir(dir) == -1)
        perror("Imposible cambiar directorio");
}

void Cmd_pwd(void)
{
    MostrarDirActual();
}

void Cmd_date(char *arg[])
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char buf[64];

    if (tm == NULL) {
        perror("Imposible obtener la hora");
        return;
    }

    if (arg[0] == NULL || !strcmp(arg[0], "-t")) {
        strftime(buf, sizeof(buf), "%H:%M:%S", tm);
        printf("%s\n", buf);
    }

    if (arg[0] == NULL || !strcmp(arg[0], "-d")) {
        strftime(buf, sizeof(buf), "%d/%m/%Y", tm);
        printf("%s\n", buf);
    }
}

void Cmd_sysinfo(char *arg[])
{
    struct utsname u;
    (void) arg;   /* no usa argumentos */

    if (uname(&u) == -1) {
        perror("Imposible obtener informacion del sistema");
        return;
    }
    printf("%s (%s), OS: %s-%s-%s\n",
           u.nodename, u.machine, u.sysname, u.release, u.version);
}

static const tHelp helps[] = {
    {"exit",     "exit",                               "Termina la ejecucion del shell"},
    {"bye",      "bye",                                "Termina la ejecucion del shell"},
    {"date",     "date [-d|-t]",                       "Muestra fecha y hora. -d solo la fecha, -t solo la hora"},
    {"pid",      "pid [-p]",                           "Muestra el pid del shell. -p muestra el pid de su proceso padre"},
    {"authors",  "authors [-l|-n]",                    "Muestra los autores del shell. -l solo logins, -n solo nombres"},
    {"sysinfo",  "sysinfo",                            "Muestra informacion de la maquina"},
    {"help",     "help [cmd]",                         "Sin argumentos lista los comandos; con cmd, da informacion de ese comando"},
    {"chdir",    "chdir [dir]",                        "Cambia el directorio de trabajo a dir. Sin argumentos muestra el actual"},
    {"open",     "open [file m1 m2...]",               "Abre file con los modos m1, m2... (cr, ap, ex, ro, rw, wo, tr) y lo anade a la lista de abiertos. Sin argumentos lista los abiertos"},
    {"close",    "close [df]",                         "Cierra el descriptor df y lo elimina de la lista de abiertos"},
    {"listopen", "listopen",                           "Lista los ficheros abiertos por el shell"},
    {"dup",      "dup df",                             "Duplica el descriptor df y lo anade a la lista de abiertos"},
    {"lseek",    "lseek df pos ref",                   "Posiciona el cursor de df en pos. ref: SEEK_SET, SEEK_CUR o SEEK_END"},
    {"readstr",  "readstr df cont",                    "Lee cont bytes de df y los muestra como una cadena"},
    {"writestr", "writestr df str",                    "Escribe la cadena str en el fichero descrito por df"},
    {"makefile", "makefile name",                      "Crea un fichero vacio de nombre name"},
    {"makedir",  "makedir name",                       "Crea un directorio de nombre name"},
    {"delete",   "delete name1 name2...",              "Borra ficheros, enlaces o directorios vacios"},
    {"deltree",  "deltree name1 name2...",             "Borra ficheros, enlaces o directorios con todo su contenido"},
    {"listfile", "listfile [-long][-link][-acc] name1 name2...", "Da informacion de los objetos del sistema de ficheros (nombre y tamano). -long listado largo, -link muestra el destino de los enlaces, -acc usa la fecha de ultimo acceso"},
    {"list",     "list [-reca][-recb][-hid][-long][-link][-acc] name1 name2...", "Como listfile, pero si name es un directorio lista su contenido. -hid incluye ocultos, -reca recursivo despues, -recb recursivo antes"},
    {"allocate", "[-malloc|-shared|-createshared|-mmap]...", "Asigna un bloque de memoria\n -malloc tam: asigna un bloque malloc de tamaño tam\n -createshared cl tam: asigna creando un bloque de memoria compartida de clave cl y tamaño tam\n -shared cl: asigna el bloque de memoria compartida ya existente de clave cl\n -mmap fich perm: mapea el fichero fich con los permisos perm\n"},
    {"deallocate", "[-malloc|-shared|-delkey|-mmap|addr]..", "Desasigna un bloque de memoria\n -malloc tam: desasigna el bloque malloc de tamaño tam\n -shared cl: desasigna/desmapea el bloque de memoria compartda de clave cl\n -delkey cl: elimina del sistema (sin desmapear) la clave de memoria cl\n -mmap dich: desmapea el fichero mapeado fich\n addr: desasigna el bloque de memoria de la dirección addr\n"},
    {"i-o", "i-o [read|write] [-o] f addr cnt", "Entrada/salida directa entre ficheros y memoria\n read f addr cnt: lee cnt bytes del fichero f en la direccion addr (hexadecimal)\n write [-o] f addr cnt: escribe cnt bytes desde la direccion addr en el fichero f. Con -o se puede sobrescribir\n"},
    {"memory", "memory [-blocks|-funcs|-vars|-all|-pmap]", "Muestra informacion sobre la memoria del shell\n -blocks: muestra la lista de bloques asignados (malloc, shared y mmap)\n -funcs: muestra las direcciones de tres funciones del programa y tres de libreria\n -vars: muestra las direcciones de tres variables locales, tres estaticas y tres externas\n -all: equivale a -blocks -funcs -vars\n -pmap: muestra el mapa de memoria del proceso (usando pmap)\n"},
    {"read",  "read df addr cont",  "Transfiere cont bytes del fichero descrito por df a la direccion addr (hexadecimal)"},
    {"write", "write df addr cont", "Transfiere cont bytes desde la direccion addr (hexadecimal) al fichero descrito por df"},
    {"memfill", "memfill addr cont hcode", "Llena cont bytes a partir de la direccion addr con el byte hcode (hexadecimal)"},
    {"memdump", "memdump addr cont",       "Vuelca en pantalla cont bytes a partir de la direccion addr, en hexadecimal y como caracteres"},
    {"recurse", "recurse [n]", "Llama de forma recursiva desde n a 0 mostrando en cada nivel las direcciones de memoria automáticas vs estáticas"},
    {NULL, NULL, NULL}
};

void Cmd_help(char *tr[])
{
    int i;

    if (tr[0] == NULL) {
        printf("'help [cmd]' ayuda sobre los comandos\n");
        printf("Comandos disponibles:");
        for (i = 0; helps[i].nombre != NULL; i++)
            printf(" %s", helps[i].nombre);
        printf("\n");
        return;
    }

    for (i = 0; helps[i].nombre != NULL; i++) {
        if (!strcmp(tr[0], helps[i].nombre)) {
            printf("%s: %s\n", helps[i].uso, helps[i].descripcion);
            return;
        }
    }
    printf("%s no encontrado\n", tr[0]);
}

