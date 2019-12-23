#include "String.h"

#define STRING_GROWTH_FACTOR    (string_size_type)2U

#define ZERO    (string_size_type)0U
#define ONE     (string_size_type)1U

#ifdef NDEBUG
#   define RELEASE_MODE
#else
#   define DEBUG_MODE
#endif

#ifdef DEBUG_MODE
#   define ENABLE_BOUNDS_CHECKS
#endif

static void destructor(struct String_ *string);
static void PRIVATEchangeCapacity(struct String_ *string, string_size_type newCapacity);
static void PRIVATEexpandCapacity(struct String_ *string, string_size_type growBy);
static void PRIVATEprintStatus(struct String_ const *string);
static string_size_type size(struct String_ const *string);
static string_size_type capacity(struct String_ const *string);
static string_size_type PRIVATEbufferSize(struct String_ const *string);
static string_value_type *data(struct String_ const *string);
static string_value_type *at(struct String_ const *string, string_size_type index);
static string_value_type *front(struct String_ const *string);
static string_value_type *back(struct String_ const *string);
static void clear(struct String_ *string);
static void PRIVATEensureNotFreed(struct String_ const *string, string_value_type const *func);
static BOOL isEmpty(struct String_ const *string);
static void shrinkToFit(struct String_ *string);
static void fromBuffer(struct String_ *string, string_value_type const *buffer);
static void PRIVATEgrowToAppend(struct String_ *string, string_size_type newCharsNeeded);
static BOOL PRIVATEcanBeAppended(struct String_ const *string, string_size_type charsNeeded);
static BOOL PRIVATEfits(struct String_ const *string, string_size_type otherLen);
static void PRIVATEgrowToFit(struct String_ *string, string_size_type fitThis);
static void toBuffer(struct String_ const *string, string_value_type *bufferToModify, string_size_type bufSiz);
static void append(struct String_ *string, string_value_type const *appendMe);
static int compare(struct String_ const *string, string_value_type const *other);
static BOOL equals(struct String_ const *string, string_value_type const *other);
static void fillWith(struct String_ *string, string_value_type character);
static void pushBack(struct String_ *receiver, string_value_type theChar);
static string_value_type popBack(struct String_ *receiver);
static void ensureNotNull(struct String_ const *string, const char *functionName);
static void pushFront(struct String_ *receiver, string_value_type theChar);
static void prepend(struct String_ *receiver, string_value_type const *str);
static string_value_type popFront(struct String_ *receiver);
static void *myRealloc(void *ptr, size_t oldSize, size_t newSize);
static void assertTrue(BOOL boolean, char const *funcName);

String createString(void) {
    String str;
    string_value_type *p = (string_value_type *)HEAP_ALLOC8(ONE * sizeof(string_value_type));
    if (p == NULL) {
        PRINT_DEBUG("HEAP_ALLOC8 failed in %s.\n", __FUNCTION__);
        return str;
    }
    // public begin
    str.destructor = &destructor;
    str.size = &size;
    str.capacity = &capacity;
    str.data = &data;
    str.at = &at;
    str.front = &front;
    str.back = &back;
    str.clear = &clear;
    str.isEmpty = &isEmpty;
    str.shrinkToFit = &shrinkToFit;
    str.fromBuffer = &fromBuffer;
    str.toBuffer = &toBuffer;
    str.append = &append;
    str.compare = &compare;
    str.equals = &equals;
    str.fillWith = &fillWith;
    str.pushBack = &pushBack;
    str.popBack = &popBack;
    str.pushFront = &pushFront;
    str.prepend = &prepend;
    str.popFront = &popFront;
    // public end

    // private begin
    str.PRIVATEbufferSize = &PRIVATEbufferSize;
    str.PRIVATEchangeCapacity = &PRIVATEchangeCapacity;
    str.PRIVATEexpandCapacity = &PRIVATEexpandCapacity;
    str.PRIVATEprintStatus = &PRIVATEprintStatus;
    str.PRIVATEensureNotFreed = &PRIVATEensureNotFreed;
    str.PRIVATEgrowToAppend = &PRIVATEgrowToAppend;
    str.PRIVATEcanBeAppended = &PRIVATEcanBeAppended;
    str.PRIVATEfits = &PRIVATEfits;
    str.PRIVATEgrowToFit = &PRIVATEgrowToFit;

    // data members begin
    str.PRIVATEdata_ = p;
    str.PRIVATEcapacity_ = ZERO;
    str.PRIVATEsize_ = ZERO;
    str.PRIVATEwasFreed_ = false;
    str.PRIVATEgrowthFactor = STRING_GROWTH_FACTOR;
    // data member end

    // private end

    return str;
}

static void destructor(struct String_ *string) {
    ensureNotNull(string, __FUNCTION__);
    if (string->PRIVATEwasFreed_) {
        PRINT_DEBUG("attempted to free a String that was already freed in %s\n", __FUNCTION__);
        return;
    }
    string->clear(string);
    string->PRIVATEcapacity_ = ZERO;
    kfree(string->PRIVATEdata_);
    string->PRIVATEwasFreed_ = TRUE;
    string->PRIVATEdata_ = NULL;
}

