#pragma once

__attribute__((weak)) int SsWriteFile(void *fd, const void *data, unsigned int len);
__attribute__((weak)) int SsReadFile(void *fd, void *data, unsigned int len);
