#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/types.h>
#include <linux/syscalls.h>

#define AUTHOR1 "Enrique Arango Lyons"
#define AUTHOR2 "Santiago Achury Jaramillo"
#define AUTHOR3 "Juan Camilo Cardona Cano"

#define SUCCESS 0
#define DEV_NAME "cheater"
#define CHEATER_MINORS 1
#define KERNEL_SECTOR_SIZE 512

static int cheater_major = 0;
static char* cheater_name = DEV_NAME;
static int nsectors = 10;
static int hardsect_size = 512;
static int which = 0;
static int fd;
static char* filename;
static mm_segment_t old_fs;

struct cheater_dev {
  int size;
  u8 *data;
  spinlock_t lock;
  struct request_queue *queue;
  struct gendisk *gd;
};

static void cheater_transfer(struct cheater_dev *dev, unsigned long sector, unsigned long nsect, char *buffer, int write) {
  printk(KERN_INFO "transferring data request");
  unsigned long offset = sector * KERNEL_SECTOR_SIZE;
  unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
  if ((offset + nbytes) > dev->size){ 
    printk(KERN_NOTICE "Beyond end-write (%ld %ld)\n", offset, nbytes);
    return;
  }
  if (write)
    memcpy(dev->data + offset, buffer, nbytes);
  else
    memcpy(buffer, dev->data + offset, nbytes);
}

static void cheater_request(struct request_queue_t *q) {
  printk(KERN_INFO "a request has arrived");
  struct request *req;
  while((req = blk_fetch_request(q)) != NULL){
    struct cheater_dev *dev = req->rq_disk->private_data;
    if(!blk_fs_request(req)){
      printk(KERN_NOTICE "skip non-fs request\n");
      blk_end_request_cur(req, -1);
      continue;
    }
    cheater_transfer(dev, blk_rq_pos(req), blk_rq_cur_sectors(req), req->buffer, rq_data_dir(req));
    blk_end_request_cur(req, 0);
  }
}

static int cheater_open(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "executing cheater_open\n");
  struct cheater_dev *dev = inode->i_bdev->bd_disk->private_data;
  printk(KERN_INFO "initialized cheater_dev\n");
  //filp->private_data = dev;
  printk(KERN_INFO "got private data from filp\n");
  //fd = sys_open(filename, O_RDONLY | O_WRONLY, 0644);
  old_fs = get_fs();
  printk(KERN_INFO "obtained old_fs through get_fs()\n");
  set_fs(KERNEL_DS);
  printk(KERN_INFO "called set_fs() for KERNEL_DS");
  return SUCCESS;
}

static int cheater_release(struct inode *inode, struct file *filp) {
  printk(KERN_INFO "executing cheater_release\n");
  struct cheater_dev *dev = inode->i_bdev->bd_disk->private_data;
  sys_close(fd);
  set_fs(old_fs);
  return SUCCESS;
}

static struct cheater_dev *dev;

struct block_device_operations cheater_ops = {
  .owner = THIS_MODULE,
  .open = cheater_open,
  .release = cheater_release,
};

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
  dev->gd->fops = &cheater_ops;
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

MODULE_AUTHOR(AUTHOR1);
MODULE_AUTHOR(AUTHOR2);
MODULE_AUTHOR(AUTHOR3);

