#ifndef Device_H
#define Device_H

#include "Header.h"
#include "Caesar.h"
#include "String.h"
/* BEGIN function prototypes */
int transDeviceOpen(struct inode *deviceFile, 
                    struct file *instance);
int transDeviceClose(struct inode *deviceFile,
                     struct file *instance);
ssize_t transDeviceRead(struct file *instance,
                        char *user, size_t count,
                        loff_t *offset);
ssize_t transDeviceWrite(struct file *filp,
                         char const __user *buf,
                         size_t count,
                         loff_t *offs);
/* END function prototypes */

typedef struct { // struct that represents a device
    String string;
    ssize_t maxBufSize;
    ssize_t readers;
    ssize_t writers;
    int minorNumber;
    struct semaphore sem;
    wait_queue_head_t q;
} TransDevice;

#endif // Device_H
