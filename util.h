#ifndef _UTIL_H
#define _UTIL_H

#define max(x, y)                                                              \
  ({                                                                           \
    __auto_type __x = (x);                                                     \
    __auto_type __y = (y);                                                     \
    __x > __y ? __x : __y;                                                     \
  })

#define min(x, y)                                                              \
  ({                                                                           \
    __auto_type __x = (x);                                                     \
    __auto_type __y = (y);                                                     \
    __x < __y ? __x : __y;                                                     \
  })

#endif // !_UTIL_H
