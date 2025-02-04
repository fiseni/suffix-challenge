#include <string.h>
#include "allocator.h"
#include "common.h"
#include "source_data.h"
#include "processor.h"

static size_t run(const char *partsFile, const char *masterPartsFile, const char *resultsFile) {
    allocator_init();

    SourceData data = { 0 };
    source_data_load(&data, partsFile, masterPartsFile);
    processor_initialize(&data);

    // Two records per line. Each record is max 49 chars + CR + LC + separator
    char *resultsBlock = allocator_alloc((MAX_STRING_LENGTH * 2 + 3) * data.partsOriginalCount);
    size_t resultsBlockIndex = 0;
    size_t matchCount = 0;

    for (size_t i = 0; i < data.partsOriginalCount; i++) {
        const Part partOriginal = data.partsOriginal[i];
        size_t mpIndex = processor_find_mp_index(partOriginal.code, partOriginal.codeLength);

        memcpy(resultsBlock + resultsBlockIndex, partOriginal.code, partOriginal.codeLength);
        resultsBlockIndex += partOriginal.codeLength;
        resultsBlock[resultsBlockIndex++] = CHAR_SEMICOLON;

        if (mpIndex != MAX_SIZE_T_VALUE) {
            const Part mpOriginal = data.masterPartsOriginal[mpIndex];
            memcpy(resultsBlock + resultsBlockIndex, mpOriginal.code, mpOriginal.codeLength);
            resultsBlockIndex += mpOriginal.codeLength;
            matchCount++;
        }

        resultsBlock[resultsBlockIndex++] = '\n';
    };

    FILE *file = fopen(resultsFile, "w");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }
    fwrite(resultsBlock, 1, resultsBlockIndex, file);
    fclose(file);

    // We switched to allocator
    //free(resultsBlock);
    //processor_clean();
    //source_data_clean(&data);

    allocator_destroy();
    return matchCount;
}

int main(int argc, char *argv[]) {

#if _DEBUG
    run("../../data/parts.txt", "../../data/master-parts.txt", "results.txt");
    return 0;
#endif

    if (argc < 4) {
        printf("\nInvalid arguments!\n\n");
        printf("Usage: %s <parts file> <master parts file> <results file>\n\n", argv[0]);
        return 1;
    }

    size_t output = run(argv[1], argv[2], argv[3]);
    printf("%zu\n", output);
    return 0;
}
