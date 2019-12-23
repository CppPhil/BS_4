#include "Device.h"

extern TransDevice *devices; // from module.c
extern int *pTransOffset; // from module.c

int transDeviceOpen(struct inode *deviceFile,
                       struct file *instance) { // called when a process opens the device
    PRINT_DEBUG("transDeviceOpen called\n");
    
    int minorNumber = MINOR(deviceFile->i_rdev); // extract the minor device number
    TransDevice *device = &devices[minorNumber];
    device->minorNumber = minorNumber; // let the device know which minor device number it has
    instance->private_data = device; // save a pointer to the TransDevice struct in the struct file *
    
    if (instance->f_mode & FMODE_WRITE) { // opened in write mode
        PRINT_DEBUG("%s: write mode device: %d\n", __FUNCTION__, minorNumber);
        if (device->writers != 0) {
            PRINT_DEBUG("%s: writers != 0, got -EBUSY device: %d\n", __FUNCTION__, minorNumber);
            return -EBUSY; // you can't write if another process is already writing.
        } // end if
        ++device->writers;
        PRINT_DEBUG("%s: incremented writers device: %d\n", __FUNCTION__, minorNumber);
    } // end if
    
    if (instance->f_mode & FMODE_READ) { // opened in read mode
        PRINT_DEBUG("%s: read mode device: %d\n", __FUNCTION__, minorNumber);
        if (device->readers != 0) {
            PRINT_DEBUG("%s: readers != 0, got -EBUSY device: %d\n", __FUNCTION__, minorNumber);
            return -EBUSY; // you can't read if another process is already reading.
        } // end if
        ++device->readers;
        PRINT_DEBUG("%s: incremented readers device: %d\n", __FUNCTION__, minorNumber);
    } // end if
    
    nonseekable_open(deviceFile, instance); // no seeking!
    PRINT_DEBUG("device: %d exited %s successfully.\n", minorNumber, __FUNCTION__);
    return EXIT_OK;
} // end transDeviceOpen

int transDeviceClose(struct inode *deviceFile,
                       struct file *instance) { // called when a process closes its connection to the device.
    PRINT_DEBUG("transDeviceClose called\n");
    TransDevice *device = instance->private_data; // get a pointer to the TransDevice we stored.
    
    if (instance->f_mode & FMODE_WRITE) {
        PRINT_DEBUG("device %d in %s: decrementing writers\n", device->minorNumber, __FUNCTION__);
        --device->writers;
    } // end if
    
    if (instance->f_mode & FMODE_READ) {
        PRINT_DEBUG("device %d in %s: decrementing readers\n", device->minorNumber, __FUNCTION__);
        --device->readers;
    } // end if   
    PRINT_DEBUG("device %d: exited %s successfully. String capacity: %u\n", device->minorNumber, __FUNCTION__, device->string.capacity(&device->string));
    return EXIT_OK;
} // end transDeviceClose

