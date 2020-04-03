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

static char* WORK_FILE = "work_file";
static struct file * file;

static int my_open(struct inode *i, struct file *f)
{
    pr_info("Driver: open()\n");

    //file = filp_open(WORK_FILE, O_RDWR|O_CREAT, 0644);

    return 0;
}

static int my_close(struct inode *i, struct file *f)
{
    pr_info("Driver: close()\n");

    return 0;
}

static bool starts_with(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0) {

        return 1;
    }

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

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    char* data = kmalloc(len, GFP_USER);

    set_fs(KERNEL_DS);
    set_fs(USER_DS);

    data = transform_string(data);

    size_t rlen = strlen(data);

    pr_info("Driver: read()\n");

    if(*off != rlen) {
        *off = rlen;
    } else {

        return 0;
    }

    if(copy_to_user(buf, data, rlen) != 0) {
        return -EFAULT;
    }

    return rlen;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    char * data = kmalloc(len + 1, GFP_USER);
    if(copy_from_user(data, buf, len) != 0) {
        kfree(data);

        return -EFAULT;
    }
    data[len] = '\0';

    if (starts_with(data, "open ")){
        if (file != NULL) {
            filp_close(file, NULL);
        }

        int fileNameLen = strlen(data) - 5;
        char subbuff[fileNameLen + 1];

        memcpy(&subbuff[0], &data[5], fileNameLen);
        subbuff[fileNameLen] = '\0';
        WORK_FILE = subbuff;

        pr_info("Имя файла - %s\n", WORK_FILE);
        file = filp_open(WORK_FILE, O_RDWR|O_CREAT, 0644);

        return len;
    } else if (starts_with(data, "close")){
        if (file != NULL) {
            filp_close(file, NULL);
            file = NULL;
        } else {
            pr_info("Driver: can`t close (nothing was opened)\n");
        }

        return len;
    } else if (file == NULL){
        pr_info("Driver: can`t write (nothing was opened)\n");

        return -1;
    }

    set_fs(KERNEL_DS);


    vfs_write(file, data, len, &file->f_pos);

//    vfs_write(file, len, sizeof(len)/si, &file->f_pos);

    set_fs(USER_DS);

    pr_info("Driver: write() len = %ld\n", len);
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
