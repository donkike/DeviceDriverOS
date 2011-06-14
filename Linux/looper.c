#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Santiago Achury Jaramillo");
MODULE_AUTHOR("Enrique Arango Lyons");
MODULE_AUTHOR("Juan Camilo Cardona Cano");
MODULE_DESCRIPTION("Mount an image file as a dev node");

static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, int, 0);
static char* filename = NULL;
module_param(filename, charp, 0000);
MODULE_PARM_DESC(filename, "Image filename to mount");

static struct file* filp;

#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * The internal representation of our device.
 */
static struct looper_device {
  unsigned long size;
  spinlock_t lock;
  u8 *data;
  struct gendisk *gd;
} Device;


/*
 * Open file
 */
struct file* file_open(const char* path, int flags, int rights) {
  
  mm_segment_t oldfs;
  int err = 0;
  
  printk(KERN_INFO "looper: opening file");
  
  oldfs = get_fs();
  set_fs(get_ds());
  filp = filp_open(path, flags, rights);
  if (IS_ERR(filp)) {
    err = PTR_ERR(filp);
    return NULL;
  }
  return filp;
}

/*
 * Close file
 */
void file_close(struct file* filp) {
  printk(KERN_INFO "looper: closing file");
  filp_close(filp, NULL);
}

/*
 * Read file
 */
int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {

  mm_segment_t old_fs;
  int ret;
  
  printk(KERN_INFO "looper: reading file");  

  old_fs = get_fs();
  set_fs(get_ds());
  
  ret = vfs_read(file, data, size, &offset);

  set_fs(old_fs);
  return ret;
}

/*
 * Write to file
 */
int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
  mm_segment_t oldfs;
  int ret;

  printk(KERN_INFO "looper: writing file");
  
  oldfs = get_fs();
  set_fs(get_ds());
  
  ret = vfs_write(file, data, size, &offset);
  
  set_fs(oldfs);
  return ret;
}

/*
 * Handle an I/O request.
 */
static void looper_transfer(struct looper_device *dev, sector_t sector,
			 unsigned long nsect, char *buffer, int write) {
 
  unsigned long offset = sector * logical_block_size;
  unsigned long nbytes = nsect * logical_block_size;
  
  printk(KERN_INFO "looper: executing transfer");
 
  if ((offset + nbytes) > dev->size) {
    printk (KERN_NOTICE "looper: Beyond-end write (%ld %ld)\n", offset, nbytes);
    return;
  }
  //filp = file_open(filename, 0, 0);	
  if (write)  	  
    file_write(filp, offset, buffer, nbytes); 
  else	 
    file_read(filp, offset, buffer, nbytes);
  //file_close(filp);
}

static void looper_request(struct request_queue *q) {
 
  struct request *req;
 
  printk(KERN_INFO "looper: executing request");
   
  req = blk_fetch_request(q);
  while (req != NULL) {
    if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
      printk (KERN_NOTICE "Skip non-CMD request\n");
      __blk_end_request_all(req, -EIO);
      continue;
    }
    looper_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
		 req->buffer, rq_data_dir(req));
    if ( ! __blk_end_request_cur(req, 0) ) {
      req = blk_fetch_request(q);
    }
  }
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int looper_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
  
  long size;
  
  printk(KERN_INFO "looper: executing getgeo");

  /* We have no real geometry, of course, so make something up. */
  size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
  geo->cylinders = (size & ~0x3f) >> 6;
  geo->heads = 4;
  geo->sectors = 16;
  geo->start = 0;
  return 0;
}


static int looper_open(struct block_device *bdev, fmode_t mode) {
  printk(KERN_INFO "looper: executing open");
  filp = file_open(filename, 0, 0);
  return 0;
}

static int looper_release(struct gendisk *gd, fmode_t mode) {
  printk(KERN_INFO "looper: executing close");
  file_close(filp);
  return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations looper_ops = {
  .owner  = THIS_MODULE,
  .open = looper_open,
  .release = looper_release,
  .getgeo = looper_getgeo
};

static int __init looper_init(void) {
  
  if (filename == NULL) {
    printk(KERN_WARNING "looper: no filename defined");
    return -1;
  }

  /*
   * Set up our internal device.
   */
  Device.size = nsectors * logical_block_size;
  spin_lock_init(&Device.lock);
  Device.data = vmalloc(Device.size);
  if (Device.data == NULL)
    return -ENOMEM;
  /*
   * Get a request queue.
   */
  Queue = blk_init_queue(looper_request, &Device.lock);
  if (Queue == NULL)
    goto out;
  blk_queue_logical_block_size(Queue, logical_block_size);
  /*
   * Get registered.
   */
  major_num = register_blkdev(major_num, "looper");
  if (major_num <= 0) {
    printk(KERN_WARNING "looper: unable to get major number\n");
    goto out;
  }
  /*
   * And the gendisk structure.
   */
  Device.gd = alloc_disk(16);
  if (!Device.gd)
    goto out_unregister;
  Device.gd->major = major_num;
  Device.gd->first_minor = 0;
  Device.gd->fops = &looper_ops;
  Device.gd->private_data = &Device;
  strcpy(Device.gd->disk_name, "looper0");
  set_capacity(Device.gd, nsectors);
  Device.gd->queue = Queue;
  add_disk(Device.gd);
  
  return 0;
  
 out_unregister:
  unregister_blkdev(major_num, "looper");
 out:
  vfree(Device.data);
  return -ENOMEM;
}

static void __exit looper_exit(void)
{
  del_gendisk(Device.gd);
  put_disk(Device.gd);
  unregister_blkdev(major_num, "looper");
  blk_cleanup_queue(Queue);
  vfree(Device.data);
}

module_init(looper_init);
module_exit(looper_exit);
