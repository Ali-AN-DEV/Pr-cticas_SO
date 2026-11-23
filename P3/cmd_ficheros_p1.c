/* Comandos de ficheros y directorios: open, close, dup, lseek, readstr,
   writestr, makefile, makedir, listfile, list, delete, deltree */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

#include "comun.h"
#include "tabla_p1.h"
#include "cmd_general_p1.h"
#include "cmd_ficheros_p1.h"

#define OPT_LONG 0x01
#define OPT_LINK 0x02
#define OPT_ACC  0x04
#define OPT_HID  0x08
#define OPT_RECA 0x10
#define OPT_RECB 0x20

static int EsDirectorio(const char *dir)
{
    struct stat s;
    if (lstat(dir, &s) == -1)   /* si no puedo acceder, para mi no es directorio */
        return 0;
    return S_ISDIR(s.st_mode);
}

void Cmd_open(char *tr[])
{
    int i;
    int df;
    int modo = 0;

    if (tr[0] == NULL) {
        ListarFicherosAbiertos();
        return;
    }

    for (i = 1; tr[i] != NULL; i++) {
        if (!strcmp(tr[i], "cr"))
            modo |= O_CREAT;
        else if (!strcmp(tr[i], "ex"))
            modo |= O_EXCL;
        else if (!strcmp(tr[i], "ro"))
            modo |= O_RDONLY;
        else if (!strcmp(tr[i], "wo"))
            modo |= O_WRONLY;
        else if (!strcmp(tr[i], "rw"))
            modo |= O_RDWR;
        else if (!strcmp(tr[i], "ap"))
            modo |= O_APPEND;
        else if (!strcmp(tr[i], "tr"))
            modo |= O_TRUNC;
        else
            break;
    }

    df = open(tr[0], modo, 0777);

    if (df == -1) {
        perror("Imposible abrir fichero");
        return;
    }

    AnadirFichero(df, tr[0], modo);

    printf("Anadida entrada a la tabla ficheros abiertos %d: %s\n", df, tr[0]);
}

void Cmd_close(char *tr[])
{
    int df;

    if (tr[0] == NULL || (df = atoi(tr[0])) < 0) {   /* sin argumento -> listar, mismo patron que Cmd_open */
        ListarFicherosAbiertos();
        return;
    }

    if (!DescriptorAbierto(df)) {
        printf("Descriptor %d no esta abierto\n", df);
        return;
    }

    if (close(df) == -1) {
        perror("Imposible cerrar descriptor");
        return;
    }

    MarcarCerrado(df);
    printf("Cerrado descriptor %d\n", df);
}

void Cmd_dup(char *tr[])
{
    int df, duplicado;
    char aux[MAXNOMBRE + 32];   /* buffer para el nombre del fichero duplicado */

    if (tr[0] == NULL || (df = atoi(tr[0])) < 0) {
        ListarFicherosAbiertos();
        return;
    }

    if ((duplicado = dup(df)) == -1) {
        perror("Imposible duplicar descriptor");
        return;
    }

    snprintf(aux, sizeof(aux), "dup %d (%s)", df, NombreDescriptor(df));
    AnadirFichero(duplicado, aux, 0);
    printf("df=%d fichero=%s\n", duplicado, aux);
}

void Cmd_lseek(char *tr[])
{
    int df;
    off_t pos, resultado;
    int whence;

    if (tr[0] == NULL || tr[1] == NULL || tr[2] == NULL) {
        fprintf(stderr, "uso: lseek df pos ref\n");
        return;
    }
    df = atoi(tr[0]);
    pos = atoll(tr[1]);

    if (!strcmp(tr[2], "SEEK_SET")) whence = SEEK_SET;
    else if (!strcmp(tr[2], "SEEK_CUR")) whence = SEEK_CUR;
    else if (!strcmp(tr[2], "SEEK_END")) whence = SEEK_END;
    else { fprintf(stderr, "lseek: referencia desconocida %s\n", tr[2]); return; }

    resultado = lseek(df, pos, whence);
    if (resultado == -1)
        perror("Imposible posicionar el descriptor");
    else
        printf("%lld\n", (long long) resultado);
}

void Cmd_readstr(char *tr[])
{
    int df, cont, leidos;
    char *buffer;

    if (tr[0] == NULL || tr[1] == NULL) { fprintf(stderr, "uso: readstr df cont\n"); return; }
    df = atoi(tr[0]);
    cont = atoi(tr[1]);
    if (cont < 0) { fprintf(stderr, "readstr: cantidad invalida\n"); return; }

    buffer = malloc(cont + 1);
    if (buffer == NULL) { perror("malloc"); return; }

    leidos = read(df, buffer, cont);
    if (leidos == -1) {
        perror("Imposible leer del descriptor");
    } else {
        buffer[leidos] = '\0';
        printf("%s\n", buffer);
    }
    free(buffer);
}

