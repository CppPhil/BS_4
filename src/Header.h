#ifndef Header_H
#define Header_H

/* BEGIN includes */
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h> /* copy_to_user, copy_from_user */
#include <linux/slab.h> /* dynamic memory allocation */
#include <linux/cdev.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/string.h>
/* END includes */
/* BEGIN macros */
#define DEBUG /* comment/uncomment this to enable/disable debug mode */
#define DRIVER_NAME "translate"
#define MAJOR_NUMBER    0   /* 0 triggers dynamic major number selection */
#define BUFFERSIZE  40
#define BOOL    int
#define TRUE    1
#define FALSE   0
#define TRANS_OFFSET    3 /* by how much to 'shift' a character on en/decoding */
#define NUM_DEVICES 2 /* 2 devices, trans0 and trans1 */
#define CHARS_IN_ALPHABET   26
#define EXIT_OK 0
#define EXIT_FAIL   -1
#ifdef DEBUG
#   define PRINT_DEBUG(formatStr, args...) printk(KERN_DEBUG DRIVER_NAME ": " formatStr, ## args)
#else
#   define PRINT_DEBUG(formatStr) (void)formatStr
#endif
#define COUNTOF(arr)    (sizeof(arr) / sizeof(*arr)) /* elements in array, the array must not be a pointer, beware of array to pointer decay */
#define HEAP_ALLOC8(bytes) kzalloc(bytes, GFP_KERNEL) /* allocate bytes bytes on the heap and initialize them to zero */
/* END macros */

#endif // Header_H
