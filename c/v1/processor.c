#include "common.h"
#include "thread_utils.h"
#include "hash_table.h"
#include "source_data.h"

typedef struct PartsInfo {
    Part *parts;
    size_t partsCount;
    HTableSizeList *suffixesByLength[MAX_STRING_LENGTH];
} PartsInfo;

typedef struct MasterPartsInfo {
    MasterPart *masterParts;
    MasterPart *masterPartsNoHyphens;
    size_t masterPartsCount;
    size_t masterPartsNoHyphensCount;
    HTableString *suffixesByLength[MAX_STRING_LENGTH];
    HTableString *suffixesByNoHyphensLength[MAX_STRING_LENGTH];
} MasterPartsInfo;

typedef struct ThreadArgs {
    void *info;
    size_t startIndex;
    size_t suffixLength;
} ThreadArgs;

static void build_masterPartsInfo(const SourceData *data, MasterPartsInfo *mpInfo);
static void build_partsInfo(const SourceData *data, PartsInfo *partsInfo);
static void free_info_allocations(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo);
static void create_suffix_tables(void *info, size_t *startIndexByLength, thread_func_t func);
static thread_ret_t create_suffix_table_for_mp_code(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_mp_codeNh(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_part_code(thread_arg_t arg);
static int compare_mp_by_code_length_asc(const void *a, const void *b);
static int compare_mp_by_codeNh_length_asc(const void *a, const void *b);
static int compare_part_by_code_length_asc(const void *a, const void *b);
static void backward_fill(size_t *array);

static const size_t MAX_VALUE = ((size_t)-1);
static HTableString *dictionary = NULL;

const char *processor_find_match(const char *partCode, size_t partCodeLength) {
    const char *match = htable_string_search(dictionary, partCode, partCodeLength);
    return match;
}

void processor_initialize(const SourceData *data) {
    MasterPartsInfo masterPartsInfo = { 0 };
    build_masterPartsInfo(data, &masterPartsInfo);
    PartsInfo partsInfo = { 0 };
    build_partsInfo(data, &partsInfo);

    dictionary = htable_string_create(partsInfo.partsCount);

    for (size_t i = 0; i < partsInfo.partsCount; i++) {
        Part part = partsInfo.parts[i];

        HTableString *masterPartsBySuffix = masterPartsInfo.suffixesByLength[part.codeLength];
        if (masterPartsBySuffix) {
            const char *match = htable_string_search(masterPartsBySuffix, part.code, part.codeLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.code, part.codeLength, match);
                continue;
            }
        }
        masterPartsBySuffix = masterPartsInfo.suffixesByNoHyphensLength[part.codeLength];
        if (masterPartsBySuffix) {
            const char *match = htable_string_search(masterPartsBySuffix, part.code, part.codeLength);
            if (match) {
                htable_string_insert_if_not_exists(dictionary, part.code, part.codeLength, match);
            }
        }
    }

    for (long i = (long)masterPartsInfo.masterPartsCount - 1; i >= 0; i--) {
        MasterPart mp = masterPartsInfo.masterParts[i];

        HTableSizeList *partsBySuffix = partsInfo.suffixesByLength[mp.codeLength];
        if (partsBySuffix) {
            const ListItem *originalParts = htable_sizelist_search(partsBySuffix, mp.code, mp.codeLength);
            while (originalParts) {
                size_t originalPartIndex = originalParts->value;
                Part part = partsInfo.parts[originalPartIndex];
                htable_string_insert_if_not_exists(dictionary, part.code, part.codeLength, mp.codeOriginal);
                originalParts = originalParts->next;
            }
        }
    }

    free_info_allocations(masterPartsInfo, partsInfo);
}

static void free_info_allocations(MasterPartsInfo masterPartsInfo, PartsInfo partsInfo) {
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (masterPartsInfo.suffixesByLength[length]) {
            htable_string_free(masterPartsInfo.suffixesByLength[length]);
        }
        if (masterPartsInfo.suffixesByNoHyphensLength[length]) {
            htable_string_free(masterPartsInfo.suffixesByNoHyphensLength[length]);
        }
    }
    free(masterPartsInfo.masterPartsNoHyphens);

    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        if (partsInfo.suffixesByLength[length]) {
            htable_sizelist_free(partsInfo.suffixesByLength[length]);
        }
    }
    free(partsInfo.parts);
}

