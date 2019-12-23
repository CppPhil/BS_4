#include "String.h"

//#define READ_INTO_STRING_DEBUG_MESSAGES   /* uncomment to enable debug messages for readFrom */

#define FIXEDBUFSIZ (string_size_type)21U /* This is the size of chars (bytes) to be read at once (per block) from the input stream, note that the last byte is used for '\0' */

#define STRING_GROWTH_FACTOR    1.5

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
static void readFrom(struct String_ *string, FILE *stream);
static void writeTo(struct String_ const *string, FILE *stream);
static string_size_type size(struct String_ const *string);
static string_size_type capacity(struct String_ const *string);
static string_size_type PRIVATEbufferSize(struct String_ const *string);
static string_value_type *data(struct String_ const *string);
static string_value_type *at(struct String_ const *string, string_size_type index);
static string_value_type *front(struct String_ const *string);
static string_value_type *back(struct String_ const *string);
static void clear(struct String_ *string);
static void PRIVATEensureNotFreed(struct String_ const *string, string_value_type const *func);
static bool isEmpty(struct String_ const *string);
static void shrinkToFit(struct String_ *string);
static void fromBuffer(struct String_ *string, string_value_type const *buffer);
static void PRIVATEgrowToAppend(struct String_ *string, string_size_type newCharsNeeded);
static bool PRIVATEcanBeAppended(struct String_ const *string, string_size_type charsNeeded);
static bool PRIVATEfits(struct String_ const *string, string_size_type otherLen);
static void PRIVATEgrowToFit(struct String_ *string, string_size_type fitThis);
static void toBuffer(struct String_ const *string, string_value_type *bufferToModify, string_size_type bufSiz);
static void append(struct String_ *string, string_value_type const *appendMe);
static int compare(struct String_ const *string, string_value_type const *other);
static bool equals(struct String_ const *string, string_value_type const *other);
static void fillWith(struct String_ *string, string_value_type character);
static void swap(struct String_ *receiver, struct String_ *other);
static void swapHelper(void *a, void *b, size_t bytes);
static void pushBack(struct String_ *receiver, string_value_type theChar);
static string_value_type popBack(struct String_ *receiver);
static void reverse(struct String_ *receiver);
static void readLine(struct String_ *string, FILE *stream);
static void ensureNotNull(struct String_ const *string, const char *functionName);
static void clearFixedBuf(void);
static void pushFront(struct String_ *receiver, string_value_type theChar);
static void prepend(struct String_ *receiver, string_value_type const *str);
static string_value_type popFront(struct String_ *receiver);

static string_value_type fixedBuf[FIXEDBUFSIZ] = { 0 };

