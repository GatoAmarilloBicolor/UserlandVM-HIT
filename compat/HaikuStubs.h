/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * HaikuStubs.h - Stub implementations for Haiku kernel functions
 * These are no-ops for running on Linux.
 */
#ifndef _HAIKU_STUBS_H
#define _HAIKU_STUBS_H

#include "HaikuCompat.h"
#include <sys/stat.h>

#ifndef __HAIKU__

/* Extended image info structure - for runtime_loader registration */
struct extended_image_info {
  struct {
    int32_t id;
    int32_t type;
    int32_t sequence;
    int32_t init_order;
    void *init_routine;
    void *term_routine;
    int32_t device;
    int64_t node;
    char name[1024];
    void *text;
    void *data;
    int32_t text_size;
    int32_t data_size;
    int32_t api_version;
    int32_t abi;
  } basic_info;
  intptr_t text_delta;
  void *symbol_table;
  void *symbol_hash;
  void *string_table;
};

/* Stub for Haiku kernel stat function */
static inline status_t _kern_read_stat(int fd, const char *path,
                                       bool traverseLink, struct stat *stat_buf,
                                       size_t statSize) {
  (void)traverseLink;
  (void)statSize;
  if (path) {
    return (status_t)stat(path, stat_buf);
  }
  return (status_t)fstat(fd, stat_buf);
}

/* Stub for image registration - no-op on Linux */
static inline status_t _kern_register_image(struct extended_image_info *info,
                                            size_t size) {
  (void)info;
  (void)size;
  return B_OK;
}

/* Stub for image unregistration - no-op on Linux */
static inline status_t _kern_unregister_image(image_id id) {
  (void)id;
  return B_OK;
}

#endif /* !__HAIKU__ */

#endif /* _HAIKU_STUBS_H */
