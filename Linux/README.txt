Looper
======

Device Driver para montar imagen de disco como dispositivo nodo

Instrucciones
-------------

Para montar:
1. Correr `make`
2. Correr `sudo insmod looper.ko filename=<filename>`
  Nota: Se debe especificar un archivo de imagen <filename>
3. Montar el dispositivo /dev/looper0 en algún directorio

Para desmontar:
1. Desmontar el dispositivo /dev/looper0 del directorio donde se encuentre
2. Correr `sudo rmmod looper`

Más información acerca del driver se puede ver con `modinfo looper.ko`

Integrantes
-----------

 * Santiago Achury Jaramillo
 * Enrique Arango Lyons
 * Juan Camilo Cardona Cano