void Cmd_writestr(char *tr[])
{
    ssize_t escritos;
    size_t len;

    if (tr[0] == NULL || tr[1] == NULL) { fprintf(stderr, "uso: writestr df str\n"); return; }
    len = strlen(tr[1]);
    escritos = write(atoi(tr[0]), tr[1], len);
    if (escritos == -1)
        perror("Imposible escribir en el descriptor");
    else if ((size_t) escritos < len)
        fprintf(stderr, "writestr: escritura parcial (%zd de %zu bytes)\n", escritos, len);
}

void Cmd_makefile(char *tr[])
{
    int i, fd;
    for (i = 0; tr[i] != NULL; i++) {
        fd = open(tr[i], O_CREAT | O_EXCL | O_WRONLY, 0666);   /* excl es vital */
        if (fd == -1) {
            fprintf(stderr, "makefile: %s: %s\n", tr[i], strerror(errno));
            continue;
        }
        close(fd);
    }
}

void Cmd_makedir(char *tr[])
{
    int i;
    for (i = 0; tr[i] != NULL; i++)
        if (mkdir(tr[i], 0777) == -1)
            fprintf(stderr, "makedir: %s: %s\n", tr[i], strerror(errno));
}

static char LetraTF(mode_t m)
{
    switch (m & S_IFMT) {          /* and bit a bit con los bits de formato, 0170000 */
        case S_IFSOCK: return 's'; /* socket */
        case S_IFLNK:  return 'l'; /* symbolic link */
        case S_IFREG:  return '-'; /* fichero normal */
        case S_IFBLK:  return 'b'; /* block device */
        case S_IFDIR:  return 'd'; /* directorio */
        case S_IFCHR:  return 'c'; /* char device */
        case S_IFIFO:  return 'p'; /* pipe */
        default:       return '?'; /* desconocido, no deberia aparecer */
    }
}

static char *ConvierteModo(mode_t m, char *permisos)
{
    strcpy(permisos, "---------- ");

    permisos[0] = LetraTF(m);
    if (m & S_IRUSR) permisos[1] = 'r';    /* propietario */
    if (m & S_IWUSR) permisos[2] = 'w';
    if (m & S_IXUSR) permisos[3] = 'x';
    if (m & S_IRGRP) permisos[4] = 'r';    /* grupo */
    if (m & S_IWGRP) permisos[5] = 'w';
    if (m & S_IXGRP) permisos[6] = 'x';
    if (m & S_IROTH) permisos[7] = 'r';    /* resto */
    if (m & S_IWOTH) permisos[8] = 'w';
    if (m & S_IXOTH) permisos[9] = 'x';
    if (m & S_ISUID) permisos[3] = 's';    /* setuid, setgid y stickybit */
    if (m & S_ISGID) permisos[6] = 's';
    if (m & S_ISVTX) permisos[9] = 't';

    return permisos;
}

static void ImprimirInfoObjeto(const char *ruta, const char *nombreMostrar, int flags)
{
    struct stat s;
    char permisos[12];   /* necesita 12 para no tener errores */
    struct passwd *pw;
    struct group  *gr;
    char tbuf[32];
    time_t t;
    char destino[PATH_MAX];
    char flecha[PATH_MAX + 8] = "";   /* " -> destino" o cadena vacia */
    ssize_t n;

    if (lstat(ruta, &s) == -1) {
        fprintf(stderr, " ****error al acceder a %s:%s\n", ruta, strerror(errno));
        return;
    }

    /* Primero preparamos la flecha (si procede), luego imprimimos UNA sola linea */
    if ((flags & OPT_LINK) && S_ISLNK(s.st_mode)) {
        n = readlink(ruta, destino, sizeof(destino) - 1);
        if (n != -1) {
            destino[n] = '\0';                       /* readlink NO pone el \0 */
            snprintf(flecha, sizeof(flecha), " -> %s", destino);
        }
    }

    if (!(flags & OPT_LONG)) {
        printf("%8ld  %s%s\n", (long) s.st_size, nombreMostrar, flecha);
    } else {
        ConvierteModo(s.st_mode, permisos);
        pw = getpwuid(s.st_uid);
        gr = getgrgid(s.st_gid);
        t = (flags & OPT_ACC) ? s.st_atime : s.st_mtime;
        strftime(tbuf, sizeof(tbuf), "%Y/%m/%d-%H:%M", localtime(&t));
        printf("%s%4ld (%8ld)%9s%9s %s%9ld %s%s\n",
               tbuf, (long) s.st_nlink, (long) s.st_ino,
               pw ? pw->pw_name : "?", gr ? gr->gr_name : "?",
               permisos, (long) s.st_size, nombreMostrar, flecha);
    }
}

