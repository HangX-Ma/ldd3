#ifndef __CHAR_DEVICE__H__
#define __CHAR_DEVICE__H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/device/class.h>


#define DUMMY_CHAR_NBANK 4
#define DUMMY_CHAR_CLASS "dummy-char-class"
#define DUMMY_CHAR_DEVICE_NAME "dummy-char"

#endif  //!__CHAR_DEVICE__H__