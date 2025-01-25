#include <stdio.h>
#include "cross_platform_time.h"
#include "source_data.h"
#include "utils.h"
#include "processor.h"

static size_t run(const char *partsFile, const char *masterPartsFile, const char* resultsFile) {
    const SourceData *data = source_data_read(masterPartsFile, partsFile);
    if (data == NULL) {
        return 0;
    }

    FILE *file = fopen(resultsFile, "w");
    if (!file) {
        perror("Failed to open file");
        return 0;
    }

    processor_initialize(data);
    size_t matchCount = 0;
    for (size_t i = 0; i < data->partsCount; i++) {
        char *partCode = (char*)data->parts[i].code;
        size_t length;
        str_trim_in_place(partCode, &length);
        const char *match = processor_find_match(partCode);
        if (match) {
            matchCount++;
            fprintf(file, "%s;%s\n", partCode, match);
        }
        else {
            fprintf(file, "%s;\n", partCode);
        }
    };

    fclose(file);
    processor_clean();
    source_data_clean(data);
    return matchCount;
}

int main(int argc, char *argv[]) {

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
