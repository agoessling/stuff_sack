#include "test/linux_logging.h"

#include <stdio.h>

int SsWriteFile(void *fd, const void *data, unsigned int len) {
  return fwrite(data, 1, len, fd);
}

int SsReadFile(void *fd, void *data, unsigned int len) {
  return fread(data, 1, len, fd);
}
