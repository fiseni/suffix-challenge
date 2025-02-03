#include "source_data.h"
#include "common.h"
#include "processor.h"

static size_t run(const char *partsFile, const char *masterPartsFile, const char *resultsFile) {
    SourceData data = { 0 };
    source_data_load(&data, partsFile, masterPartsFile);
    processor_initialize(&data);

    FILE *file = fopen(resultsFile, "w");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    size_t matchCount = 0;
    for (size_t i = 0; i < data.partsOriginalCount; i++) {
        const Part partOriginal = data.partsOriginal[i];
        const char *match = processor_find_match(partOriginal.code, partOriginal.codeLength);

        if (match) {
            matchCount++;
            fprintf(file, "%s;%s\n", partOriginal.code, match);
        }
        else {
            fprintf(file, "%s;\n", partOriginal.code);
        }
    };

    fclose(file);
    //processor_clean();
    //source_data_clean(data);
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
