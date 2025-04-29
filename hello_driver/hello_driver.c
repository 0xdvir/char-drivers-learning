#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "hello_driver"
#define BUF_LEN 5

static int major;
static char msg[BUF_LEN];
static short msg_size;
static int device_open_count = 0;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

static int __init hello_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
    {
        printk(KERN_ALERT "Registering char device failed with %d\n", major);
        return major;
    }
    printk(KERN_INFO "I was assigned major number %d.\n", major);
    return 0; 
}

static void __exit hello_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    printk(KERN_INFO "Goodbye!\n");
}

static int device_open(struct inode *inode, struct file *file)
{
    if (device_open_count > 0)
    {
        printk(KERN_ALERT "Device busy\n");
        return -EBUSY;
    }

    device_open_count++;
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    device_open_count--;
    printk(KERN_INFO "Device released\n");
    return 0;
}

static ssize_t device_read(struct file *flip, char *buffer, size_t length, loff_t *offset)
{
    int bytes_read = 0;

    if (*msg == 0)
        return 0;

    // Check if we've finished reading the message
    if (*offset >= msg_size)
        return 0;

    if (length > msg_size - *offset)
        length = msg_size - *offset;

    // Copy data to user space
    if (copy_to_user(buffer, msg + *offset, length)) {
        printk(KERN_ERR "Failed to send data to user\n");
        return -EFAULT;
    }

    *offset += length;
    bytes_read = length;

    printk(KERN_INFO "Sent %d characters to the user\n", bytes_read);
    return bytes_read;
}

static ssize_t device_write(struct file *flip, const char *buff, size_t len, loff_t *off)
{
    if (len > BUF_LEN)
    {
        printk(KERN_ERR "Message too long\n");
        return -EINVAL;
    }

    if (copy_from_user(msg, buff, len))
    {
        printk(KERN_ERR "Failed to copy data from user\n");
        return -EFAULT;
    }

    msg_size = len;
    msg[msg_size] = '\0';  // null terminate for safety
    printk(KERN_INFO "Received %zu characters from the user\n", len);
    return len;
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dvir");
MODULE_DESCRIPTION("A simple Linux chat driver");
