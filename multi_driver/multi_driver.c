#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "multi_driver"
#define DEVICE_CLASS_NAME "multi_class"
#define NUM_DEVICES 4
#define BUFFER_SIZE 1024

static dev_t dev_num; // Major and minor combined
static struct class *multi_class;
static struct cdev multi_cdev[NUM_DEVICES]; // Array of structs of each device files

// Data of each device
struct multi_device_data {
    char buffer[BUFFER_SIZE];
    size_t size;
};

static struct multi_device_data devices[NUM_DEVICES];

// File operations
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init multi_init(void)
{
    int ret = 0;
    int i = 0;

    // Allocate a range of device numbers
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to allocate major number\n");
        return ret;
    }
    
    // Create a device class
    multi_class = class_create(DEVICE_CLASS_NAME);
    if (IS_ERR(multi_class))
    {
        unregister_chrdev_region(dev_num, NUM_DEVICES);
        printk(KERN_ERR "Failed to create class\n");
        return PTR_ERR(multi_class);
    }

    // Initialize each device
    for (i = 0; i < NUM_DEVICES; i++)
    {
        cdev_init(&multi_cdev[i], &fops);
        multi_cdev[i].owner = THIS_MODULE;

        ret = cdev_add(&multi_cdev[i], MKDEV(MAJOR(dev_num), MINOR(dev_num) + i), 1);
        if (ret < 0)
        {
            printk(KERN_ERR "Failed to add cdev %d\n", i);
            goto cleanup;
        }

        if (IS_ERR(device_create(multi_class, NULL, MKDEV(MAJOR(dev_num), MINOR(dev_num) + i), NULL, "multi_driver%d", i)))
        {
            printk(KERN_ERR "Failed to create device file for device %d\n", i);
            cdev_del(&multi_cdev[i]);
            goto cleanup;
        }
    }

    printk(KERN_INFO "Allocared major number: %d\n", MAJOR(dev_num));
    printk(KERN_INFO "Multi driver initialized with %d devices\n", NUM_DEVICES);
    return 0;

cleanup:
    while (i--)
    {
        device_destroy(multi_class, MKDEV(MAJOR(dev_num), MINOR(dev_num) + i));
        cdev_del(&multi_cdev[i]);
    }
    class_destroy(multi_class);
    unregister_chrdev_region(dev_num, NUM_DEVICES);
    return ret;
}

static void __exit multi_exit(void)
{
    for (int i = 0; i < NUM_DEVICES; i++)
    {
        device_destroy(multi_class, MKDEV(MAJOR(dev_num), i));
        cdev_del(&multi_cdev[i]);
    }

    class_destroy(multi_class);        
    unregister_chrdev_region(dev_num, NUM_DEVICES);
    printk(KERN_INFO "Multi driver unloaded\n");
}

static int device_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    if (minor >= NUM_DEVICES)
    {
        printk(KERN_WARNING "multi_driver: Invalid minor number %d\n", minor);
        return -ENODEV;
    }

    file->private_data = &devices[minor];
    printk(KERN_INFO "multi_driver: Device opened (minor: %d)\n", minor);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "multi_driver: Device closed (minor: %d)\n", MINOR(inode->i_rdev));
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    struct multi_device_data *dev = file->private_data;
    size_t remaining = dev->size - *offset;

    if (*offset >= dev->size)
        return 0;
        
    if (length > remaining)
        length = remaining;

    if (copy_to_user(buffer, dev->buffer + *offset, length))
        return -EFAULT;

    *offset += length;

    printk(KERN_INFO "multi_driver: Read requested\n");
    return length;
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset)
{
    struct multi_device_data *dev = file->private_data;

    if (length > BUFFER_SIZE)
        length = BUFFER_SIZE;

    if (copy_from_user(dev->buffer, buffer, length))
        return -EFAULT;

    dev->size = length;
    *offset = 0;

    printk(KERN_INFO "multi_driver: Write requested\n");
    return length;
}

module_init(multi_init);
module_exit(multi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dvir");
MODULE_DESCRIPTION("A multi-device driver with separate buffers");
