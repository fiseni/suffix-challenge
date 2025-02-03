#include <stdlib.h>
#include <stdio.h>
#include "mmap_utils.h"

#if defined(_WIN32) || defined(_WIN64)
// Windows includes
#include <windows.h>
#else
// POSIX includes
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

const char *read_file(const char *filePath, size_t *fileSizeOut, size_t *lineCountOut) {
    size_t fileSize;
    char *data;

#if defined(_WIN32) || defined(_WIN64)
    // Windows implementation
    HANDLE hFile = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to open file: %s\n", filePath);
        exit(EXIT_FAILURE);
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        fprintf(stderr, "Failed to get file size: %s\n", filePath);
        CloseHandle(hFile);
        exit(EXIT_FAILURE);
    }
    fileSize = (size_t)size.QuadPart;
    if (fileSize == 0) {
        fprintf(stderr, "Empty file: %s\n", filePath);
        CloseHandle(hFile);
        exit(EXIT_FAILURE);
    }

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL) {
        fprintf(stderr, "Failed to create file mapping: %s\n", filePath);
        CloseHandle(hFile);
        exit(EXIT_FAILURE);
    }

    data = (char *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (data == NULL) {
        fprintf(stderr, "Failed to map view of file: %s\n", filePath);
        CloseHandle(hMap);
        CloseHandle(hFile);
        exit(EXIT_FAILURE);
    }

    CloseHandle(hMap);
    CloseHandle(hFile);
#else
    // POSIX implementation
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("fstat failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    fileSize = st.st_size;
    if (fileSize == 0) {
        fprintf(stderr, "Empty file: %s\n", filePath);
        close(fd);
        exit(EXIT_FAILURE);
    }

    data = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
#endif

    // Count the number of lines
    size_t lineCount = 0;
    for (size_t i = 0; i < fileSize; i++) {
        if (data[i] == '\n') {
            lineCount++;
        }
    }

    // If the last byte is not a newline, count the last line
    if (fileSize > 0 && data[fileSize - 1] != '\n') {
        lineCount++;
    }

    *fileSizeOut = fileSize;
    *lineCountOut = lineCount;

    return data;
}

void unmap_file(char *data, size_t fileSize) {
#if defined(_WIN32) || defined(_WIN64)
    UnmapViewOfFile(data);
#else
    munmap(data, fileSize);
#endif
}
