#include <sys/stat.h>
#include "thread_utils.h"
#include "common.h"
#include "source_data.h"

static thread_ret_t build_parts(thread_arg_t arg);
static thread_ret_t build_masterParts(thread_arg_t arg);
static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut);
static int compare_by_code_length_asc(const void *a, const void *b);

typedef struct ThreadArgs {
    const char *filePath;
    SourceData *data;
} ThreadArgs;

const SourceData *source_data_read(const char *partsFile, const char *masterPartsFile) {
    SourceData *data = (SourceData *)malloc(sizeof(*data));
    CHECK_ALLOC(data);

    thread_t thread1;
    int status = create_thread(&thread1, build_parts, &(ThreadArgs){.data = data, .filePath = partsFile });
    CHECK_THREAD_CREATE_STATUS(status, (size_t)0);
    thread_t thread2;
    status = create_thread(&thread2, build_masterParts, &(ThreadArgs){.data = data, .filePath = masterPartsFile });
    CHECK_THREAD_CREATE_STATUS(status, (size_t)0);


    status = join_thread(thread1, NULL);
    CHECK_THREAD_JOIN_STATUS(status, (size_t)0);
    status = join_thread(thread2, NULL);
    CHECK_THREAD_JOIN_STATUS(status, (size_t)0);

    return data;
}

void source_data_clean(const SourceData *data) {
    // All strings are allocated from a single block
    free((void *)data->masterPartsOriginal->code);
    free((void *)data->partsOriginal->code);

    free((void *)data->masterPartsOriginal);
    free((void *)data->masterPartsAsc);
    free((void *)data->masterPartsNhAsc);
    free((void *)data->partsOriginal);
    free((void *)data->partsAsc);
    free((void *)data);
}

static thread_ret_t build_parts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const char *partsPath = args->filePath;
    SourceData *data = args->data;

    size_t lineCount;
    size_t contentSize;
    char *block = read_file(partsPath, 2, &contentSize, &lineCount);

    Part *partsOriginal = malloc(lineCount * sizeof(*partsOriginal));
    CHECK_ALLOC(partsOriginal);
    Part *partsAsc = malloc(lineCount * sizeof(*partsAsc));
    CHECK_ALLOC(partsAsc);

    size_t partsIndex = 0;
    size_t blockIndex = 0;
    size_t blockUpperIndex = contentSize;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] != '\n') continue;

        block[i] = '\0';
        size_t length = i - blockIndex;
        if (i > 0 && block[i - 1] == '\r') {
            block[i - 1] = '\0';
            length--;
        }
        assert(partsIndex < lineCount);

        const char *trimmedRecord = str_trim_in_place(&block[blockIndex], length, &length);
        partsOriginal[partsIndex].code = trimmedRecord;
        partsOriginal[partsIndex].codeLength = length;
        partsOriginal[partsIndex].index = partsIndex;

        partsAsc[partsIndex].code = str_to_upper(trimmedRecord, length, &block[blockUpperIndex]);
        partsAsc[partsIndex].codeLength = length;
        partsAsc[partsIndex].index = partsIndex;
        blockUpperIndex += length + 1; // +1 for null terminator

        partsIndex++;
        blockIndex = i + 1;
    }

    qsort(partsAsc, partsIndex, sizeof(*partsAsc), compare_by_code_length_asc);

    data->partsOriginal = partsOriginal;
    data->partsOriginalCount = partsIndex;
    data->partsAsc = partsAsc;
    data->partsAscCount = partsIndex;
    return 0;
}

