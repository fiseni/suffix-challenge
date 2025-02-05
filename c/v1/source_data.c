#include <sys/stat.h>
#include "allocator.h"
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
    free((void *)data->stringBlock.blockParts);
    free((void *)data->stringBlock.blockMasterParts);

    free((void *)data->partCodesOriginal);
    free((void *)data->mpCodesOriginal);
    free((void *)data->partsAsc);
    free((void *)data->mpAsc);
    free((void *)data->mpNhAsc);
}

static thread_ret_t build_parts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const char *partsPath = args->filePath;
    SourceData *data = args->data;

    size_t lineCount;
    size_t contentSize;
    char *block = read_file(partsPath, 2, &contentSize, &lineCount);

    StringView *partCodesOriginal = allocator_alloc(lineCount * sizeof(*partCodesOriginal));
    CHECK_ALLOC(partCodesOriginal);
    Part *partsAsc = allocator_alloc(lineCount * sizeof(*partsAsc));
    CHECK_ALLOC(partsAsc);

    size_t partsIndex = 0;
    size_t blockIndex = 0;
    size_t blockUpperIndex = contentSize;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] != '\n') continue;

        size_t length = i - blockIndex;
        if (i > 0 && block[i - 1] == '\r') length--;

        StringView trimmedRecord = sv_slice_trim(block + blockIndex, length);
        partCodesOriginal[partsIndex] = trimmedRecord;

        partsAsc[partsIndex].code = sv_to_upper(&trimmedRecord, &block[blockUpperIndex]);
        partsAsc[partsIndex].codeOriginal = &partCodesOriginal[partsIndex];
        blockUpperIndex += trimmedRecord.length;

        partsIndex++;
        blockIndex = i + 1;
    }

    merge_sort_by_code_length(partsAsc, partsIndex);

    data->partCodesOriginal = partCodesOriginal;
    data->partsCount = partsIndex;
    data->partsAsc = partsAsc;
    data->partsAscCount = partsIndex;
    data->stringBlock.blockParts = block;
    return 0;
}

static thread_ret_t build_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const char *masterPartsPath = args->filePath;
    SourceData *data = args->data;

    size_t lineCount;
    size_t contentSize;
    char *block = read_file(masterPartsPath, 3, &contentSize, &lineCount);

    StringView *mpCodesOriginal = allocator_alloc(lineCount * sizeof(*mpCodesOriginal));
    CHECK_ALLOC(mpCodesOriginal);
    Part *mpAsc = allocator_alloc(lineCount * sizeof(*mpAsc));
    CHECK_ALLOC(mpAsc);
    Part *mpNhAsc = allocator_alloc(lineCount * sizeof(*mpNhAsc));
    CHECK_ALLOC(mpNhAsc);

    size_t mpIndex = 0;
    size_t mpNhIndex = 0;
    size_t blockIndex = 0;
    size_t blockIndexExtra = contentSize;
    bool containsHyphens = false;

    for (size_t i = 0; i < contentSize; i++) {
        if (block[i] == CHAR_HYPHEN) containsHyphens = true;
        if (block[i] != '\n') continue;

        size_t length = i - blockIndex;
        if (i > 0 && block[i - 1] == '\r') length--;

        StringView trimmedRecord = sv_slice_trim(block + blockIndex, length);

        if (trimmedRecord.length >= MIN_STRING_LENGTH) {
            mpCodesOriginal[mpIndex] = trimmedRecord;

            StringView upperRecord = sv_to_upper(&trimmedRecord, &block[blockIndexExtra]);
            mpAsc[mpIndex].code = upperRecord;
            mpAsc[mpIndex].codeOriginal = &mpCodesOriginal[mpIndex];
            blockIndexExtra += upperRecord.length;

            if (containsHyphens) {
                StringView nhRecord = sv_to_no_hyphens(&upperRecord, &block[blockIndexExtra]);
                mpNhAsc[mpNhIndex].code = nhRecord;
                mpNhAsc[mpNhIndex].codeOriginal = &mpCodesOriginal[mpIndex];
                mpNhIndex++;
                blockIndexExtra += nhRecord.length;
            }

            mpIndex++;
        }
        blockIndex = i + 1;
        containsHyphens = false;
    }

    merge_sort_by_code_length(mpAsc, mpIndex);
    merge_sort_by_code_length(mpNhAsc, mpNhIndex);

    data->mpCodesOriginal = mpCodesOriginal;
    data->mpCount= mpIndex;
    data->mpAsc = mpAsc;
    data->mpAscCount = mpIndex;
    data->mpNhAsc = mpNhAsc;
    data->mpNhAscCount = mpNhIndex;
    data->stringBlock.blockMasterParts = block;
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
    char *block = allocator_alloc(blockSize);
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
        if (array[i].code.length <= array[j].code.length) {
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
    Part *tempArray = allocator_alloc(size * sizeof(Part));
    CHECK_ALLOC(tempArray);
    merge_sort_recursive(array, tempArray, 0, size - 1);
    //free(tempArray); // We switched to allocator.
}
