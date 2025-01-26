#include "cross_platform_time.h"
#include "source_data.h"
#include "common.h"
#include "processor.h"

static size_t run(const char *partsFile, const char *masterPartsFile, const char *resultsFile) {
    const SourceData *data = source_data_read(partsFile, masterPartsFile);

    FILE *file = fopen(resultsFile, "w");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    processor_initialize(data);
    size_t matchCount = 0;

    for (size_t i = 0; i < data->partsOriginalCount; i++) {
        const Part part = data->partsOriginal[i];
        const char *match = processor_find_match(part.code, part.codeLength);

        if (match) {
            matchCount++;
            fprintf(file, "%s;%s\n", part.code, match);
        }
        else {
            fprintf(file, "%s;\n", part.code);
        }
    };

    fclose(file);
    processor_clean();
    source_data_clean(data);
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

    double start = time_get_seconds();
    size_t output = run(argv[1], argv[2], argv[3]);
    printf("Completed in: \t\t%f seconds.\n", time_get_seconds() - start);
    printf("Output: %zu\n", output);

    return 0;
}
