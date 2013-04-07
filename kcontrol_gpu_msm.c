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
#include <mach/kgsl.h>
#include <linux/platform_device.h>

#define THIS_EXPERIMENTAL 1

#define DRIVER_AUTHOR "Dennis Rassmann <showp1984@gmail.com>"
#define DRIVER_DESCRIPTION "KControl GPU module for msm devices"
#define DRIVER_VERSION "1.0"
#define LOGTAG "kcontrol_gpu_msm: "

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

struct global_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *a, struct attribute *b,
			 const char *c, size_t count);
};

#define define_one_global_ro(_name)		\
static struct global_attr _name =		\
__ATTR(_name, 0444, show_##_name, NULL)

/* module parameters */
static uint kgsl_pdata = 0x00000000;
module_param(kgsl_pdata, uint, 0444);

static uint kgsl_3d_dev = 0x00000000;
module_param(kgsl_3d_dev, uint, 0444);

struct kgsl_device_platform_data *kpdata;
struct platform_device *kpdev;

struct kobject *kcontrol_gpu_msm_kobject;
static ssize_t show_kgsl_pwrlevels(struct kobject *a, struct attribute *b,
				   char *buf)
{
	int i = 0, len = 0;
	for (i=0; i<ARRAY_SIZE(kpdata->pwrlevel); i++) {
		len += sprintf(buf + len, "%u ", kpdata->pwrlevel[i].gpu_freq);
	}
	return len;
}

define_one_global_ro(kgsl_pwrlevels);

static struct attribute *kcontrol_gpu_msm_attributes[] = {
	&kgsl_pwrlevels.attr,
	NULL
};

static struct attribute_group kcontrol_gpu_msm_attr_group = {
	.attrs = kcontrol_gpu_msm_attributes,
	.name = "gpu_msm",
};

static int __init kcontrol_gpu_msm_init(void)
{
	int rc = 0;
	kpdata = (struct kgsl_device_platform_data *)kgsl_pdata;
	kpdev = (struct platform_device *)kgsl_3d_dev;

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

    WARN_ON(kpdata = NULL);
    WARN_ON(kpdev = NULL);

	kcontrol_gpu_msm_kobject = kobject_create_and_add("kcontrol", kernel_kobj);
	if (kcontrol_gpu_msm_kobject) {
		rc = sysfs_create_group(kcontrol_gpu_msm_kobject,
							&kcontrol_gpu_msm_attr_group);
		if (rc) {
			pr_warn(LOGTAG"sysfs: ERROR, could not create sysfs group");
		}
	} else
		pr_warn(LOGTAG"sysfs: ERROR, could not create sysfs kobj");


    printk(KERN_INFO LOGTAG "everything done, have fun!\n");
	return 0;
}

static void __exit kcontrol_gpu_msm_exit(void)
{
	printk(KERN_INFO LOGTAG "unloaded\n");
}

module_init(kcontrol_gpu_msm_init);
module_exit(kcontrol_gpu_msm_exit);

