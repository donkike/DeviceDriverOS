#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

#define SUCCESS 0
#define DEV_NAME "cheater"
#define CHEATER_MINORS 1
#define KERNEL_SECTOR_SIZE 512

static int cheater_major = 0;
static char* cheater_name = DEV_NAME;
static int nsectors = 10;
static int hardsect_size = 512;
static int which = 0;

struct cheater_dev {
  int size;
  u8 *data;
  spinlock_t lock;
  struct request_queue *queue;
  struct gendisk *gd;
};

static void cheater_request(struct request_queue_t *q) {
  struct request *req;
  
}

static int cheater_open(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "executing cheater_open");
  struct cheater_dev *dev = inode->i_bdev->bd_disk->private_data;
  filp->private_data = dev;
  return SUCCESS;
}

static int cheater_release(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "executing cheater_release");
  struct cheater_dev *dev = inode->i_bdev->bd_disk->private_data;
  return SUCCESS;
}

static struct cheater_dev *dev;

static int __init init_driver(void) {
  struct cheater_dev myDev;
  dev = &myDev;
  cheater_major = register_blkdev(cheater_major, cheater_name);
  if (cheater_major <= 0) {
    printk(KERN_WARNING "Problem registering device %s\n", cheater_name);
    return -EBUSY;
  }
  printk(KERN_INFO "Block device %s registered with major %d\n", cheater_name, cheater_major);
  memset(dev, 0, sizeof(struct cheater_dev));
  printk(KERN_INFO "dev initialized with memset\n");
  dev->size = nsectors * hardsect_size;
  printk(KERN_INFO "defined dev size\n");
  dev->data = vmalloc(dev->size);
  printk(KERN_INFO "dev data vmalloc'd\n");
  if (dev->data == NULL) {
    printk(KERN_WARNING "vmalloc failure\n");
    return -1;
  }
  spin_lock_init(&dev->lock);
  printk(KERN_INFO "cheater_dev initialized\n");
  dev->queue = blk_init_queue(cheater_request, &dev->lock);
  if (dev->queue == NULL) {
    printk(KERN_WARNING "error allocating request queue\n");
    return -1;
  }
  dev->gd = alloc_disk(CHEATER_MINORS);
  if (! dev->gd) {
    printk(KERN_WARNING "alloc_disk failure\n");
    goto out_vfree;
  }
  printk(KERN_INFO "cheater disk allocated\n");
  dev->gd->major = cheater_major;
  dev->gd->first_minor = which * CHEATER_MINORS;
  //dev->gd->fops = &cheater_ops;
  dev->gd->queue = dev->queue;
  dev->gd->private_data = dev;
  snprintf(dev->gd->disk_name, 32, "cheater%c", which + 'a');
  printk(KERN_INFO "disk_name is %s\n", dev->gd->disk_name);
  set_capacity(dev->gd, nsectors * (hardsect_size / KERNEL_SECTOR_SIZE));
  add_disk(dev->gd);
  printk(KERN_INFO "disk_device %s added\n", dev->gd->disk_name);
  return SUCCESS;
 out_vfree:
  return -1;
}

static void __exit exit_driver(void) {
  unregister_blkdev(cheater_major, cheater_name);
  printk(KERN_INFO "Block device %s with major %d removed", cheater_name, cheater_major);
}

module_init(init_driver);
module_exit(exit_driver);

MODULE_LICENSE("Dual BSD/GPL");


