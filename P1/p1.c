#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAXENTRADA  2048
#define MAXNOMBRE 1024

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
  
void Cmd_autores(char *tr[]) {
  const char *nombres[] = { "Ruben Varela Tarrio", "Nombre2" };
    const char *logins[]  = { "ruben.varela.tarrio@udc.es", "Login2" };
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

void Cmd_close(char *tr[])
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



/** int open(const char *path, int flags, ...
 mode_t mode  );**/




/**************************SHELL**************************/
void DecidirComando(char *tr[])
{
  if (tr[0]==NULL)  /*por si cambiamos lo de TroearCadena==0*/
    return;         /*no hace falta que ya comprobamos que TrocearCadena no devielve 0*/
  if (!strcmp(tr[0],"fin")) exit(0);
  else if (!strcmp(tr[0],"quit")) exit(0);
  else if (!strcmp(tr[0],"exit")) exit(0);	
  else if (!strcmp(tr[0],"autores")) Cmd_autores(tr+1);
  else if (!strcmp(tr[0],"exec")) Cmd_exec(tr+1);
  else if (!strcmp(tr[0],"pplano")) Cmd_pplano(tr+1);
  else if (!strcmp(tr[0],"splano")) Cmd_splano(tr+1);
  else if (!strcmp(tr[0],"chdir")) Cmd_chdir (tr[1]);
  else if (!strcmp(tr[0],"pwd")) Cmd_pwd ();
  else if (!strcmp(tr[0],"pid")) Cmd_pid (tr[1]);
  else if (!strcmp(tr[0], "open"))  Cmd_open(tr + 1); //nuevo
  else if (!strcmp(tr[0], "listopen")) ListarFicherosAbiertos(); //nuevo

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
      fgets(entrada,MAXENTRADA,stdin);
      ProcesarEntrada(entrada);
   }
}