static void PRIVATEchangeCapacity(struct String_ *string, string_size_type newCapacity) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    if (newCapacity == ZERO) {
        PRINT_DEBUG("Attempted to free string by passing a capacity_ of %u in %s.\n", newCapacity, __FUNCTION__);
        return;
    }
    if (string->PRIVATEdata_ == NULL) {
        PRINT_DEBUG("data_ in string was NULL in %s this would behave like a call to malloc and is most likely unintended.\n", __FUNCTION__);
        return;
    }
    string_value_type *ret = (string_value_type *)myRealloc(string->PRIVATEdata_, string->PRIVATEcapacity_ + ONE, newCapacity + ONE);
    if (ret == NULL) {
        PRINT_DEBUG("Failed to allocate memory in %s\n", __FUNCTION__);
        return;
    }
    string->PRIVATEdata_ = ret;
    string->PRIVATEcapacity_ = newCapacity;
    string->PRIVATEdata_[newCapacity] = '\0';
}

static void PRIVATEexpandCapacity(struct String_ *string, string_size_type growBy) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type newSize = string->PRIVATEcapacity_ + growBy;
    string->PRIVATEchangeCapacity(string, newSize);
}

static void PRIVATEprintStatus(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    PRINT_DEBUG("String content: %s\n"
                "String capacity_: %u\n"
                "was freed?: %s\n",
                string->PRIVATEdata_,
                string->PRIVATEcapacity_,
                string->PRIVATEwasFreed_ ? "true" : "false"
               );
    PRINT_DEBUG("Hex dump:");
    string_size_type strl = strlen(string->PRIVATEdata_);
    for (string_size_type i = ZERO; i < strl; ++i) {
        PRINT_DEBUG("%x", string->PRIVATEdata_[i]);
    }
    PRINT_DEBUG("%s", "\n\n");
}

static string_size_type size(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    return string->PRIVATEsize_;
}

static string_size_type capacity(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    return string->PRIVATEcapacity_;
}

static string_size_type PRIVATEbufferSize(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    return string->PRIVATEcapacity_ + ONE;
}

static string_value_type *data(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    return string->PRIVATEdata_;
}

static string_value_type *at(struct String_ const *string, string_size_type index) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
#   ifdef ENABLE_BOUNDS_CHECKS
    if (index >= string->size(string)) {
        PRINT_DEBUG("Tried to access string %s with size %u at index %u", string->data(string), string->size(string), index);
        return NULL;
    }
#   endif

    return string->data(string) + index;
}

static string_value_type *front(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    return string->at(string, ZERO);
}

static string_value_type *back(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    return string->at(string, string->size(string) - ONE);
}

static void clear(struct String_ *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    memset(string->data(string),
           0,
           (string->capacity(string) + ONE) *
           sizeof(string_value_type)
          );
    string->PRIVATEsize_ = ZERO;
}

static void PRIVATEensureNotFreed(struct String_ const *string, string_value_type const *func) {
    assertTrue((func != NULL), "func in PRIVATEensureNotFreed was null.");
    ensureNotNull(string, __FUNCTION__);
    if (string->PRIVATEwasFreed_) {
        PRINT_DEBUG("Illegal operation in function %s on a freed string.\nIf this string was freed reconstruct it first.\n", func);
        return;
    }
}

static BOOL isEmpty(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    return string->size(string) == ZERO;
}

static void shrinkToFit(struct String_ *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEchangeCapacity(string, string->size(string));
}

static void fromBuffer(struct String_ *string, string_value_type const *buffer) {
    assertTrue((buffer != NULL), "buffer in fromBuffer was null!");
    ensureNotNull(string, __FUNCTION__);
    string_size_type lenOfBuf = strlen(buffer);
    if (!string->PRIVATEfits(string, lenOfBuf)) {
        string->PRIVATEgrowToFit(string, lenOfBuf);
    }
    string->clear(string);
    strcpy(string->data(string), buffer);
    string->PRIVATEsize_ = lenOfBuf;
}

static void PRIVATEgrowToAppend(struct String_ *string, string_size_type newCharsNeeded) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type growthFactor = string->PRIVATEgrowthFactor;
    string_size_type oldSize = string->size(string);
    string_size_type reqSize = oldSize + newCharsNeeded;
    string_size_type newCap = (string_size_type)(reqSize * growthFactor);
    string->PRIVATEchangeCapacity(string, newCap);
}

static BOOL PRIVATEcanBeAppended(struct String_ const *string, string_size_type charsNeeded) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type oldCap = string->capacity(string);
    string_size_type oldSize = string->size(string);
    string_size_type diff = oldCap - oldSize;
    return charsNeeded <= diff;
}

static BOOL PRIVATEfits(struct String_ const *string, string_size_type otherLen) {
    ensureNotNull(string, __FUNCTION__);
    return string->capacity(string) >= otherLen;
}

static void PRIVATEgrowToFit(struct String_ *string, string_size_type fitThis) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type growTo = (string_size_type)(string->PRIVATEgrowthFactor * fitThis);
    string->PRIVATEchangeCapacity(string, growTo);
}

