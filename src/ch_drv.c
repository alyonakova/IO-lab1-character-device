#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
static dev_t first;
static struct cdev c_dev; 
static struct class *cl;
static char* WORK_FILE = "work_file";
static struct file * file;
static bool starts_with(const char *, const char *);
static char* transform_string(const char*);
static size_t bytes_counter;

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	if (file == NULL){
		  printk(KERN_INFO "Driver: cannot read from not opened file()\n");
		return -1;
	}
  char* data = kmalloc(len, GFP_USER);

  //file = filp_open(WORK_FILE, O_RDWR|O_CREAT, 0644);
  set_fs(KERNEL_DS);
  size_t wlen = vfs_read(file, data, len, 0);
  set_fs(USER_DS);

  data = transform_string(data);

  size_t rlen = strlen(data);

  printk(KERN_INFO "Driver: read()\n");
  pr_info("Total amount of data written so far: %ld bytes", bytes_counter);

  if(*off != rlen)
    *off = rlen;
  else
    return 0;

  if(copy_to_user(buf, data, rlen) != 0) {
    return -EFAULT;
  }

  return rlen;
}

static ssize_t my_write(struct file *f, const char __user *buf,  size_t len, loff_t *off)
{

  char * data = kmalloc(len, GFP_USER);
  if(copy_from_user(data, buf, len) != 0) {
    kfree(data);
    return -EFAULT;
  }
  bytes_counter += len;
  pr_info("Total amount of data written so far: %ld bytes", bytes_counter);

  if (starts_with(data, "open ")){
	if (file != NULL)
		filp_close(file, NULL);
		
	int fileNameLen = strlen(data) - 5;
	char subbuff[fileNameLen];
	memcpy( subbuff, &data[5], fileNameLen);
	subbuff[fileNameLen] = '\0';
	WORK_FILE = subbuff;

	file = filp_open(WORK_FILE, O_RDWR|O_CREAT, 0644);
    //my_open(NULL, f);
	return len;
  } else if (starts_with(data, "close")){// && strlen(data) == 5){ 
	if (file != NULL) {
	    filp_close(file, NULL);
		file = NULL;
	}
	else
		  printk(KERN_INFO "Driver: cannot close file which is not opened()\n");
	return len;
  } else if (file == NULL){
		  printk(KERN_INFO "Driver: cannot write to not opened file()\n");
		return -1;
	}

  //WORK_FILE = "write_file";
  size_t wlen = 0;
  
  //file = filp_open(WORK_FILE, O_RDWR|O_CREAT, 0644);

  
  set_fs(KERNEL_DS);

  
  wlen = vfs_write(file, data, len, &file->f_pos);
  //wlen = vfs_write(file, len , sizeof(len)/sizeof(int), &file->f_pos);
  
  set_fs(USER_DS);

  printk(KERN_INFO "Driver: write() len = %ld, %lld\n", len, file->f_pos);
  kfree(data);

  return len;
}

static struct file_operations mychdev_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write
};
 
static bool starts_with(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

static char* transform_string(const char* string)
{
  int len = strlen(string);
  char* result = kmalloc(len*3, GFP_USER);
  int pos = 0, i;
  for (i = 0; i < len; i++){
    int charNum = string[i];
    if (charNum >= 65 && charNum <= 90 || charNum >= 97 && charNum <= 122){
	    charNum -= (charNum > 90) ? 96 : 64;
	    if (charNum > 9){
	    	result[pos++] = (charNum / 10) + 48;
			result[pos++] = (charNum % 10) + 48;
		} 
    	else {
			result[pos++] = charNum + 48;
    	}
		result[pos++] = ' ';
    } 
  }
  return result;
}

static int __init ch_drv_init(void)
{
    printk(KERN_INFO "Hello!\n");
    if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0) 
	{
		return -1;
	}
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		unregister_chrdev_region(first, 1);
		return -1;
	}
    if (device_create(cl, NULL, first, NULL, "mychdev") == NULL) 
	{
      class_destroy(cl);
      unregister_chrdev_region(first, 1);
      return -1;
	}
    cdev_init(&c_dev, &mychdev_fops);
    
    if (cdev_add(&c_dev, first, 1) == -1)
	{
      device_destroy(cl, first);
      class_destroy(cl);
      unregister_chrdev_region(first, 1);
      return -1;
	}
    return 0;
}
 
static void __exit ch_drv_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "Bye!!!\n");
}
 
module_init(ch_drv_init);
module_exit(ch_drv_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("The first kernel module");

