#include "src/logging.h"

#include <stdbool.h>
#include <stdint.h>

#include "src/logging_interface.h"

int SsFindLogDelimiter(void *fd) {
  const int kDelimLen = sizeof(kSsLogDelimiter) - 1;
  uint8_t buf[4 * kDelimLen];

  int file_index = 0;
  int delim_index = 0;
  while (true) {
    const int ret = SsReadFile(fd, buf, sizeof(buf));
    if (ret < 0) return ret;

    for (int i = 0; i < ret; ++i) {
      if (buf[i] != kSsLogDelimiter[delim_index]) {
        delim_index = 0;
      }

      if (buf[i] == kSsLogDelimiter[delim_index]) {
        if (++delim_index >= kDelimLen) {
          return file_index + 1;
        }
      }

      file_index++;
    }

    if (ret != sizeof(buf)) return -1;
  }
}
