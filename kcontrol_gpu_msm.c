/*
 * KControl GPU module for msm devices
 *
 * Copyright (c) 2013 Dennis Rassmann
 * Author: Dennis Rassmann <showp1984@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kallsyms.h>
#include <linux/sysfs.h>

#define THIS_EXPERIMENTAL 1

#define DRIVER_AUTHOR "Dennis Rassmann <showp1984@gmail.com>"
#define DRIVER_DESCRIPTION "KControl GPU module for msm devices"
#define DRIVER_VERSION "1.0"
#define LOGTAG "kcontrol_gpu_msm: "

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

/* module parameters */ //no need atm
//static uint enabled = 1;
//module_param(enabled, uint, 0444);

static int __init kcontrol_gpu_msm_init(void)
{


#if THIS_EXPERIMENTAL
        printk(KERN_WARNING LOGTAG "#######################################");
        printk(KERN_WARNING LOGTAG "WARNING: THIS MODULE IS EXPERIMENTAL!\n");
        printk(KERN_WARNING LOGTAG "You have been warned.\n");
	printk(KERN_INFO LOGTAG "%s, version %s\n", DRIVER_DESCRIPTION,
		DRIVER_VERSION);
	printk(KERN_INFO LOGTAG "author: %s\n", DRIVER_AUTHOR);
        printk(KERN_WARNING LOGTAG "#######################################");
#else
        printk(KERN_INFO LOGTAG "%s, version %s loaded\n", DRIVER_DESCRIPTION,
               DRIVER_VERSION);
        printk(KERN_INFO LOGTAG "by: %s\n", DRIVER_AUTHOR);
#endif
        BUG_ON(old_kobj == NULL);



        printk(KERN_INFO LOGTAG "all done, have fun!\n");
	return 0;
}

static void __exit kcontrol_gpu_msm_exit(void)
{
	printk(KERN_INFO LOGTAG "unloaded\n");
}

module_init(kcontrol_gpu_msm_init);
module_exit(kcontrol_gpu_msm_exit);

