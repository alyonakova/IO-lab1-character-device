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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

static dev_t first;
static struct cdev c_dev;
static struct class *cl;
static size_t bytes_counter;

int scull_major = 0;
int scull_minor = 0;
int scull_nr_devs = 1;
int scull_quantum = 4000;
int scull_qset = 1000;

struct scull_qset {
    void **data;
    struct scull_qset *next;
};

struct scull_dev {
    struct scull_qset *data;
    int quantum;
    int qset;
    unsigned long size;
    unsigned int access_key;
    struct semaphore sem;
    struct cdev cdev;
};

struct scull_dev *scull_device;


int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    int i;

    for (dptr = dev->data; dptr; dptr = next) {
        if (dptr->data) {
            for (i = 0; i < qset; i++)
                kfree(dptr->data[i]);

            kfree(dptr->data);
            dptr->data = NULL;
        }

        next = dptr->next;
        kfree(dptr);
    }

    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;

    return 0;
}

static int my_open(struct inode *i, struct file *f)
{
    struct scull_dev *dev;

    pr_info("Driver: open()\n");

    dev = container_of(i->i_cdev, struct scull_dev, cdev);
    f->private_data = dev;

    if ((f->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;

        scull_trim(dev);
        up(&dev->sem);
    }

    pr_info("Driver: opened \n");

    return 0;
}

static int my_close(struct inode *i, struct file *f)
{
    pr_info("Driver: close()\n");

    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{

    char data[] = "Data from kernel module\n";
    size_t rlen = strlen(data);

    pr_info("Driver: read()\n");
    pr_info("Total amount of data written so far: %ld bytes", bytes_counter);

    if (*off != rlen)
        *off = rlen;
    else
        return 0;

    if (copy_to_user(buf, data, rlen) != 0)
    {
        return -EFAULT;
    }

    return rlen;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{

    char fname[] = "wfile";
    size_t wlen = 0;
    struct file *test_file = filp_open(fname, O_RDWR | O_CREAT, 0644);
    char *data = kmalloc(len, GFP_USER);

    if (copy_from_user(data, buf, len) != 0)
    {
        kfree(data);
        return -EFAULT;
    }

    bytes_counter += len;

    set_fs(KERNEL_DS);

    wlen = vfs_write(test_file, data, len, &test_file->f_pos);

    set_fs(USER_DS);

    pr_info("Driver: write() len = %ld, %lld\n", len, test_file->f_pos);
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

static int __init ch_drv_init(void)
{
    pr_info("Hello!\n");
    if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
    {
        return -1;
    }
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
    {
        unregister_chrdev_region(first, 1);
        return -1;
    }
    if (device_create(cl, NULL, first, NULL, "var1") == NULL)
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

    bytes_counter = 0;

    return 0;
}

static void __exit ch_drv_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    pr_info("Total amount of data written: %ld bytes", bytes_counter);
    pr_info("Bye!!!\n");
}

module_init(ch_drv_init);
module_exit(ch_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandra Zhurbova");
MODULE_AUTHOR("Alyona Kovalyova");
MODULE_AUTHOR("Egor Dubenetskiy");
MODULE_AUTHOR("Vadim Kolishchuk");
MODULE_DESCRIPTION("A simple character device driver");