ssize_t transDeviceWrite(struct file *filp,
                         char const __user *buf,
                         size_t count, loff_t *offs) { // called when a process writes to the device.
    PRINT_DEBUG("%s called.\n", __FUNCTION__);
    if (count == 0U) {
        return count;
    }
    char *fromUser = HEAP_ALLOC8(sizeof(char) * (count + 1)); // we will store the raw input from the user here. +1 for the '\0', we would like this to be a properly formed string.
    if (fromUser == NULL) { // heap allocation failed.
        PRINT_DEBUG("ERROR: no memory for fromUser in transDeviceWrite\n");
        return -ENOMEM; // not enough memory :(
    } // end if
    
    TransDevice *device = filp->private_data; // get a pointer to the TransDevice
    PRINT_DEBUG("device %d in %s trying to acquire semaphore, line: %d\n", device->minorNumber, __FUNCTION__, __LINE__);
    int retVal = down_interruptible(&device->sem); /*
    * down_interruptible - acquire the semaphore unless interrupted
    *
    * Attempts to acquire the semaphore.  If no more tasks are allowed to
    * acquire the semaphore, calling this function will put the task to sleep.
    * If the sleep is interrupted by a signal, this function will return -EINTR.
    * If the semaphore is successfully acquired, this function returns 0.
    */
    PRINT_DEBUG("device %d in %s got the semaphore, line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
    if (retVal != 0) { /* if process woke up from signal */
        PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        return -ERESTARTSYS; // try again if you can
    } // end if
    while (device->string.size(&device->string) >= device->maxBufSize) { // if full -> we have to wait, because there is no more room in the buffer. Someone has to read something first.
        PRINT_DEBUG("device %d in %s: my buffer is full!\n", device->minorNumber, __FUNCTION__);
        up(&device->sem); // release semaphore
        PRINT_DEBUG("device %d in %s in line %d: releasing semaphore, waiting until my buffer is no longer full\n", device->minorNumber, __FUNCTION__, __LINE__);
        retVal = wait_event_interruptible(device->q, device->string.size(&device->string) < device->maxBufSize); // go into the wait queue and wait until the condition is true.
        if (retVal != 0) { /* if process woke up from signal */
            PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
            return -ERESTARTSYS;
        } // end if
        PRINT_DEBUG("device %d in %s: trying to acquire semaphore, line: %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        retVal = down_interruptible(&device->sem); // acquire the semaphore
        PRINT_DEBUG("device %d in %s: got semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        if (retVal != 0) { /* if process woke up from signal */
            PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
            return -ERESTARTSYS;
        } // end if
    } // end while buffer full

    size_t howMuchToAppend = min(count - 1U, (size_t)(device->maxBufSize - device->string.size(&device->string))); /* We either copy as much as the user
        wants (in characters), or we copy as much as we can still hold. device->maxBufSize is the maximum size, subtracting the current size is the remaining capacity
        */
    
    BOOL returnCount = FALSE;
    if (howMuchToAppend == (count - 1U)) {
        returnCount = TRUE;
    }
   
    retVal = copy_from_user(fromUser, // copy in here
                            buf, /* from parameter list */
                            count // that many bytes
                           ); // Returns number of bytes that could not be copied. On success, this will be zero.
    
    fromUser[howMuchToAppend] = '\0'; // shorten the string so we copy only as much as we can.                
    if (device->minorNumber == 0) { // encode        
        encodeString(fromUser, *pTransOffset); // encode the string
        PRINT_DEBUG("device %d in %s encoded the string to: %s\n", device->minorNumber, __FUNCTION__, fromUser);
    } /* end if device 0 (encoder) */ else {
        /* Device 1 (Decoder) */
        decodeString(fromUser, *pTransOffset); // decode the string, that we got from user
        PRINT_DEBUG("device %d in %s decoded the string to: %s\n", device->minorNumber, __FUNCTION__, fromUser);
    } // end if device 1
    
    device->string.append(&device->string, fromUser); // append to the device's buffer
    PRINT_DEBUG("device %d in %s appended string to my buffer here is my buffer %s\n", device->minorNumber, __FUNCTION__, device->string.data(&device->string));    
    up(&device->sem); // release semaphore
    PRINT_DEBUG("device %d in %s released semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
    wake_up(&device->q); // wake up those that wait in the queue 
    kfree(fromUser);
    PRINT_DEBUG("device %d exited %s with count %u howMuchToAppend: %u\n", device->minorNumber, __FUNCTION__, count, howMuchToAppend);
    size_t bytesWritten = 0U;
    if (returnCount) {
        bytesWritten = count;
    } else {
        bytesWritten = howMuchToAppend;
    }
    return bytesWritten; // return how many bytes were actually written
} /* end transDeviceWrite */

ssize_t transDeviceRead(struct file *instance,
                          char *user, size_t count,
                          loff_t *offset) { // called when a process reads from the device.
    PRINT_DEBUG("%s called\n", __FUNCTION__);
    int errorCode;
    TransDevice *device = NULL;
    device = instance->private_data; // get the pointer to the TransDevice.
    char *toUser = HEAP_ALLOC8(sizeof(char) * (count + 1U)); // this is where we will put the stuff we want to copy to user space. +1 because if we need to copy the entire buffer, which is count large at the most, we still want to form a valid C-String and need one more byte for '\0'.
    if (toUser == NULL) {
        PRINT_DEBUG("ERROR: no memory for toUser in transDeviceRead\n");
        return -ENOMEM;
    } // end if
    PRINT_DEBUG("device %d in %s trying to acquire semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
    errorCode = down_interruptible(&device->sem); // acquire the semaphore
    PRINT_DEBUG("device %d in %s got semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
    if (errorCode != 0) {
        PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        return -ERESTARTSYS;
    } // end if
    
    while (device->string.isEmpty(&device->string)) { // if this device's buffer is empty the process cannot read from it it must wait until there is something to read.
        PRINT_DEBUG("device %d in %s line %d: my buffer is empty\n", device->minorNumber, __FUNCTION__, __LINE__);
        up(&device->sem); /* release the semaphore */
        PRINT_DEBUG("device %d in %s line %d: released semaphore\n", device->minorNumber, __FUNCTION__, __LINE__);
        errorCode = wait_event_interruptible(device->q, !device->string.isEmpty(&device->string)); // go into the waitqueue
        PRINT_DEBUG("device %d in %s my buffer is no longer empty (or i got a signal).\n", device->minorNumber, __FUNCTION__);
        if (errorCode != 0) {
            PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
            return -ERESTARTSYS;
        } // end if
        PRINT_DEBUG("device %d in %s trying to acquire semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        errorCode = down_interruptible(&device->sem); // wait for semaphore (try to acquire it).
        PRINT_DEBUG("device %d in %s got semaphore in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
        if (errorCode != 0) {
            PRINT_DEBUG("device %d in %s woke up from signal in line %d\n", device->minorNumber, __FUNCTION__, __LINE__);
            return -ERESTARTSYS;
        } // end if        
    } // end while buffer empty
    
    PRINT_DEBUG("device %d in %s line %d count is %u\n", device->minorNumber, __FUNCTION__, __LINE__, count);
    count = min(count, device->string.size(&device->string) + 1U); // read as much as the user wants, or if we don't have that much read as much as we've got.
    PRINT_DEBUG("device %d in %s line %d count is %u\n", device->minorNumber, __FUNCTION__, __LINE__, count);   
    device->string.toBuffer(&device->string, toUser, count + 1U);
    PRINT_DEBUG("device %d in %s line %d copied %s to the toUser buffer\n", device->minorNumber, __FUNCTION__, __LINE__, toUser);
    for (size_t i = 0U; i < (count - 1U); ++i) {
        device->string.popFront(&device->string); // remove as many elements from the string as we copied to the user.
    }
    PRINT_DEBUG("device %d in %s popped stuff from the front of my string, it now looks like this: %s\n", device->minorNumber, __FUNCTION__, device->string.data(&device->string));
    
    int retCode = copy_to_user(user, // copy to user space
                               toUser, // copy from this buffer
                               count); // this amount of elements
    PRINT_DEBUG("device %d in %s copied the data to user space\n", device->minorNumber, __FUNCTION__);
    if (retCode != 0) {
        PRINT_DEBUG("ERROR: device %d in %s in line %d copy_to_user failed with %d\n", device->minorNumber, __FUNCTION__, __LINE__, retCode);
        return -EFAULT;
    }    
            
    up(&device->sem); // release semaphore
    PRINT_DEBUG("device %d in %s in line %d: released semaphore\n", device->minorNumber, __FUNCTION__, __LINE__);
    wake_up(&device->q); // wake those that sleep in queue 
    kfree(toUser); // release that buffer we used.
    PRINT_DEBUG("device %d exiting %s with count: %u\n", device->minorNumber, __FUNCTION__, count);
    return count; // return how many bytes where actually read.
} // end transDeviceRead
