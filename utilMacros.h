#ifndef MACRO_UTILS_H
#define MACRO_UTILS_H

#define OFFSET_OF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
#define LENGTH_OF(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

#endif /*MACRO_UTILS_H*/
