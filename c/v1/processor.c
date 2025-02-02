#include "common.h"
#include "thread_utils.h"
#include "hash_table.h"
#include "source_data.h"

typedef struct Context {
    SourceData *data;
    HTable *mpTable;
    HTable *mpSuffixesTables[MAX_STRING_LENGTH];
    HTable *mpNhSuffixesTables[MAX_STRING_LENGTH];
    HTable *partTables[MAX_STRING_LENGTH];
} Context;

typedef struct ThreadArgs {
    Context *ctx;
    size_t startIndex;
    size_t length;
} ThreadArgs;

static Context ctx = { 0 };

static void create_tables_in_parallel(Context *ctx, const Part *parts, size_t count, thread_func_t func, bool create_mp_table);
static thread_ret_t create_table_for_masterParts(thread_arg_t arg);
static thread_ret_t create_suffix_tables_for_masterParts(thread_arg_t arg);
static thread_ret_t create_suffix_tables_for_masterPartsNh(thread_arg_t arg);
static thread_ret_t create_tables_for_parts(thread_arg_t arg);

const char *processor_find_match(const char *partCode, size_t partCodeLength) {
    if (partCodeLength < MIN_STRING_LENGTH) {
        return NULL;
    }
    char buffer[MAX_STRING_LENGTH];
    str_to_upper(partCode, partCodeLength, buffer);

    size_t mpIndex;
    if (htable_search(ctx.mpSuffixesTables[partCodeLength], buffer, partCodeLength, &mpIndex))
        return ctx.data->masterPartsOriginal[mpIndex].code;

    if (htable_search(ctx.mpNhSuffixesTables[partCodeLength], buffer, partCodeLength, &mpIndex))
        return ctx.data->masterPartsOriginal[mpIndex].code;

    if (htable_search(ctx.partTables[partCodeLength], buffer, partCodeLength, &mpIndex))
        return ctx.data->masterPartsOriginal[mpIndex].code;

    return NULL;
}

void processor_initialize(const SourceData *data) {
    ctx.data = (SourceData *)data;
    create_tables_in_parallel(&ctx, ctx.data->masterPartsAsc, ctx.data->masterPartsAscCount, create_suffix_tables_for_masterParts, true);
    create_tables_in_parallel(&ctx, ctx.data->masterPartsNhAsc, ctx.data->masterPartsNhAscCount, create_suffix_tables_for_masterPartsNh, false);
    create_tables_in_parallel(&ctx, ctx.data->partsAsc, ctx.data->partsAscCount, create_tables_for_parts, false);
}

void processor_clean() {
    for (size_t length = 0; length < MAX_STRING_LENGTH; length++) {
        htable_free(ctx.mpSuffixesTables[length]);
        htable_free(ctx.mpNhSuffixesTables[length]);
        htable_free(ctx.partTables[length]);
    }
    htable_free(ctx.mpTable);
}

static thread_ret_t create_suffix_tables_for_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t length = args->length;
    const Part *masterPartsAsc = args->ctx->data->masterPartsAsc;
    size_t masterPartsAscCount = args->ctx->data->masterPartsAscCount;

    HTable *table = htable_create(masterPartsAscCount - startIndex);
    for (size_t i = startIndex; i < masterPartsAscCount; i++) {
        Part mp = masterPartsAsc[i];
        const char *suffix = mp.code + (mp.codeLength - length);
        htable_insert_if_not_exists(table, suffix, length, mp.index);
    }
    args->ctx->mpSuffixesTables[length] = table;
    return 0;
}

static thread_ret_t create_suffix_tables_for_masterPartsNh(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t length = args->length;
    const Part *masterPartsNhAsc = args->ctx->data->masterPartsNhAsc;
    size_t masterPartsNhAscCount = args->ctx->data->masterPartsNhAscCount;

    HTable *table = htable_create(masterPartsNhAscCount - startIndex);
    for (size_t i = startIndex; i < masterPartsNhAscCount; i++) {
        Part mpNh = masterPartsNhAsc[i];
        const char *suffix = mpNh.code + (mpNh.codeLength - length);
        htable_insert_if_not_exists(table, suffix, length, mpNh.index);
    }
    args->ctx->mpNhSuffixesTables[length] = table;
    return 0;
}

static thread_ret_t create_table_for_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const Part *masterPartsAsc = args->ctx->data->masterPartsAsc;
    size_t masterPartsAscCount = args->ctx->data->masterPartsAscCount;

    HTable *table = htable_create(masterPartsAscCount);
    for (size_t i = 0; i < masterPartsAscCount; i++) {
        Part mp = masterPartsAsc[i];
        htable_insert_if_not_exists(table, mp.code, mp.codeLength, mp.index);
    }
    args->ctx->mpTable = table;
    return 0;
}

static thread_ret_t create_tables_for_parts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t length = args->length;
    HTable *mpTable = args->ctx->mpTable;
    const Part *partsAsc = args->ctx->data->partsAsc;
    size_t partsAscCount = args->ctx->data->partsAscCount;

    HTable *table = htable_create(partsAscCount - startIndex);
    for (size_t i = startIndex; i < partsAscCount; i++) {
        Part part = partsAsc[i];
        if (part.codeLength > length) break;

        for (size_t suffixLength = part.codeLength - 1; suffixLength >= MIN_STRING_LENGTH; suffixLength--) {
            const char *suffix = part.code + (part.codeLength - suffixLength);
            size_t mpIndex;
            if (htable_search(mpTable, suffix, suffixLength, &mpIndex)) {
                htable_insert_if_not_exists(table, part.code, part.codeLength, mpIndex);
                break;
            }
        }
    }
    args->ctx->partTables[length] = table;
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

static void create_tables_in_parallel(Context *ctx, const Part *parts, size_t count, thread_func_t func, bool create_mp_table) {
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

    // We will sneak in and use one thread to create the table for master parts.
    if (create_mp_table) {
        int status = create_thread(&threads[0], create_table_for_masterParts, &(ThreadArgs) {.ctx = ctx});
        CHECK_THREAD_CREATE_STATUS(status, (size_t)0);
    }

    for (size_t length = MIN_STRING_LENGTH; length < MAX_STRING_LENGTH; length++) {
        if (startIndexByLength[length] != MAX_SIZE_T_VALUE) {
            threadArgs[length].ctx = ctx;
            threadArgs[length].length = length;
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

    if (create_mp_table) {
        int status = join_thread(threads[0], NULL);
        CHECK_THREAD_JOIN_STATUS(status, (size_t)0);
    }
}
