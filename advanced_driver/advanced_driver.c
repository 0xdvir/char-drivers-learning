#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#define SUCCESS 0

#define MY_IOCTL_CMD_CREATE_DEVICE _IO('a', 1)

#define DEVICE_NAME "advanced_driver"
#define DEVICE_CLASS_NAME "advanced_class"
#define MAX_NUM_DEVICES 5
#define BUFFER_SIZE 1024

static int major;
static struct cdev *advanced_cdevs[MAX_NUM_DEVICES];
static struct class *advanced_class;
static int device_count = 0; // Track the number of devices created

// Data of each device
struct multi_device_data {
    char buffer[BUFFER_SIZE];
    size_t size;
};

static struct multi_device_data devices[MAX_NUM_DEVICES];

// File operations
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
long my_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = my_ioctl,
};

long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int result = SUCCESS;
    switch (cmd)
    {
        case MY_IOCTL_CMD_CREATE_DEVICE:
            if (device_count >= MAX_NUM_DEVICES)
            {
                pr_err("Maximum number of devices reached\n");
                return -ENOMEM;
            }
            
            // Allocate the new device dynamically
            dev_t dev_num = MKDEV(major, device_count);

            // Register the new device
            struct cdev *new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
            if (!new_cdev)
            {
                pr_err("Failed to allocate memory for new cdev\n");
                return -ENOMEM;
            }

            cdev_init(new_cdev, &fops);
            new_cdev->owner = THIS_MODULE;

            // Add the new cdev to the system
            result = cdev_add(new_cdev, dev_num, 1);
            if (result < 0)
            {
                kfree(new_cdev);
                unregister_chrdev_region(dev_num, 1);
                pr_err("Failed to add cdev\n");
                return result;
            }

            advanced_cdevs[device_count] = new_cdev;

            // Create a device file
            if (IS_ERR(device_create(advanced_class, NULL, dev_num, NULL, "%s%d", DEVICE_NAME, device_count)))
            {
                cdev_del(new_cdev);
                unregister_chrdev_region(dev_num, 1);
                kfree(new_cdev);
                return -EFAULT;
            }

            device_count++;
            pr_info("Device created, current count: %d\n", device_count);
            pr_info("New device /dev/%s%d\n", DEVICE_NAME, device_count - 1);

            break;
        default:
            return -EINVAL;
    }
    return result;
}

static int __init advanced_init(void)
{
    int result = SUCCESS;
    dev_t dev_num;

    // Allocate a major number
    result = alloc_chrdev_region(&dev_num, 0, MAX_NUM_DEVICES, DEVICE_NAME);
    if (result < 0)
    {
        pr_err("Failed to allocate major number\n");
        return result;
    }

    major = MAJOR(dev_num);

    // Create a class
    advanced_class = class_create(DEVICE_CLASS_NAME);
    if (IS_ERR(advanced_class))
    {
        unregister_chrdev_region(MKDEV(major, 0), MAX_NUM_DEVICES);
        pr_err("Failed to create class\n");
        return PTR_ERR(advanced_class);
    }

    dev_num = MKDEV(major, 0);
    advanced_cdevs[0] = kzalloc(sizeof(struct cdev), GFP_KERNEL);
    if (!advanced_cdevs[0])
    {
        class_destroy(advanced_class);
        unregister_chrdev_region(dev_num, MAX_NUM_DEVICES);
        pr_err("Failed to allocate memory for cdev\n");
        return -ENOMEM;
    }

    cdev_init(advanced_cdevs[0], &fops);
    advanced_cdevs[0]->owner = THIS_MODULE;

    result = cdev_add(advanced_cdevs[0], dev_num, 1);
    if (result < 0)
    {
        kfree(advanced_cdevs[0]);
        class_destroy(advanced_class);
        unregister_chrdev_region(dev_num, MAX_NUM_DEVICES);
        pr_err("Failed to add cdev\n");
        return result;
    }

    if (IS_ERR(device_create(advanced_class, NULL, dev_num, NULL, "%s%d", DEVICE_NAME, device_count)))
    {
        cdev_del(advanced_cdevs[0]);
        kfree(advanced_cdevs[0]);
        class_destroy(advanced_class);
        unregister_chrdev_region(dev_num, MAX_NUM_DEVICES);
        pr_err("Failed to create device file\n");
        return -EFAULT;
    }

    device_count = 1; // Initialize device count to 1
    pr_info("Driver initialized. Device /dev/%s0 created\n", DEVICE_NAME);
    return result;
}

static void __exit advanced_exit(void)
{
    // Cleanup
    int i = 0;
    for (i = 0; i < device_count; i++)
    {
        dev_t dev_num = MKDEV(major, i);
        device_destroy(advanced_class, dev_num); // Remove device file
        cdev_del(advanced_cdevs[i]); // Unregister cdeev
        kfree(advanced_cdevs[i]);
    }

    class_destroy(advanced_class); // Destroy class
    unregister_chrdev_region(major, MAX_NUM_DEVICES); // Unregister major number
    pr_info("Driver unloaded\n");
}

static int device_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    if (minor >= device_count)
    {
        pr_err("Invalid minor number %d\n", minor);
        return -ENODEV;
    }

    file->private_data = &devices[minor];

    if (!file->private_data)
    {
        pr_err("Failed to assign private_data for minor %d\n", minor);
        return -ENODEV;
    }

    pr_info("Device opened\n");
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("Device released\n");
    return SUCCESS;
}

static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    struct multi_device_data *dev = file->private_data;
    size_t remaining = dev->size - *offset;

    if (*offset >= dev->size)
        return SUCCESS;
        
    if (length > remaining)
        length = remaining;

    if (copy_to_user(buffer, dev->buffer + *offset, length))
        return -EFAULT;

    *offset += length;

    pr_info("multi_driver: Read requested\n");
    return length;
}

static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
    struct multi_device_data *dev = file->private_data;

    if (length > BUFFER_SIZE)
        length = BUFFER_SIZE;

    if (copy_from_user(dev->buffer, buffer, length))
        return -EFAULT;

    dev->size = length;
    *offset = 0;

    pr_info("multi_driver: Write requested\n");
    return length;
}

module_init(advanced_init);
module_exit(advanced_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("0xdvir");
MODULE_DESCRIPTION("Advanced Linux Driver");
