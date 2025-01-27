#include "common.h"
#include "thread_utils.h"
#include "hash_table.h"
#include "source_data.h"

typedef struct Context {
    SourceData *data;
    HTableSizeT *dictionary;
    HTableSizeT *mpSuffixesByLength[MAX_STRING_LENGTH];
    HTableSizeT *mpNhSuffixesByLength[MAX_STRING_LENGTH];
    HTableSizeTList *partSuffixesByLength[MAX_STRING_LENGTH];
} Context;

typedef struct ThreadArgs {
    Context *ctx;
    size_t startIndex;
    size_t suffixLength;
} ThreadArgs;

static Context ctx = { 0 };

static void create_suffix_tables(Context *ctx, const Part *parts, size_t count, thread_func_t func);
static thread_ret_t create_suffix_table_for_masterParts(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_masterPartsNh(thread_arg_t arg);
static thread_ret_t create_suffix_table_for_parts(thread_arg_t arg);

const char *processor_find_match(const char *partCode, size_t partCodeLength) {
    if (partCodeLength < MIN_STRING_LENGTH) {
        return NULL;
    }
    char buffer[MAX_STRING_LENGTH];
    str_to_upper(partCode, partCodeLength, buffer);

    size_t mpIndex;
    if (htable_sizet_search(ctx.dictionary, buffer, partCodeLength, &mpIndex)) {
        return ctx.data->masterPartsOriginal[mpIndex].code;
    }

    return NULL;
}

void processor_initialize(const SourceData *data) {
    ctx.data = (SourceData *)data;
    ctx.dictionary = htable_sizet_create(data->partsAscCount);

    create_suffix_tables(&ctx, ctx.data->masterPartsAsc, ctx.data->masterPartsAscCount, create_suffix_table_for_masterParts);
    create_suffix_tables(&ctx, ctx.data->masterPartsAscNh, ctx.data->masterPartsAscNhCount, create_suffix_table_for_masterPartsNh);
    create_suffix_tables(&ctx, ctx.data->partsAsc, ctx.data->partsAscCount, create_suffix_table_for_parts);

    for (size_t i = 0; i < data->partsAscCount; i++) {
        Part part = data->partsAsc[i];
        size_t mpIndex;

        if (htable_sizet_search(ctx.mpSuffixesByLength[part.codeLength], part.code, part.codeLength, &mpIndex)) {
            htable_sizet_insert_if_not_exists(ctx.dictionary, part.code, part.codeLength, mpIndex);
            continue;
        }
        if (htable_sizet_search(ctx.mpNhSuffixesByLength[part.codeLength], part.code, part.codeLength, &mpIndex)) {
            htable_sizet_insert_if_not_exists(ctx.dictionary, part.code, part.codeLength, mpIndex);
        }
    }

    for (long i = (long)data->masterPartsAscCount - 1; i >= 0; i--) {
        Part masterPart = data->masterPartsAsc[i];

        HTableSizeTList *partsBySuffix = ctx.partSuffixesByLength[masterPart.codeLength];
        const ListItem *originalPartIndices = htable_sizetlist_search(partsBySuffix, masterPart.code, masterPart.codeLength);
        while (originalPartIndices) {
            size_t originalPartIndex = originalPartIndices->value;
            Part part = data->partsAsc[originalPartIndex];
            htable_sizet_insert_if_not_exists(ctx.dictionary, part.code, part.codeLength, masterPart.index);
            originalPartIndices = originalPartIndices->next;
        }
    }
}

void processor_clean() {
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        htable_sizet_free(ctx.mpSuffixesByLength[length]);
        htable_sizet_free(ctx.mpNhSuffixesByLength[length]);
        htable_sizetlist_free(ctx.partSuffixesByLength[length]);
    }
    htable_sizet_free(ctx.dictionary);
}

static thread_ret_t create_suffix_table_for_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;
    const Part *masterPartsAsc = args->ctx->data->masterPartsAsc;
    size_t masterPartsAscCount = args->ctx->data->masterPartsAscCount;

    HTableSizeT *table = htable_sizet_create(masterPartsAscCount - startIndex);
    for (size_t i = startIndex; i < masterPartsAscCount; i++) {
        Part mp = masterPartsAsc[i];
        const char *suffix = mp.code + (mp.codeLength - suffixLength);
        htable_sizet_insert_if_not_exists(table, suffix, suffixLength, mp.index);
    }
    args->ctx->mpSuffixesByLength[suffixLength] = table;
    return 0;
}

static thread_ret_t create_suffix_table_for_masterPartsNh(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;
    const Part *masterPartsAscNh = args->ctx->data->masterPartsAscNh;
    size_t masterPartsAscNhCount = args->ctx->data->masterPartsAscNhCount;

    HTableSizeT *table = htable_sizet_create(masterPartsAscNhCount - startIndex);
    for (size_t i = startIndex; i < masterPartsAscNhCount; i++) {
        Part mpNh = masterPartsAscNh[i];
        const char *suffix = mpNh.code + (mpNh.codeLength - suffixLength);
        htable_sizet_insert_if_not_exists(table, suffix, suffixLength, mpNh.index);
    }
    args->ctx->mpNhSuffixesByLength[suffixLength] = table;
    return 0;
}

static thread_ret_t create_suffix_table_for_parts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t suffixLength = args->suffixLength;
    const Part *partsAsc = args->ctx->data->partsAsc;
    size_t partsAscCount = args->ctx->data->partsAscCount;

    HTableSizeTList *table = htable_sizetlist_create(partsAscCount - startIndex);
    for (size_t i = startIndex; i < partsAscCount; i++) {
        Part part = partsAsc[i];
        const char *suffix = part.code + (part.codeLength - suffixLength);
        htable_sizetlist_insert(table, suffix, suffixLength, i);
    }
    args->ctx->partSuffixesByLength[suffixLength] = table;
    return 0;
}

static inline void backward_fill(size_t *array) {
    size_t tmp = array[MAX_STRING_LENGTH - 1];
    for (long length = (long)MAX_STRING_LENGTH - 1; length >= 0; length--) {
        if (array[length] == MAX_SIZE_T_VALUE) {
            array[length] = tmp;
        }
        else {
            tmp = array[length];
        }
    }
}

static void create_suffix_tables(Context *ctx, const Part *parts, size_t count, thread_func_t func) {
    size_t startIndexByLength[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        startIndexByLength[length] = MAX_SIZE_T_VALUE;
    }
    for (size_t i = 0; i < count; i++) {
        size_t length = parts[i].codeLength;
        if (startIndexByLength[length] == MAX_SIZE_T_VALUE) {
            startIndexByLength[length] = i;
        }
    }
    backward_fill(startIndexByLength);

    thread_t threads[MAX_STRING_LENGTH] = { 0 };
    ThreadArgs threadArgs[MAX_STRING_LENGTH] = { 0 };
    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLength[length] != MAX_SIZE_T_VALUE) {
            threadArgs[length].ctx = ctx;
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
