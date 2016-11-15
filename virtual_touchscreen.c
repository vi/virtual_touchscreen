#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/uaccess.h>

#define MODNAME "virtual_touchscreen"

#define ABS_X_MIN	0
#define ABS_X_MAX	1024
#define ABS_Y_MIN	0
#define ABS_Y_MAX	1024

#define MAX_CONTACTS 10    // 10 fingers is it

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

static struct input_dev *virt_ts_dev;

static int __init virt_ts_init(void)
{
	int err;

	virt_ts_dev = input_allocate_device();
	if (!virt_ts_dev)
		return -ENOMEM;

	virt_ts_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	virt_ts_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(virt_ts_dev, ABS_X, ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(virt_ts_dev, ABS_Y, ABS_Y_MIN, ABS_Y_MAX, 0, 0);

	virt_ts_dev->name = "Virtual touchscreen";
	virt_ts_dev->phys = "virtual_ts/input0";

    input_mt_init_slots(virt_ts_dev, MAX_CONTACTS, INPUT_MT_DIRECT);

	input_set_abs_params(virt_ts_dev, ABS_MT_POSITION_X, ABS_X_MIN, ABS_X_MAX, 0, 0);
	input_set_abs_params(virt_ts_dev, ABS_MT_POSITION_Y, ABS_Y_MIN, ABS_Y_MAX, 0, 0);

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
        "    x num  - move to (x, ...)\n"
        "    y num  - move to (..., y)\n"
        "    d 0    - touch down\n"
        "    u 0    - touch up\n"
        "    s slot - select multitouch slot (0 to 9)\n"
        "    a flag - report if the selected slot is being touched\n"
        "    e 0   - trigger input_mt_report_pointer_emulation\n"
        "    X num - report x for the given slot\n"
        "    Y num - report y for the given slot\n"
        "    S 0   - sync (should be after every block of commands)\n"
        "    M 0   - multitouch sync\n"
        "    T num - tracking ID\n"
        "    also 0123456789:; - arbitrary ABS_MT_ command (see linux/input.h)\n"
        "  each command is char and int: sscanf(\"%c%d\",...)\n"
        "  <s>x and y are from 0 to 1023</s> Probe yourself range of x and y\n"
        "  Each command is terminated with '\\n'. Short writes == dropped commands.\n"
        "  Read linux Documentation/input/multi-touch-protocol.txt to read about events\n";
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
	
static void execute_command(char command, int arg1) {
    switch(command) {
        case 'x':
            input_report_abs(virt_ts_dev, ABS_X, arg1);
            break;
        case 'y':
            input_report_abs(virt_ts_dev, ABS_Y, arg1);
            break;
        case 'd':
            input_report_key(virt_ts_dev, BTN_TOUCH, 1);
            break;
        case 'u':
            input_report_key(virt_ts_dev, BTN_TOUCH, 0);
            break;

        case 's':
            input_mt_slot(virt_ts_dev, arg1);
            break;
        case 'a':
            input_mt_report_slot_state(virt_ts_dev, MT_TOOL_FINGER, arg1);
            break;
        case 'e':
            input_mt_report_pointer_emulation(virt_ts_dev, true);
            break;
        case 'X':
            input_event(virt_ts_dev, EV_ABS, ABS_MT_POSITION_X, arg1);
            break;
        case 'Y':
            input_event(virt_ts_dev, EV_ABS, ABS_MT_POSITION_Y, arg1);
            break;

        case 'S':
	        input_sync(virt_ts_dev);
            break;
        case 'M':
            input_mt_sync(virt_ts_dev);
            break;
        case 'T':
            input_event(virt_ts_dev, EV_ABS, ABS_MT_TRACKING_ID, arg1);
            break;
        default:
            if ((command>=0x30) && (command<=0x3b)) {
                input_event(virt_ts_dev, EV_ABS, command, arg1);
            } else {
                printk("<4>virtual_touchscreen: Unknown command %c with arg %d\n", command, arg1);
            }
    }
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
    char command;
    int arg1;

    int i;
    int p=0;
    for(i=0; i<len; ++i) {
        if (buff[i]=='\n') {
            sscanf(buff+p, "%c%d", &command, &arg1);
            p=i+1;
            execute_command(command, arg1);
        }
    }

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
