#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/delay.h>

#define MODNAME "virtual_touchscreen"

#define ABS_X_MIN	0
#define ABS_X_MAX	1024
#define ABS_Y_MIN	0
#define ABS_Y_MAX	1024

#define DEVICE_NAME "virtual_touchscreen"
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int Major;            /* Major number assigned to our device driver */
static int Device_Open = 0;  /* Is device open?  Used to prevent multiple access to the device */

struct class * cl;
struct device * dev; 

struct file_operations fops = {
       read: device_read,
       write: device_write,
       open: device_open,
       release: device_release
};			

static void do_softint(struct work_struct *work);

static struct input_dev *virt_ts_dev;
static DECLARE_DELAYED_WORK(work, do_softint);

static void do_softint(struct work_struct *work)
{
	int absx = 0, absy = 0;
	int touched = 0;

	if (touched) {
		input_report_key(virt_ts_dev, BTN_TOUCH, 1);
		input_report_abs(virt_ts_dev, ABS_X, absx);
		input_report_abs(virt_ts_dev, ABS_Y, absy);
	} else {
		input_report_key(virt_ts_dev, BTN_TOUCH, 0);
	}

	input_sync(virt_ts_dev);
}

static int __init virt_ts_init(void)
{
	int err;

	virt_ts_dev = input_allocate_device();
	if (!virt_ts_dev)
		return -ENOMEM;

	virt_ts_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	virt_ts_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(virt_ts_dev, ABS_X,
		ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(virt_ts_dev, ABS_Y,
		ABS_Y_MIN, ABS_Y_MAX, 0, 0);

	virt_ts_dev->name = "Virtual touchscreen";
	virt_ts_dev->phys = "virtual_ts/input0";


	err = input_register_device(virt_ts_dev);
	if (err)
		goto fail1;


    /* Above is evdev part. Below is character device part */

    Major = register_chrdev(0, DEVICE_NAME, &fops);	
    if (Major < 0) {
	printk ("Registering the character device failed with %d\n", Major);
	    goto fail1;
    }
    printk ("virtual_touchscreen: Major=%d\n", Major);

    cl = class_create(THIS_MODULE, DEVICE_NAME);
    if (!IS_ERR(cl)) {
	    dev = device_create(cl, NULL, MKDEV(Major,0), NULL, DEVICE_NAME);
    }


	return 0;

 fail1:	input_free_device(virt_ts_dev);
	return err;
}

static int device_open(struct inode *inode, struct file *file) {
    if (Device_Open) return -EBUSY;
    ++Device_Open;
    return 0;
}
	
static int device_release(struct inode *inode, struct file *file) {
    --Device_Open;
    return 0;
}
	
static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset) {
    const char* message = 
        "Usage: write the following commands to /dev/virtual_touchscreen:\n"
        "    move x y\n"
        "    down\n"
        "    up\n";
    const size_t msgsize = strlen(message);
    loff_t off = *offset;
    if (off >= msgsize) {
        return 0;
    }
    if (length > msgsize - off) {
        length = msgsize - off;
    }
    if (copy_to_user(buffer, message+off, length) != 0) {
        return -EFAULT;
    }

    *offset+=length;
    return length;
}
	
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
    input_report_key(virt_ts_dev, BTN_TOUCH, 1);
    input_report_abs(virt_ts_dev, ABS_X, 0x33);
    input_report_abs(virt_ts_dev, ABS_Y, 0x44);
	input_sync(virt_ts_dev);
    input_report_key(virt_ts_dev, BTN_TOUCH, 0);
	input_sync(virt_ts_dev);
    return len;
}

static void __exit virt_ts_exit(void)
{
	input_unregister_device(virt_ts_dev);

    if (!IS_ERR(cl)) {
	    device_destroy(cl, MKDEV(Major,0));
	    class_destroy(cl);
    }
    unregister_chrdev(Major, DEVICE_NAME);
}

module_init(virt_ts_init);
module_exit(virt_ts_exit);

MODULE_AUTHOR("Vitaly Shukela, vi0oss@gmail.com");
MODULE_DESCRIPTION("Virtual touchscreen driver");
MODULE_LICENSE("GPL");
