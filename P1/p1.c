/*
 * Sistemas Operativos - P1
 * Autores:
 *   Ruben Varela Tarrio - ruben.varela.tarrio@udc.es
 *   Ali Abu-afash Nayef - ali.nayef@udc.es
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>        //para date
#include <sys/utsname.h> //para sysinfo
#include <pwd.h> //para obtner nombre de usuario a partir del uid
#include <grp.h> //para obtner nombre de grupo a partir del gid
#include <limits.h> //para PATH_MAX

#define MAXENTRADA  2048
#define MAXNOMBRE 1024
#define OPT_LONG 0x01
#define OPT_LINK 0x02
#define OPT_ACC 0x04
#define OPT_HID  0x08
#define OPT_RECA 0x10
#define OPT_RECB 0x20

typedef struct {
    const char *nombre;
    const char *uso;
    const char *descripcion;
} tHelp;

//Lista 
typedef struct tFichero {
    int descriptor;
    char nombre[MAXNOMBRE];
    int modo;
    int abierto;
} tFichero;

tFichero *tabla = NULL;
int numEntradas = 0;  // tamaño actual del array (== capacidad usada)

//Funciones

void MostrarDirActual()
{
   char dir[MAXNOMBRE];
   
    if (getcwd(dir,MAXNOMBRE)==NULL)
	perror("Imposible obtener directorio");
    else
       printf ("%s\n",dir);
}

int EsDirectorio(const char *dir)
{
    struct stat s;
    if (lstat(dir, &s) == -1)   /* si no puedo acceder, para mi no es directorio */
        return 0;
    return S_ISDIR(s.st_mode);
}

int ComprobarSegundoPlano (char *tr[])
{
    int i;
    for (i=0; tr[i]!=NULL;i++)
        if (!strcmp(tr[i],"&")){ /*& indica segundo plano*/ 
            tr[i]=NULL;         /*es el ultimo argumento*/
            return i;       /*si solo hay un & no se ejecuta nada en pplano*/
            }
    return 0;
}

int BorrarRecursivo(const char *nombre)   /* 0 = bien, -1 = error (con errno) */
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
            err = errno;                     /* closedir podría cambiar errno */
            closedir(d);
            errno = err;
            return -1;                       /* ...y al primer error, abortamos */
        }
    }
    closedir(d);
    return rmdir(nombre);                    /* ...y al final el padre */
}

void Proceso (char *tr[], int splano)
{
   pid_t pid;
   void Cmd_exec (char **);
   int background=splano || ComprobarSegundoPlano(tr);
   if ((pid=fork())==-1){
        perror ("Imposible crear proceso");
        return;
        }
  if (pid==0){  /*proceso hijo*/
    Cmd_exec (tr);
    exit(255); /*por si falla exec*/
    }
  if (!background) 
    waitpid(pid,NULL,0);
}

//funciones de la tabla
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
    if (descriptor >= numEntradas) {
        tabla = realloc(tabla, (descriptor + 1) * sizeof(tFichero)); 
        if (tabla == NULL) {
            perror("Imposible ampliar tabla");
            return;    
        }
        numEntradas = descriptor + 1; 
    }
    tabla[descriptor].descriptor = descriptor; 
    strcpy(tabla[descriptor].nombre, nombre); 
    tabla[descriptor].modo = modo; 
    tabla[descriptor].abierto = 1; 
}

void ListarFicherosAbiertos(void) {
    int i; 
    for (i = 0; i < numEntradas; i++) {
        if (tabla[i].abierto){
            printf("Descriptor: %d, fichero: %s\n", tabla[i].descriptor, tabla[i].nombre);
        }
    }
}


/*********************************************/
/*************COMANDOS DEL SHELL************************/
  
