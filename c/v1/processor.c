#include "common.h"
#include "thread_utils.h"
#include "hash_table.h"
#include "source_data.h"

typedef struct Context {
    const SourceData *data;
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

const StringView *processor_find_match(const StringView *partCode) {
    if (partCode->length < MIN_STRING_LENGTH) {
        return NULL;
    }

    char buffer[MAX_STRING_LENGTH];
    StringView partCodeUpper = sv_to_upper(partCode, buffer);

    const StringView *match = htable_search(ctx.mpSuffixesTables[partCode->length], &partCodeUpper);
    if (!match) match = htable_search(ctx.mpNhSuffixesTables[partCode->length], &partCodeUpper);
    if (!match) match = htable_search(ctx.partTables[partCode->length], &partCodeUpper);

    return match;
}

void processor_initialize(const SourceData *data) {
    ctx.data = (SourceData *)data;
    create_tables_in_parallel(&ctx, ctx.data->mpAsc, ctx.data->mpAscCount, create_suffix_tables_for_masterParts, true);
    create_tables_in_parallel(&ctx, ctx.data->mpNhAsc, ctx.data->mpNhAscCount, create_suffix_tables_for_masterPartsNh, false);
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
    const Part *mpAsc = args->ctx->data->mpAsc;
    size_t mpAscCount = args->ctx->data->mpAscCount;

    HTable *table = htable_create(mpAscCount - startIndex);
    for (size_t i = startIndex; i < mpAscCount; i++) {
        Part mp = mpAsc[i];
        StringView suffix = sv_slice_suffix(&mp.code, length);
        htable_insert_if_not_exists(table, &suffix, mp.codeOriginal);
    }
    args->ctx->mpSuffixesTables[length] = table;
    return 0;
}

static thread_ret_t create_suffix_tables_for_masterPartsNh(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    size_t startIndex = args->startIndex;
    size_t length = args->length;
    const Part *mpNhAsc = args->ctx->data->mpNhAsc;
    size_t mpNhAscCount = args->ctx->data->mpNhAscCount;

    HTable *table = htable_create(mpNhAscCount - startIndex);
    for (size_t i = startIndex; i < mpNhAscCount; i++) {
        Part mpNh = mpNhAsc[i];
        StringView suffix = sv_slice_suffix(&mpNh.code, length);
        htable_insert_if_not_exists(table, &suffix, mpNh.codeOriginal);
    }
    args->ctx->mpNhSuffixesTables[length] = table;
    return 0;
}

static thread_ret_t create_table_for_masterParts(thread_arg_t arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    const Part *mpAsc = args->ctx->data->mpAsc;
    size_t mpAscCount = args->ctx->data->mpAscCount;

    HTable *table = htable_create(mpAscCount);
    for (size_t i = 0; i < mpAscCount; i++) {
        Part mp = mpAsc[i];
        htable_insert_if_not_exists(table, &mp.code, mp.codeOriginal);
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
        if (part.code.length > length) break;

        for (size_t suffixLength = part.code.length - 1; suffixLength >= MIN_STRING_LENGTH; suffixLength--) {
            StringView suffix = sv_slice_suffix(&part.code, suffixLength);
            const StringView *match = htable_search(mpTable, &suffix);
            if (match) {
                htable_insert_if_not_exists(table, &part.code, match);
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
        size_t length = parts[i].code.length;
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
