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

Referencias
-----------
 * "An introduction to block device drivers" - Linux Journal
	http://www.linuxjournal.com/article/2890
 * "Linux Device Drivers, 3rd Edition" - Corbet, J.; Rubini, A.; Kroah-Hartman, G.
	http://lwn.net/Kernel/LDD3/
 * "A simple block driver" - Corbet
	https://lwn.net/Articles/58719/
 * "A Simple Block Driver for Linux Kernel 2.6.31" - Pat Patterson
	http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
 * "kernelHow to read/write files within kernel module?" - StackOverflow
	http://stackoverflow.com/questions/1184274/kernelhow-to-read-write-files-within-kernel-module
