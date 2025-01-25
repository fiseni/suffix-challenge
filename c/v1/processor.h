#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "source_data.h"

const char *processor_get_identifier();
const char *processor_find_match(const char *partNumber);
void processor_initialize(const SourceData *data);
void processor_clean();
#endif
