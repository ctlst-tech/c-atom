//
// Created by goofy on 9/11/21.
//

#ifndef HW_BRIDGE_EINT_TYPES_H
#define HW_BRIDGE_EINT_TYPES_H

typedef int boolval_t;

#define TRUE 1
#define FALSE 0

typedef enum {
    nr_na = 0,
    nr_float,
    nr_int,
    nr_string,
    nr_bool,
    nr_ptr
} result_type_t;

typedef union {
    double  f; // float
    int32_t i; // int
    boolval_t b; // boolean
    char   *s; // string
    void   *p; // special
} result_val_t;

typedef struct result_reg {
    result_type_t   type;
    result_val_t    val;
} result_reg_t;



#endif //HW_BRIDGE_EINT_TYPES_H