void Cmd_listfile(char *tr[])
{
    int i, flags = 0;

    for (i = 0; tr[i] != NULL; i++) {
        if      (!strcmp(tr[i], "-long")) flags |= OPT_LONG;
        else if (!strcmp(tr[i], "-link")) flags |= OPT_LINK;
        else if (!strcmp(tr[i], "-acc"))  flags |= OPT_ACC;
        else break;
    }
    if (tr[i] == NULL) { fprintf(stderr, "uso: listfile [-long][-link][-acc] name...\n"); return; }
    for (; tr[i] != NULL; i++)
        ImprimirInfoObjeto(tr[i], tr[i], flags);
}

static void RecursarEnSubdirectorios(const char *ruta, char *nombres[], int n, int flags);

static void ListarDirectorioRec(const char *ruta, int flags)
{
    DIR *d;
    struct dirent *ent;
    char **nombres = NULL;
    int n = 0, capacidad = 0, i;
    char hijo[PATH_MAX];

    d = opendir(ruta);
    if (d == NULL) {
        fprintf(stderr, " ****error al acceder a %s:%s\n", ruta, strerror(errno));
        return;
    }

    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.' && !(flags & OPT_HID))
            continue;
        if (n == capacidad) {
            capacidad = capacidad ? capacidad * 2 : 8;
            nombres = realloc(nombres, capacidad * sizeof(char *));
            if (nombres == NULL) { perror("realloc"); closedir(d); return; }
        }
        nombres[n] = strdup(ent->d_name);
        if (nombres[n] == NULL) { perror("strdup"); break; }
        n++;
    }
    closedir(d);

    if (flags & OPT_RECB)
        RecursarEnSubdirectorios(ruta, nombres, n, flags);

    printf("************%s\n", ruta);
    for (i = 0; i < n; i++) {
        snprintf(hijo, sizeof(hijo), "%s/%s", ruta, nombres[i]);
        ImprimirInfoObjeto(hijo, nombres[i], flags);
    }

    if (flags & OPT_RECA)
        RecursarEnSubdirectorios(ruta, nombres, n, flags);

    for (i = 0; i < n; i++) free(nombres[i]);
    free(nombres);
}

static void RecursarEnSubdirectorios(const char *ruta, char *nombres[], int n, int flags)
{
    int i;
    char hijo[PATH_MAX];

    for (i = 0; i < n; i++) {
        if (!strcmp(nombres[i], ".") || !strcmp(nombres[i], ".."))
            continue;
        snprintf(hijo, sizeof(hijo), "%s/%s", ruta, nombres[i]);
        if (EsDirectorio(hijo))
            ListarDirectorioRec(hijo, flags);
    }
}

void Cmd_list(char *tr[])
{
    int i, flags = 0;

    for (i = 0; tr[i] != NULL; i++) {
        if      (!strcmp(tr[i], "-long")) flags |= OPT_LONG;
        else if (!strcmp(tr[i], "-link")) flags |= OPT_LINK;
        else if (!strcmp(tr[i], "-acc"))  flags |= OPT_ACC;
        else if (!strcmp(tr[i], "-hid"))  flags |= OPT_HID;
        else if (!strcmp(tr[i], "-reca")) flags |= OPT_RECA;
        else if (!strcmp(tr[i], "-recb")) flags |= OPT_RECB;
        else break;
    }
    if (tr[i] == NULL) { fprintf(stderr, "uso: list [opciones] name...\n"); return; }

    for (; tr[i] != NULL; i++) {
        if (EsDirectorio(tr[i]))
            ListarDirectorioRec(tr[i], flags);
        else
            ImprimirInfoObjeto(tr[i], tr[i], flags);
    }
}

static int BorrarRecursivo(const char *nombre)   /* 0 = bien, -1 = error (con errno) */
{
    DIR *d;
    struct dirent *e;
    char ruta[MAXNOMBRE];
    int err;

    if (!EsDirectorio(nombre))
        return unlink(nombre);
    if ((d = opendir(nombre)) == NULL)
        return -1;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
            continue;
        snprintf(ruta, sizeof(ruta), "%s/%s", nombre, e->d_name);
        if (BorrarRecursivo(ruta) == -1) {   /* primero los hijos... */
            err = errno;                     /* closedir podria cambiar errno */
            closedir(d);
            errno = err;
            return -1;                       /* ...y al primer error, abortamos */
        }
    }
    closedir(d);
    return rmdir(nombre);                    /* ...y al final el padre */
}

void Cmd_delete(char *tr[])
{
    int i, r;
    if (tr[0] == NULL) {
        MostrarDirActual();
        return;
    }

    for (i = 0; tr[i] != NULL; i++) {
        r = EsDirectorio(tr[i]) ? rmdir(tr[i]) : unlink(tr[i]);
        if (r == -1)
            printf("Imposible borrar %s: %s\n", tr[i], strerror(errno));
    }
}

void Cmd_deltree(char *tr[])
{
    int i;

    if (tr[0] == NULL) {
        MostrarDirActual();
        return;
    }

    for (i = 0; tr[i] != NULL; i++)
        if (BorrarRecursivo(tr[i]) == -1)
            printf("Imposible borrar %s: %s\n", tr[i], strerror(errno));
}

