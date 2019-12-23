#include "Module.h"

MODULE_LICENSE("GPL"); /* GPL license allows usage of all kernel functions. */
MODULE_AUTHOR("CppPhil, Shryne");
MODULE_DESCRIPTION("Two devices may be created from this kernel module. The 0th device trans0 encodes strings written to it using caesar encoding with an offset of 3. Reading from trans0 will return the encoded string. trans1 decodes strings written to it and returns the decoded strings when read from. Run install.sh as super user after compiling this kernel module to create the devices in /dev/");
MODULE_SUPPORTED_DEVICE("none");

static int majorNumber = -1; // -1 is a dummy value, this is initialized in moduleInit
static int bufSize = BUFFERSIZE; // this may be replaced
module_param(bufSize, int, 0); // here ^^
MODULE_PARM_DESC(bufSize, "The maximum size of the buffer");
static int transOffset = TRANS_OFFSET;
module_param(transOffset, int, 0);
MODULE_PARM_DESC(transOffset, "The offset by which to caesar.");
int *pTransOffset = NULL;

TransDevice *devices = NULL; // also used in device.c
char *alphabet = NULL; // also used in caesar.c
size_t const alphBufSiz = (CHARS_IN_ALPHABET * 2) + 2; // 26 * 2 == 52 (26 lower and upper case letters each); + 2: 1 for the '\0' and and for ' '; also used in caesar.c

static struct file_operations fops = { /* set up the file opecations */
    .owner = THIS_MODULE,
    .read = &transDeviceRead,
    .open = &transDeviceOpen,
    .release = &transDeviceClose,
    .write = &transDeviceWrite,
};

static int __init moduleInit(void) {
    int errorCode = -1;
    if (bufSize <= 0) {
        PRINT_DEBUG("bufSize is not large enough, it was 0 or less.\n");
    } // end if
    
    int retVal = register_chrdev(MAJOR_NUMBER, DRIVER_NAME, &fops); // since a dynamic major number is used this returns 0 on error and the major number on success.
    if (retVal == 0) {
        PRINT_DEBUG("register_chrdev failed.\n");
        return -EIO;
    } // end if
    majorNumber = retVal; // set the global majorNumber to the major number we got from the system
    
    pTransOffset = HEAP_ALLOC8(sizeof(int) * 1U);
    if (pTransOffset == NULL) {
        PRINT_DEBUG("Failed to allocate memory for pTransOffset\n");
        errorCode = -ENOMEM;
        goto error;
    }
    *pTransOffset = transOffset;
    
    devices = HEAP_ALLOC8(sizeof(TransDevice) * NUM_DEVICES);
    
    if (devices == NULL) {
        PRINT_DEBUG("Failed to allocate memory for the devices\n");
        errorCode = -ENOMEM;
        goto error;
    } // end if
    
    alphabet = HEAP_ALLOC8(sizeof(char) * alphBufSiz);

    if (alphabet == NULL) {
        PRINT_DEBUG("Failed to allocate memory for the alphabet\n");
        errorCode = -ENOMEM;
        goto error;
    }
    
    strcpy(alphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz");
    
    for (ssize_t i = 0; i < NUM_DEVICES; ++i) {
        sema_init(&devices[i].sem, 1);
        init_waitqueue_head(&devices[i].q);
        devices[i].string = createString();
        devices[i].string.PRIVATEchangeCapacity(&devices[i].string, (string_size_type)bufSize);
        devices[i].maxBufSize = bufSize;
    } // end for    
    return EXIT_OK;
    
error:
    moduleExit();
    return errorCode;
} // end moduleInit

static void moduleExit(void) {
    PRINT_DEBUG("moduleExit called\n");
    kfree(pTransOffset);
    if (devices == NULL) {
        return;
    } // end if
    
    for (ssize_t i = 0; i < NUM_DEVICES; ++i) {
        devices[i].string.destructor(&devices[i].string); // free all the buffers of all the devices
    } // end for
    kfree(devices); // free the devices
    kfree(alphabet);
    unregister_chrdev(majorNumber,
                      DRIVER_NAME);
} // end moduleExit

module_init(moduleInit);
module_exit(moduleExit);