static thread_ret_t build_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const char *masterPartsPath = args->filePath;
    SourceData *data = args->data;

    size_t lineCount;
    size_t contentSize;
    char *block = read_file(masterPartsPath, 2, &contentSize, &lineCount);

    Part *mpOriginal = malloc(lineCount * sizeof(*mpOriginal));
    CHECK_ALLOC(mpOriginal);
    Part *mpAsc = malloc(lineCount * sizeof(*mpAsc));
    CHECK_ALLOC(mpAsc);
    Part *mpNhAsc = malloc(lineCount * sizeof(*mpNhAsc));
    CHECK_ALLOC(mpNhAsc);

    size_t mpIndex = 0;
    size_t mpNhIndex = 0;
    size_t blockIndex = 0;
    size_t blockIndexExtra = contentSize;
    bool containsHyphens = false;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == CHAR_HYPHEN) containsHyphens = true;
        if (block[i] != '\n') continue;

        block[i] = '\0';
        size_t length = i - blockIndex;
        if (i > 0 && block[i - 1] == '\r') {
            block[i - 1] = '\0';
            length--;
        }
        assert(mpIndex < lineCount);

        const char *trimmedRecord = str_trim_in_place(&block[blockIndex], length, &length);
        if (length >= MIN_STRING_LENGTH) {
            mpOriginal[mpIndex].code = trimmedRecord;
            mpOriginal[mpIndex].codeLength = length;
            mpOriginal[mpIndex].index = mpIndex;

            const char *upperRecord = str_to_upper(trimmedRecord, length, &block[blockIndexExtra]);
            mpAsc[mpIndex].code = upperRecord;
            mpAsc[mpIndex].codeLength = length;
            mpAsc[mpIndex].index = mpIndex;
            blockIndexExtra += length + 1; // +1 for null terminator

            if (containsHyphens) {
                size_t codeNhLength;
                mpNhAsc[mpNhIndex].code = str_remove_hyphens(upperRecord, length, &block[blockIndexExtra], &codeNhLength);
                mpNhAsc[mpNhIndex].codeLength = codeNhLength;
                mpNhAsc[mpNhIndex].index = mpIndex;
                mpNhIndex++;
                blockIndexExtra += codeNhLength + 1; // +1 for null terminator
            }

            mpIndex++;
        }
        blockIndex = i + 1;
        containsHyphens = false;
    }

    qsort(mpAsc, mpIndex, sizeof(*mpAsc), compare_by_code_length_asc);
    qsort(mpNhAsc, mpNhIndex, sizeof(*mpNhAsc), compare_by_code_length_asc);

    data->masterPartsOriginal = mpOriginal;
    data->masterPartsOriginalCount = mpIndex;
    data->masterPartsAsc = mpAsc;
    data->masterPartsAscCount = mpIndex;
    data->masterPartsNhAsc = mpNhAsc;
    data->masterPartsNhAscCount = mpNhIndex;
    return 0;
}

static long get_file_size_bytes(const char *filePath) {
    assert(filePath);

    struct stat st;
    if (stat(filePath, &st) != 0) {
        perror("stat failed");
        return -1;
    }
    return st.st_size;
}

static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut) {
    long fileSize = get_file_size_bytes(filePath);
    FILE *file = fopen(filePath, "rb");
    if (!file || fileSize == -1) {
        fprintf(stderr, "Failed to open file: %s\n", filePath);
        exit(EXIT_FAILURE);
    }
    assert(fileSize > 0);
    assert(sizeFactor > 0);

    size_t blockSize = sizeof(char) * fileSize * sizeFactor + sizeFactor;
    char *block = malloc(blockSize);
    CHECK_ALLOC(block);
    size_t contentSize = fread(block, 1, fileSize, file);
    fclose(file);

#pragma warning(disable: 6385 6386 )
    // MSVC thinks there is a buffer overrun, it can't calculate based on sizeFactor.
    // If you put a literal for sizeFactor, e.g. 1, warning goes way.

    // Handle the case where the file does not end with a newline
    if (contentSize > 0 && block[contentSize - 1] != '\n') {
        block[contentSize] = '\n';
        contentSize++;
    }

    size_t lineCount = 0;
    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == '\n') {
            lineCount++;
        }
    }
#pragma warning(default: 6385 6386  )

    *contentSizeOut = contentSize;
    *lineCountOut = lineCount;
    return block;
}

static int compare_by_code_length_asc(const void *a, const void *b) {
    const Part *partA = (const Part *)a;
    const Part *partB = (const Part *)b;
    return (partA->codeLength < partB->codeLength) ? -1
        : (partA->codeLength > partB->codeLength) ? 1
        : (partA->index < partB->index) ? -1 : (partA->index > partB->index) ? 1 : 0;
}

/*
* By applying simple qsort, the whole app is 30% faster.
* But, it's not a stable sort. It still finds the same number of matches though.
* However, in case of ties, it might return any of them.
* Requirements specify that for ties we should return the first masterPart in the file.
*/
//static int compare_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const Part *)a)->codeLength;
//    size_t lenB = ((const Part *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
