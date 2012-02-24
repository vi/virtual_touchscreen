#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/delay.h>

#define MODNAME "virtual_touchscreen"

#define ABS_X_MIN	40
#define ABS_X_MAX	950
#define ABS_Y_MIN	80
#define ABS_Y_MAX	910

#define	PHDR	0xa400012e
#define SCPDR	0xa4000136

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

	return 0;

 fail1:	input_free_device(virt_ts_dev);
	return err;
}

static void __exit virt_ts_exit(void)
{
	input_unregister_device(virt_ts_dev);
}

module_init(virt_ts_init);
module_exit(virt_ts_exit);

MODULE_AUTHOR("Vitaly Shukela, vi0oss@gmail.com");
MODULE_DESCRIPTION("Virtual touchscreen driver");
MODULE_LICENSE("GPL");
