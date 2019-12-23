#ifndef String_H
#define String_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define PUBLIC_BEGIN
#define PUBLIC_END
#define PRIVATE_BEGIN
#define PRIVATE_END

#define GLUE_IMPL(a, b)  a##b
#define GLUE(a, b) GLUE_IMPL(a, b)
#define PRIVATE(identifier) GLUE(PRIVATE, identifier)  

typedef size_t string_size_type;
typedef char string_value_type;

typedef struct String_ {
    PUBLIC_BEGIN
    void (*destructor)(struct String_ *);
    void (*readFrom)(struct String_ *, FILE *);
    void (*writeTo)(struct String_ const *, FILE *);
    string_size_type (*size)(struct String_ const *);
    string_size_type (*capacity)(struct String_ const *);
    string_value_type *(*data)(struct String_ const *);
    string_value_type *(*at)(struct String_ const *, string_size_type);
    string_value_type *(*front)(struct String_ const *);
    string_value_type *(*back)(struct String_ const *);
    void (*clear)(struct String_ *);
    bool (*isEmpty)(struct String_ const *);
    void (*shrinkToFit)(struct String_ *);
    void (*fromBuffer)(struct String_ *, string_value_type const *);
    void (*toBuffer)(struct String_ const *, string_value_type *, string_size_type);
    void (*append)(struct String_ *, string_value_type const *);
    int (*compare)(struct String_ const *, string_value_type const *);
    bool (*equals)(struct String_ const *, string_value_type const *);
    void (*fillWith)(struct String_ *, string_value_type);
    void (*swap)(struct String_ *, struct String_ *);
    void (*pushBack)(struct String_ *, string_value_type);
    string_value_type (*popBack)(struct String_ *);
    void (*reverse)(struct String_ *);
    void (*readLine)(struct String_ *, FILE *);
    void (*pushFront)(struct String_ *, string_value_type);
    void (*prepend)(struct String_ *, string_value_type const *);
    string_value_type (*popFront)(struct String_ *);
    PUBLIC_END
    /*----------------------------------------------------*/
    PRIVATE_BEGIN
    string_size_type (*PRIVATE(bufferSize) )(struct String_ const *);
    void (*PRIVATE(changeCapacity) )(struct String_ *, string_size_type);
    void (*PRIVATE(expandCapacity) )(struct String_ *, string_size_type);
    void (*PRIVATE(printStatus) )(struct String_ const *);
    void (*PRIVATE(ensureNotFreed) )(struct String_ const *, string_value_type const *);
    void (*PRIVATE(growToAppend) )(struct String_ *, string_size_type);
    bool (*PRIVATE(canBeAppended) )(struct String_ const *, string_size_type);
    bool (*PRIVATE(fits) )(struct String_ const *, string_size_type);
    void (*PRIVATE(growToFit) )(struct String_ *, string_size_type);
   
    // data members:
    string_value_type *PRIVATE(data_);
    string_size_type PRIVATE(capacity_);
    string_size_type PRIVATE(size_);
    bool PRIVATE(wasFreed_);
    double PRIVATE(growthFactor);
    PRIVATE_END
} String;

String createString(void);

#undef PRIVATE
#undef GLUE_IMPL
#undef GLUE

#undef PUBLIC_BEGIN
#undef PUBLIC_END
#undef PRIVATE_BEGIN
#undef PRIVATE_END

#endif // String_H