void processor_clean() {
    htable_string_free(dictionary);
}

static void build_masterPartsInfo(const SourceData *data, MasterPartsInfo *mpInfo) {
    // Build masterParts
    size_t masterPartsCount = data->masterPartsCount;
    MasterPart *masterParts = (MasterPart *)data->masterParts;
    qsort(masterParts, masterPartsCount, sizeof(*masterParts), compare_mp_by_code_length_asc);

    // Build masterPartsNoHyphens
    MasterPart *masterPartsNoHyphens = malloc(masterPartsCount * sizeof(*masterPartsNoHyphens));
    CHECK_ALLOC(masterPartsNoHyphens);
    size_t masterPartsNoHyphensCount = 0;
    for (size_t i = 0; i < masterPartsCount; i++) {
        if (masterParts[i].codeNhLength >= MIN_STRING_LENGTH) {
            masterPartsNoHyphens[masterPartsNoHyphensCount++] = masterParts[i];
        }
    }
    qsort(masterPartsNoHyphens, masterPartsNoHyphensCount, sizeof(*masterPartsNoHyphens), compare_mp_by_codeNh_length_asc);

    // Populate MasterPartsInfo
    mpInfo->masterParts = masterParts;
    mpInfo->masterPartsCount = masterPartsCount;
    mpInfo->masterPartsNoHyphens = masterPartsNoHyphens;
    mpInfo->masterPartsNoHyphensCount = masterPartsNoHyphensCount;

    // Create and populate start indices.
    size_t startIndexByLength[MAX_STRING_LENGTH] = { 0 };
    size_t startIndexByLengthNoHyphens[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLength[length] = MAX_VALUE;
        startIndexByLengthNoHyphens[length] = MAX_VALUE;
    }
    for (size_t i = 0; i < masterPartsCount; i++) {
        size_t length = masterParts[i].codeLength;
        if (startIndexByLength[length] == MAX_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    for (size_t i = 0; i < masterPartsNoHyphensCount; i++) {
        size_t length = masterPartsNoHyphens[i].codeNhLength;
        if (startIndexByLengthNoHyphens[length] == MAX_VALUE) {
            startIndexByLengthNoHyphens[length] = i;
        }
    }
    backward_fill(startIndexByLength);
    backward_fill(startIndexByLengthNoHyphens);

    create_suffix_tables(mpInfo, startIndexByLength, create_suffix_table_for_mp_code);
    create_suffix_tables(mpInfo, startIndexByLengthNoHyphens, create_suffix_table_for_mp_codeNh);
}

static void build_partsInfo(const SourceData *data, PartsInfo *partsInfo) {
    // Build parts
    size_t partsCount = data->partsCount;
    Part *parts = malloc(sizeof(*parts) * partsCount);
    CHECK_ALLOC(parts);
    memcpy(parts, data->parts, partsCount * sizeof(*parts));
    qsort(parts, partsCount, sizeof(*parts), compare_part_by_code_length_asc);

    partsInfo->parts = parts;
    partsInfo->partsCount = partsCount;

    // Create and populate start indices.
    size_t startIndexByLength[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLength[length] = MAX_VALUE;
    }
    for (size_t i = 0; i < partsCount; i++) {
        size_t length = parts[i].codeLength;
        if (startIndexByLength[length] == MAX_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    backward_fill(startIndexByLength);

    create_suffix_tables(partsInfo, startIndexByLength, create_suffix_table_for_part_code);
}

static void create_suffix_tables(void *info, size_t *startIndexByLength, thread_func_t func) {
    thread_t threads[MAX_STRING_LENGTH] = { 0 };
    ThreadArgs threadArgs[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLength[length] != MAX_VALUE) {
            threadArgs[length].info = info;
            threadArgs[length].suffixLength = length;
            threadArgs[length].startIndex = startIndexByLength[length];
            int status = create_thread(&threads[length], func, &threadArgs[length]);
            CHECK_THREAD_CREATE_STATUS(status, length);
        }
    }
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (threads[length]) {
            int status = join_thread(threads[length], NULL);
            CHECK_THREAD_JOIN_STATUS(status, length);
        }
    }
}

static thread_ret_t create_suffix_table_for_mp_code(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    MasterPartsInfo *mpInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    MasterPart *masterParts = mpInfo->masterParts;
    size_t masterPartsCount = mpInfo->masterPartsCount;

    HTableString *table = htable_string_create(masterPartsCount - startIndex);
    for (size_t i = startIndex; i < masterPartsCount; i++) {
        MasterPart mp = masterParts[i];
        const char *suffix = mp.code + (mp.codeLength - suffixLength);
        htable_string_insert_if_not_exists(table, suffix, suffixLength, mp.codeOriginal);
    }
    mpInfo->suffixesByLength[suffixLength] = table;
    return 0;
}

static thread_ret_t create_suffix_table_for_mp_codeNh(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    MasterPartsInfo *mpInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    MasterPart *masterPartsNoHyphens = mpInfo->masterPartsNoHyphens;
    size_t masterPartsNoHyphensCount = mpInfo->masterPartsNoHyphensCount;

    HTableString *table = htable_string_create(masterPartsNoHyphensCount - startIndex);
    for (size_t i = startIndex; i < masterPartsNoHyphensCount; i++) {
        MasterPart mp = masterPartsNoHyphens[i];
        const char *suffix = mp.codeNh + (mp.codeNhLength - suffixLength);
        htable_string_insert_if_not_exists(table, suffix, suffixLength, mp.codeOriginal);
    }
    mpInfo->suffixesByNoHyphensLength[suffixLength] = table;
    return 0;
}

static thread_ret_t create_suffix_table_for_part_code(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    PartsInfo *partsInfo = args->info;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;

    Part *parts = partsInfo->parts;
    size_t partsCount = partsInfo->partsCount;

    HTableSizeList *table = htable_sizelist_create(partsCount - startIndex);
    for (size_t i = startIndex; i < partsCount; i++) {
        Part part = parts[i];
        const char *suffix = part.code + (part.codeLength - suffixLength);
        htable_sizelist_add(table, suffix, suffixLength, i);
    }
    partsInfo->suffixesByLength[suffixLength] = table;
    return 0;
}

static void backward_fill(size_t *array) {
    size_t tmp = array[MAX_STRING_LENGTH - 1];
    for (long length = (long)MAX_STRING_LENGTH - 1; length >= 0; length--) {
        if (array[length] == MAX_VALUE) {
            array[length] = tmp;
        }
        else {
            tmp = array[length];
        }
    }
}

static int compare_mp_by_code_length_asc(const void *a, const void *b) {
    const MasterPart *mpA = (const MasterPart *)a;
    const MasterPart *mpB = (const MasterPart *)b;
    return (mpA->codeLength < mpB->codeLength) ? -1
        : (mpA->codeLength > mpB->codeLength) ? 1
        : (mpA->index < mpB->index) ? -1 : (mpA->index > mpB->index) ? 1 : 0;
}

static int compare_mp_by_codeNh_length_asc(const void *a, const void *b) {
    const MasterPart *mpA = (const MasterPart *)a;
    const MasterPart *mpB = (const MasterPart *)b;
    return (mpA->codeNhLength < mpB->codeNhLength) ? -1
        : (mpA->codeNhLength > mpB->codeNhLength) ? 1
        : (mpA->index < mpB->index) ? -1 : (mpA->index > mpB->index) ? 1 : 0;
}

static int compare_part_by_code_length_asc(const void *a, const void *b) {
    const Part *pA = (const Part *)a;
    const Part *pB = (const Part *)b;
    return (pA->codeLength < pB->codeLength) ? -1
        : (pA->codeLength > pB->codeLength) ? 1
        : (pA->index < pB->index) ? -1 : (pA->index > pB->index) ? 1 : 0;
}

/*
* By applying simple qsort, the whole app is 50% faster, 310ms instead of 490ms.
* But, it's not a stable sort. It still finds the same number of matches though.
* However, in case of ties, it might return any of them.
* Requirements specify for ties to return the first masterParts in the file.
*/
//static int compare_mp_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const MasterPart *)a)->codeLength;
//    size_t lenB = ((const MasterPart *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
//
//static int compare_mp_by_codeNh_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const MasterPart *)a)->codeNhLength;
//    size_t lenB = ((const MasterPart *)b)->codeNhLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
//
//static int compare_part_by_code_length_asc(const void *a, const void *b) {
//    size_t lenA = ((const Part *)a)->codeLength;
//    size_t lenB = ((const Part *)b)->codeLength;
//    return lenA < lenB ? -1 : lenA > lenB ? 1 : 0;
//}
