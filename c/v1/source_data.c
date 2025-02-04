#include <sys/stat.h>
#include "thread_utils.h"
#include "common.h"
#include "source_data.h"

static thread_ret_t build_parts(thread_arg_t arg);
static thread_ret_t build_masterParts(thread_arg_t arg);
static char *read_file(const char *filePath, unsigned int sizeFactor, size_t *contentSizeOut, size_t *lineCountOut);
static void merge_sort_by_code_length(Part *array, size_t size);

typedef struct ThreadArgs {
    const char *filePath;
    SourceData *data;
} ThreadArgs;

void source_data_load(SourceData *data, const char *partsFile, const char *masterPartsFile) {
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

    merge_sort_by_code_length(partsAsc, partsIndex);

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
    char *block = read_file(masterPartsPath, 3, &contentSize, &lineCount);

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

    merge_sort_by_code_length(mpAsc, mpIndex);
    merge_sort_by_code_length(mpNhAsc, mpNhIndex);

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


/*  We need stable sorting algorithm.
    I was using the built-in qsort (unstable) with a custom compare function that additionally uses the index to make it stable.
    But it was slower (~40 ms overall) than the hand rolled merge sort, which is a stable algorithm.
    For reference, here is the comparison function I used for qsort.

    static int compare_by_code_length_asc(const void *a, const void *b) {
        const Part *partA = (const Part *)a;
        const Part *partB = (const Part *)b;
        return partA->codeLength == partB->codeLength
            ? (int)partA->index - (int)partB->index
            : (int)partA->codeLength - (int)partB->codeLength;
    }
*/

static void merge(Part *array, Part *tempArray, size_t left, size_t mid, size_t right) {
    size_t i = left, j = mid + 1, k = left;

    while (i <= mid && j <= right) {
        if (array[i].codeLength <= array[j].codeLength) {
            tempArray[k++] = array[i++];
        }
        else {
            tempArray[k++] = array[j++];
        }
    }

    // I tried memcpy instead of these loops but it was slower.
    // Looping though the elements is faster.

    while (i <= mid) {
        tempArray[k++] = array[i++];
    }

    while (j <= right) {
        tempArray[k++] = array[j++];
    }

    for (i = left; i <= right; i++) {
        array[i] = tempArray[i];
    }
}

static void merge_sort_recursive(Part *array, Part *tempArray, size_t left, size_t right) {
    if (left < right) {
        size_t mid = left + (right - left) / 2;
        merge_sort_recursive(array, tempArray, left, mid);
        merge_sort_recursive(array, tempArray, mid + 1, right);
        merge(array, tempArray, left, mid, right);
    }
}

static void merge_sort_by_code_length(Part *array, size_t size) {
    Part *tempArray = malloc(size * sizeof(Part));
    CHECK_ALLOC(tempArray);
    merge_sort_recursive(array, tempArray, 0, size - 1);
    free(tempArray);
}