static void toBuffer(struct String_ const *string, string_value_type *bufferToModify, string_size_type bufSiz) {
    assertTrue((bufferToModify != NULL), "bufferToModify in toBuffer was null!");
    ensureNotNull(string, __FUNCTION__);
    strncpy(bufferToModify, string->data(string), bufSiz);
    bufferToModify[bufSiz - ONE] = '\0';
}

static void append(struct String_ *string, string_value_type const *appendMe) {
    assertTrue((appendMe != NULL), "appendMe in append was null!");
    ensureNotNull(string, __FUNCTION__);
    string_size_type len = strlen(appendMe);
    if (!string->PRIVATEcanBeAppended(string, len)) {
        string->PRIVATEgrowToAppend(string, len);
    }
    strcat(string->data(string), appendMe);
    string->PRIVATEsize_ += len;
}

static int compare(struct String_ const *string, string_value_type const *other) {
    assertTrue((other != NULL), "other in compare was NULL!");
    ensureNotNull(string, __FUNCTION__);
    return strcmp(string->data(string), other);
}

static BOOL equals(struct String_ const *string, string_value_type const *other) {
    assertTrue((other != NULL), "other in equals was NULL!");
    ensureNotNull(string, __FUNCTION__);
    return string->compare(string, other) == 0;
}

static void fillWith(struct String_ *string, string_value_type character) {
    ensureNotNull(string, __FUNCTION__);
    memset(string->data(string), (int)character, string->size(string));
}

static void pushBack(struct String_ *receiver, string_value_type theChar) {
    ensureNotNull(receiver, __FUNCTION__);
    string_value_type arr[(string_size_type)2U];
    arr[ZERO] = theChar;
    arr[ONE] = '\0';
    receiver->append(receiver, arr);
}

static string_value_type popBack(struct String_ *receiver) {
    ensureNotNull(receiver, __FUNCTION__);
    receiver->PRIVATEensureNotFreed(receiver, __FUNCTION__);
    if (receiver->isEmpty(receiver)) {
        PRINT_DEBUG("receiver was empty in %s!\n", __FUNCTION__);
        return -1;
    }
    string_value_type *ptr = receiver->back(receiver);
    string_value_type retMe = *ptr;
    *ptr = '\0';
    --receiver->PRIVATEsize_;
    return retMe;
}

static void ensureNotNull(struct String_ const *string, const char *functionName) {
    if (functionName == NULL) {
        PRINT_DEBUG("NULL pointer passed into functionName parameter in %s!\n", __FUNCTION__);
        return;
    }
    if (string == NULL) {
        PRINT_DEBUG("The String in %s was null!\n", functionName);
        return;
    }
}

static void pushFront(struct String_ *receiver, string_value_type theChar) {
    string_size_type len = receiver->size(receiver);
    if (!receiver->PRIVATEcanBeAppended(receiver, ONE)) {
        receiver->PRIVATEgrowToAppend(receiver, ONE);
    }

    string_value_type *pBuf = receiver->data(receiver);
    memmove(pBuf + ONE, pBuf, len * sizeof(string_value_type));
    *pBuf = theChar;
    pBuf[len + ONE] = '\0';
    ++receiver->PRIVATEsize_;
}

static void prepend(struct String_ *receiver, string_value_type const *str) {
    assertTrue((str != NULL), "str in prepend was null");
    string_size_type size = receiver->size(receiver) + ONE;
    string_value_type *buf = HEAP_ALLOC8(size * sizeof(string_value_type));
    assertTrue((buf != NULL), "buf was null in prepend");
    receiver->toBuffer(receiver, buf, size);
    receiver->clear(receiver);
    receiver->fromBuffer(receiver, str);
    receiver->append(receiver, buf);
    kfree(buf);
}

static string_value_type popFront(struct String_ *receiver) {
    ensureNotNull(receiver, __FUNCTION__);
    receiver->PRIVATEensureNotFreed(receiver, __FUNCTION__);
    if (receiver->isEmpty(receiver)) {
        PRINT_DEBUG("receiver was empty in %s!\n", __FUNCTION__);
        return -1;
    }
    string_size_type len = receiver->size(receiver);
    string_value_type *pBuf = receiver->data(receiver);
    string_value_type ret = *pBuf;
    memmove(pBuf, pBuf + ONE, (len - ONE) * sizeof(string_value_type));
    pBuf[len - ONE] = '\0';
    --receiver->PRIVATEsize_;
    return ret;
}

static void *myRealloc(void *ptr, size_t oldSize, size_t newSize) {
    if (ptr == NULL) {
        goto err;
    }
    void *newMem = HEAP_ALLOC8(newSize); // allocate the new memory
    memcpy(newMem, ptr, oldSize); // copy the old data to the new memory
    kfree(ptr); // free the old memory
    return newMem;
    
err:
    return NULL;
}

static void assertTrue(BOOL boolean, char const *funcName) {
#ifdef DEBUG_MODE
    if (!boolean) {
        PRINT_DEBUG("ASSERTION VIOLATED: %s\n", funcName);
    }
#endif
}