String createString(void) {
    String str;
    string_value_type *p = (string_value_type *)calloc(ONE, sizeof(string_value_type));
    if (p == NULL) {
        fprintf(stderr, "calloc failed in %s.\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    // public begin
    str.destructor = &destructor;
    str.readFrom = &readFrom;
    str.writeTo = &writeTo;
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
    str.swap = &swap;
    str.pushBack = &pushBack;
    str.popBack = &popBack;
    str.reverse = &reverse;
    str.readLine = &readLine;
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
        fprintf(stderr, "attempted to free a String that was already freed in %s\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    string->clear(string);
    string->PRIVATEcapacity_ = ZERO;
    free(string->PRIVATEdata_);
    string->PRIVATEwasFreed_ = true;
    string->PRIVATEdata_ = NULL;
}

static void PRIVATEchangeCapacity(struct String_ *string, string_size_type newCapacity) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    if (newCapacity == ZERO) {
        fprintf(stderr, "Attempted to free string by passing a capacity_ of %u in %s.\n", newCapacity, __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    if (string->PRIVATEdata_ == NULL) {
        fprintf(stderr, "data_ in string was NULL in %s this would behave like a call to malloc and is most likely unintended.\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    string_value_type *ret = (string_value_type *)realloc(string->PRIVATEdata_, newCapacity + ONE);
    if (ret == NULL) {
        fprintf(stderr, "Failed to allocate memory in %s\n", __FUNCTION__);
        exit(EXIT_FAILURE);
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
    printf("String content: %s\n"
           "String capacity_: %u\n"
           "was freed?: %s\n",
           string->PRIVATEdata_,
           string->PRIVATEcapacity_,
           string->PRIVATEwasFreed_ ? "true" : "false"
          );
    puts("Hex dump:");
    string_size_type strl = strlen(string->PRIVATEdata_);
    for (string_size_type i = ZERO; i < strl; ++i) {
        printf("%x", string->PRIVATEdata_[i]);
    }
    printf("%s", "\n\n");
}

static void readFrom(struct String_ *string, FILE *stream) {
    assert((stream != NULL) && "stream was NULL in readFrom.");
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);

    string->clear(string);
    clearFixedBuf();

    string_value_type *ret = fixedBuf; // some non NULL address

    while (ret != NULL && !feof(stream)) {
        if (ferror(stream)) {
            fprintf(stderr, "%s", "An error occured in stream!\n");
            exit(EXIT_FAILURE);
        } // end if
#       ifdef READ_INTO_STRING_DEBUG_MESSAGES
        printf("Fixed buf before reading: %s\n", fixedBuf);
#       endif
        ret = fgets(fixedBuf, FIXEDBUFSIZ, stream);
#       ifdef READ_INTO_STRING_DEBUG_MESSAGES
        printf("Fixed buf after reading: %s\n", fixedBuf);
#       endif
        string_size_type lenFBuf = strlen(fixedBuf);
#       ifdef READ_INTO_STRING_DEBUG_MESSAGES
        puts("dynBuf before copying:");
        string->PRIVATEprintStatus(string);
#       endif

        if (!string->PRIVATEcanBeAppended(string, lenFBuf)) {
            string->PRIVATEgrowToAppend(string, lenFBuf);
        }

        strcat(string->PRIVATEdata_, fixedBuf);
        string->PRIVATEsize_ = strlen(string->data(string));

#       ifdef READ_INTO_STRING_DEBUG_MESSAGES
        puts("dynBuf after copying:");
        string->PRIVATEprintStatus(string);
#       endif
        clearFixedBuf();
    } // end while

#   ifdef READ_INTO_STRING_DEBUG_MESSAGES
    printf("Fixed buf at the end : %s\n", fixedBuf);
    puts("dynBuf at the end:");
    string->PRIVATEprintStatus(string);
#   endif
    clearerr(stream);
}

static void writeTo(struct String_ const *string, FILE *stream) {
    assert((stream != NULL) && "stream was NULL in writeTo.");
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEensureNotFreed(string, __FUNCTION__);
    fprintf(stream, "%s", string->PRIVATEdata_);
    fflush(stream);
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
        fprintf(stderr, "Tried to access string %s with size %u at index %u", string->data(string), string->size(string), index);
        exit(EXIT_FAILURE);
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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4189)
#endif
    // ReSharper disable once CppEntityNeverUsed
    void volatile *p01 = memset(string->data(string),
                                0,
                                (string->capacity(string) + ONE) *
                                sizeof(string_value_type));
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    string->PRIVATEsize_ = ZERO;
}

static void PRIVATEensureNotFreed(struct String_ const *string, string_value_type const *func) {
    assert((func != NULL) && "func in PRIVATEensureNotFreed was null.");
    ensureNotNull(string, __FUNCTION__);
    if (string->PRIVATEwasFreed_) {
        fprintf(stderr, "Illegal operation in function %s on a freed string.\nIf this string was freed reconstruct it first.\n", func);
        exit(EXIT_FAILURE);
    }
}

static bool isEmpty(struct String_ const *string) {
    ensureNotNull(string, __FUNCTION__);
    return string->size(string) == ZERO;
}

static void shrinkToFit(struct String_ *string) {
    ensureNotNull(string, __FUNCTION__);
    string->PRIVATEchangeCapacity(string, string->size(string));
}

static void fromBuffer(struct String_ *string, string_value_type const *buffer) {
    assert((buffer != NULL) && "buffer in fromBuffer was null!");
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
    double growthFactor = string->PRIVATEgrowthFactor;
    string_size_type oldSize = string->size(string);
    string_size_type reqSize = oldSize + newCharsNeeded;
    string_size_type newCap = (string_size_type)(reqSize * growthFactor);
    string->PRIVATEchangeCapacity(string, newCap);
}

static bool PRIVATEcanBeAppended(struct String_ const *string, string_size_type charsNeeded) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type oldCap = string->capacity(string);
    string_size_type oldSize = string->size(string);
    string_size_type diff = oldCap - oldSize;
    return charsNeeded <= diff;
}

static bool PRIVATEfits(struct String_ const *string, string_size_type otherLen) {
    ensureNotNull(string, __FUNCTION__);
    return string->capacity(string) >= otherLen;
}

static void PRIVATEgrowToFit(struct String_ *string, string_size_type fitThis) {
    ensureNotNull(string, __FUNCTION__);
    string_size_type growTo = (string_size_type)(string->PRIVATEgrowthFactor * fitThis);
    string->PRIVATEchangeCapacity(string, growTo);
}

static void toBuffer(struct String_ const *string, string_value_type *bufferToModify, string_size_type bufSiz) {
    assert((bufferToModify != NULL) && "bufferToModify in toBuffer was null!");
    ensureNotNull(string, __FUNCTION__);
    strncpy(bufferToModify, string->data(string), bufSiz);
    bufferToModify[bufSiz - ONE] = '\0';
}

static void append(struct String_ *string, string_value_type const *appendMe) {
    assert((appendMe != NULL) && "appendMe in append was null!");
    ensureNotNull(string, __FUNCTION__);
    string_size_type len = strlen(appendMe);
    if (!string->PRIVATEcanBeAppended(string, len)) {
        string->PRIVATEgrowToAppend(string, len);
    }
    strcat(string->data(string), appendMe);
    string->PRIVATEsize_ += len;
}

static int compare(struct String_ const *string, string_value_type const *other) {
    assert((other != NULL) && "other in compare was NULL!");
    ensureNotNull(string, __FUNCTION__);
    return strcmp(string->data(string), other);
}

static bool equals(struct String_ const *string, string_value_type const *other) {
    assert((other != NULL) && "other in equals was NULL!");
    ensureNotNull(string, __FUNCTION__);
    return string->compare(string, other) == 0;
}

static void fillWith(struct String_ *string, string_value_type character) {
    ensureNotNull(string, __FUNCTION__);
    memset(string->data(string), (int)character, string->size(string));
}

static void swap(struct String_ *receiver, struct String_ *other) {
    ensureNotNull(receiver, __FUNCTION__);
    ensureNotNull(other, __FUNCTION__);
    swapHelper(&receiver->PRIVATEdata_, &other->PRIVATEdata_, sizeof(string_value_type *));
    swapHelper(&receiver->PRIVATEcapacity_, &other->PRIVATEcapacity_, sizeof(string_size_type));
    swapHelper(&receiver->PRIVATEsize_, &other->PRIVATEsize_, sizeof(string_size_type));
    swapHelper(&receiver->PRIVATEwasFreed_, &other->PRIVATEwasFreed_, sizeof(bool));
    swapHelper(&receiver->PRIVATEgrowthFactor, &other->PRIVATEgrowthFactor, sizeof(double));
}

static void swapHelper(void *a, void *b, size_t bytes) {
    if (a == NULL || b == NULL) {
        fprintf(stderr, "A NULL argument was passed to %s!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    if (a == b) {
        return;
    }
    void *temp = malloc(bytes);
    if (temp == NULL) {
        fprintf(stderr, "malloc failed in %s\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    memcpy(temp, a, bytes); // copy a to temp
    memcpy(a, b, bytes); // copy b to a
    memcpy(b, temp, bytes); // copy temp(a) to b
    free(temp);
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
        fprintf(stderr, "receiver was empty in %s!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    string_value_type *ptr = receiver->back(receiver);
    string_value_type retMe = *ptr;
    *ptr = '\0';
    --receiver->PRIVATEsize_;
    return retMe;
}

static void reverse(struct String_ *receiver) {
    ensureNotNull(receiver, __FUNCTION__);
    string_value_type *p = receiver->data(receiver);
    string_size_type size = receiver->size(receiver);
    for (string_size_type i = ZERO; i < size / (string_size_type)2U; ++i) {
        swapHelper(&p[i], &p[size - i - ONE], sizeof(string_value_type));
    }
}

static void ensureNotNull(struct String_ const *string, const char *functionName) {
    if (functionName == NULL) {
        fprintf(stderr, "NULL pointer passed into functionName parameter in %s!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    if (string == NULL) {
        fprintf(stderr, "The String in %s was null!\n", functionName);
        exit(EXIT_FAILURE);
    }
}

static void readLine(struct String_ *string, FILE *stream) {
    ensureNotNull(string, __FUNCTION__);
    assert((stream != NULL) && "stream was NULL in readLine");
    string->PRIVATEensureNotFreed(string, __FUNCTION__);

    string->clear(string);
    clearFixedBuf();

    string_value_type *ret = fixedBuf; // some non NULL address
    string_value_type someChar = '\0'; // just some dummy char
    string_value_type *addr = &someChar;
    string_value_type *pLastChar = addr; // just to initialize

    while (ret != NULL && *pLastChar != '\n') {
        if (ferror(stream)) {
            fprintf(stderr, "%s", "An error occured in stream!\n");
            exit(EXIT_FAILURE);
        } // end if
        ret = fgets(fixedBuf, FIXEDBUFSIZ, stream);
        string_size_type lenFBuf = strlen(fixedBuf);

        if (!string->PRIVATEcanBeAppended(string, lenFBuf)) {
            string->PRIVATEgrowToAppend(string, lenFBuf);
        }

        strcat(string->PRIVATEdata_, fixedBuf);
        string->PRIVATEsize_ = strlen(string->data(string));

        string_size_type stringLength = string->size(string);
        pLastChar = string->PRIVATEdata_ + (stringLength - ONE);
        clearFixedBuf();
    } // end while

    if (pLastChar != addr && *pLastChar == '\n') {
        *pLastChar = '\0'; // replace the last '\n' with '\0'
        string->PRIVATEsize_ -= ONE;
    }
    clearerr(stream);
}

static void clearFixedBuf(void) {
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4189)
#endif
    // ReSharper disable once CppEntityNeverUsed
    volatile void *p = memset(fixedBuf, 0, FIXEDBUFSIZ * sizeof(string_value_type));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
    assert((str != NULL) && "str in prepend was null");
    string_size_type size = receiver->size(receiver) + ONE;
    string_value_type *buf = calloc(size, sizeof(string_value_type));
    assert((buf != NULL) && "buf was null in prepend");
    receiver->toBuffer(receiver, buf, size);
    receiver->clear(receiver);
    receiver->fromBuffer(receiver, str);
    receiver->append(receiver, buf);
    free(buf);
}

static string_value_type popFront(struct String_ *receiver) {
    ensureNotNull(receiver, __FUNCTION__);
    receiver->PRIVATEensureNotFreed(receiver, __FUNCTION__);
    if (receiver->isEmpty(receiver)) {
        fprintf(stderr, "receiver was empty in %s!\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    string_size_type len = receiver->size(receiver);
    string_value_type *pBuf = receiver->data(receiver);
    string_value_type ret = *pBuf;
    memmove(pBuf, pBuf + ONE, (len - ONE) * sizeof(string_value_type));
    pBuf[len - ONE] = '\0';
    --receiver->PRIVATEsize_;
    return ret;
}
