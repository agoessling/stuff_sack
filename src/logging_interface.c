#include "src/logging_interface.h"

#ifdef __STDC_HOSTED__

#include <stdio.h>

int SsWriteFile(void *fd, const void *data, unsigned int len) {
  return fwrite(data, 1, len, fd);
}

int SsReadFile(void *fd, void *data, unsigned int len) {
  return fread(data, 1, len, fd);
}

#else

int SsWriteFile(void *fd, const void *data, unsigned int len) {
  return -1;
}

int SsReadFile(void *fd, void *data, unsigned int len) {
  return -1;
}

#endif