void Cmd_authors(char *tr[]) {
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

void Cmd_exec (char *tr[])
{
  if (execvp(tr[0],tr)==-1)
	perror ("Imposible ejecutar");
}

void Cmd_splano (char *arg[])
{
  Proceso (arg,1);
}
void Cmd_pplano (char *arg[])
{
  Proceso(arg,0);
}

void Cmd_chdir (char * dir)
{
   if (dir==NULL)
      MostrarDirActual();
   else if (chdir(dir)==-1)
      perror("Imposible cambiar directorio");
}

void Cmd_pwd()
{
    MostrarDirActual();
}

void Cmd_pid (char * arg)
{
    if (arg==NULL)
        printf ("El pid del proceso es %d\n",(int) getpid());
    else
        if (!strcmp (arg,"-p"))
            printf ("El pid del proceso padre es %d\n",(int) getppid());
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

void Cmd_close(char *tr[]) //si haces close (df) entra en un bucle
{
    int df;

    if (tr[0] == NULL || (df=atoi(tr[0]))<0)  {          // sin argumento -> listar, mismo patrón que Cmd_open
        ListarFicherosAbiertos();
        return;
    }

    df = atoi(tr[0]);

    if (df < 0 || df >= numEntradas || !tabla[df].abierto) {
        printf("Descriptor %d no esta abierto\n", df);
        return;
    }

    if (close(df) == -1) {
        perror("Imposible cerrar descriptor");
        return;
    }

    tabla[df].abierto = 0;        // liberamos la entrada, no la borramos del array
    printf("Cerrado descriptor %d\n", df);
}

void Cmd_dup(char *tr[])
{
    int df, duplicado;
    char aux[MAXNOMBRE + 32]; // buffer para el nombre del fichero duplicado
    const char *nombreOriginal;

    if (tr[0] == NULL || (df = atoi(tr[0])) < 0) {
        ListarFicherosAbiertos();
        return;
    }

    if (df < numEntradas && tabla[df].abierto)
        nombreOriginal = tabla[df].nombre;
    else
        nombreOriginal = "?";

    if ((duplicado = dup(df)) == -1) {
        perror("Imposible duplicar descriptor");
        return;
    }

    snprintf(aux, sizeof(aux), "dup %d (%s)", df, nombreOriginal);
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
        fd = open(tr[i], O_CREAT | O_WRONLY | O_TRUNC, 0666);
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

    if (uname(&u) == -1 ) {
        perror("Imposible obtener información del sistema");
        return;
    }
    printf("%s (%s), OS: %s-%s-%s\n", 
        u.nodename, u.machine, u.sysname, u.release, u.version); 
}

char LetraTF (mode_t m)
{
     switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */ 
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
     }
}

char * ConvierteModo (mode_t m, char *permisos)
{
    strcpy (permisos,"---------- ");
    
    permisos[0]=LetraTF(m);
    if (m&S_IRUSR) permisos[1]='r';    /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r';    /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r';    /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s';    /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
    
    return permisos;
}

void ImprimirInfoObjeto(const char *ruta, const char *nombreMostrar, int flags)
{
    struct stat s;
    char permisos[11];
    struct passwd *pw;
    struct group  *gr;
    char tbuf[32];
    time_t t;
 
    if (lstat(ruta, &s) == -1) {
        fprintf(stderr, " ****error al acceder a %s:%s\n", ruta, strerror(errno));
        return;
    }
 
    if (!(flags & OPT_LONG)) {
        printf("%8ld  %s\n", (long) s.st_size, nombreMostrar);
    } else {
        ConvierteModo(s.st_mode, permisos);
        pw = getpwuid(s.st_uid);
        gr = getgrgid(s.st_gid);
        t = (flags & OPT_ACC) ? s.st_atime : s.st_ctime;
        strftime(tbuf, sizeof(tbuf), "%Y/%m/%d-%H:%M", localtime(&t));
        printf("%s%4ld (%8ld)%9s%9s %s%9ld %s\n",
               tbuf, (long) s.st_nlink, (long) s.st_ino,
               pw ? pw->pw_name : "?", gr ? gr->gr_name : "?",
               permisos, (long) s.st_size, nombreMostrar);
    }
 
    if ((flags & OPT_LINK) && S_ISLNK(s.st_mode)) {
        char destino[PATH_MAX];
        ssize_t n = readlink(ruta, destino, sizeof(destino) - 1);
        if (n != -1) {
            destino[n] = '\0';
            printf("    -> %s\n", destino);
        }
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

void RecursarEnSubdirectorios(const char *ruta, char *nombres[], int n, int flags);

void ListarDirectorioRec(const char *ruta, int flags)
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

void RecursarEnSubdirectorios(const char *ruta, char *nombres[], int n, int flags)
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

/**************************SHELL**************************/


/* v3
*static struct COMANDO C[]={ no declaro dimension, que la coge de la inicializacion
*{"fin",Cmd_fin},
*{"exit",Cmd_fin},
*{"quit",Cmd_fin},
*{"pid",Cmd_pid},
*{"pwd", Cmd_pwd},
*{"chdir",Cmd_chdir},
*{"autores",Cmd_autores},
*{"exec",Cmd_exec},
*{"pplano",Cmd_pplano},
*{"splano",Cmd_splano},
*{NULL,NULL},             NULL marca el final del array
*};
*/ 


void DecidirComando(char *tr[])
{
  if (tr[0]==NULL)  /*por si cambiamos lo de TroearCadena==0*/
    return;         /*no hace falta que ya comprobamos que TrocearCadena no devielve 0*/
  if (!strcmp(tr[0],"fin")) exit(0);
  else if (!strcmp(tr[0],"quit")) exit(0);
  else if (!strcmp(tr[0],"exit")) exit(0);	
  else if (!strcmp(tr[0],"bye")) exit(0);	//nuevo
  else if (!strcmp(tr[0],"authors")) Cmd_authors(tr+1);
  else if (!strcmp(tr[0],"exec")) Cmd_exec(tr+1);
  else if (!strcmp(tr[0],"pplano")) Cmd_pplano(tr+1);
  else if (!strcmp(tr[0],"splano")) Cmd_splano(tr+1);
  else if (!strcmp(tr[0],"chdir")) Cmd_chdir (tr[1]);
  else if (!strcmp(tr[0],"pwd")) Cmd_pwd ();
  else if (!strcmp(tr[0],"pid")) Cmd_pid (tr[1]);
  else if (!strcmp(tr[0], "open"))  Cmd_open(tr + 1); //nuevo
  else if (!strcmp(tr[0], "listopen")) ListarFicherosAbiertos(); //nuevo
  else if (!strcmp(tr[0], "close")) Cmd_close(tr + 1); //nuevo
  else if (!strcmp(tr[0], "dup")) Cmd_dup(tr + 1); //nuevo
  else if (!strcmp(tr[0], "lseek")) Cmd_lseek(tr + 1); //nuevo
  else if (!strcmp(tr[0], "readstr")) Cmd_readstr(tr + 1); //nuevo
  else if (!strcmp(tr[0], "writestr")) Cmd_writestr(tr + 1); //nuevo
  else if (!strcmp(tr[0], "makefile")) Cmd_makefile(tr + 1); //nuevo
  else if (!strcmp(tr[0], "makedir"))  Cmd_makedir(tr + 1); //nuevo
  else if (!strcmp(tr[0], "date")) Cmd_date(tr+1); //nuevo
  else if (!strcmp(tr[0], "sysinfo")) Cmd_sysinfo(tr + 1); 
  else if (!strcmp(tr[0], "listfile")) Cmd_listfile(tr + 1); //nuevo
  else if (!strcmp(tr[0], "list")) Cmd_list(tr + 1); //nuevo
  else if (!strcmp(tr[0], "delete")) Cmd_delete(tr + 1); //nuevo
  else if (!strcmp(tr[0], "deltree")) Cmd_deltree(tr + 1); //nuevo
  else if (!strcmp(tr[0], "help")) Cmd_help(tr + 1);//nuevo

  else Cmd_pplano(tr);
}

int TrocearCadena(char * cadena, char * trozos[])
{
  int i=1;
  if ((trozos[0]=strtok(cadena," \n\t"))==NULL)
      return 0;
  while ((trozos[i]=strtok(NULL," \n\t"))!=NULL)
     i++;
  return i;
}	
void ProcesarEntrada(char * entrada)
{
   char *tr[MAXENTRADA/2];
   if (TrocearCadena(entrada,tr)==0) /*no hay nada*/
	return;
   DecidirComando(tr);
}

int main(int argc, char *argv[], char *ent[])
{
   char entrada[MAXENTRADA];
   InicializarTabla(); //n

   while (1){
      printf ("-> ");
      fgets(entrada,MAXENTRADA,stdin); //si pulsas ctrl+d en la terminal da un bucle raro
      ProcesarEntrada(entrada);
   }
}

