#include "Caesar.h"

static void incrementCharPointer(char const **ptr, char const *begin, char const *end);
static char caesarChar(char getNextOfThis, size_t incBy, BOOL encode);
static void ensureCharPointerIsValid(char const **ptr, char const *begin, char const *end);
static void decrementCharPointer(char const **ptr, char const *begin, char const *end);
static void caesarString(char *string, size_t caesarOffset, BOOL encode);

extern char *alphabet; // from module.c
extern size_t const alphBufSiz; // from module.c

static char caesarChar(char caesarThis, size_t changeBy, BOOL encode) {
    char const *end // pointer to the end of the string
        = alphabet + (alphBufSiz - (size_t)2U); // If the array is of size 4 adding the size (4) to the pointer to the beginning of the array will make it point to &arr[4], which is out of bounds by 1, thus we subtract 1. Then we subtract 1 again, because a string of 4 chars is of size 5 as it has to hold the '\0' char.

    char const *pointer = strchr(alphabet, caesarThis);
    if (pointer == NULL) {
        return caesarThis;
    }
    for (size_t i = 0U; i < changeBy; ++i) {
        if (encode) {
            incrementCharPointer(&pointer, // address of the pointer to modify
                                 alphabet, // pointer to the beginning of the string
                                 end // pointer to the end of the string. 
                                );
        } else {
            decrementCharPointer(&pointer, alphabet, end);
        }
    }
    return *pointer;
}

static void ensureCharPointerIsValid(char const **ptr, char const *begin, char const *end) {
    if (ptr == NULL || *ptr == NULL || *ptr < begin || *ptr > end) {
        PRINT_DEBUG("ptr in ensureCharPointerIsValid was invalid.\n");
    }
}

static void incrementCharPointer(char const **ptr, char const *begin, char const *end) {
    ensureCharPointerIsValid(ptr, begin, end);
    if (*ptr == end) {
        *ptr = begin;
    } else {
        ++*ptr;
    }
}

static void decrementCharPointer(char const **ptr, char const *begin, char const *end) {
    ensureCharPointerIsValid(ptr, begin, end);
    if (*ptr == begin) {
        *ptr = end;
    } else {
        --*ptr;
    }
}

static void caesarString(char *string, // must be a valid C-String (zero terminated)
                  size_t caesarOffset, BOOL encode) {
    for (char *iter = string;
         *iter != '\0';
         ++iter) {
        *iter = caesarChar(*iter, caesarOffset, encode);
    }
}

void encodeString(char *string, size_t incBy) {
    caesarString(string, incBy, TRUE);
}

void decodeString(char *string, size_t decBy) {
    caesarString(string, decBy, FALSE);
}
