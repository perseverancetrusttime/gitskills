#ifndef STDDEF_H
#define STDDEF_H

#undef                              NULL
#ifndef __cplusplus
#define NULL                        ((void *)0)
#else   /* C++ */
#define NULL                        0
#endif  /* C++ */

#define offsetof(s,m) (size_t)&(((s *)0)->m)

#endif

