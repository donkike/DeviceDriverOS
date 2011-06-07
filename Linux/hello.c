#include<linux/init.h>
#include<linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void){
  printk(KERN_ALERT "Hola, Mundo\n");
  return 0;
}
static void hello_exit(void){
  printk(KERN_ALERT "Adios mundo cruel\n");
}

module_init(hello_init);
module_exit(hello_exit);
