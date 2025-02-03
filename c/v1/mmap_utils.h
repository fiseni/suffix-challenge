#ifndef MMAP_UTILS_H
#define MMAP_UTILS_H

const char *read_file(const char *filePath, size_t *fileSizeOut, size_t *lineCountOut);
void unmap_file(char *data, size_t fileSize);

#endif
