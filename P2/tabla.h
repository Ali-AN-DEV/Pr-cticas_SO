#ifndef TABLA_H
#define TABLA_H

void InicializarTabla(void);
void AnadirFichero(int descriptor, const char *nombre, int modo);
void LiberarTabla(void);
void ListarFicherosAbiertos(void);

/* Consultas/modificaciones: el resto del programa no toca la tabla directamente */
int         DescriptorAbierto(int df);   /* 1 si df esta en la tabla y abierto */
const char *NombreDescriptor(int df);    /* nombre guardado, o "?" si no esta abierto */
void        MarcarCerrado(int df);

#endif
