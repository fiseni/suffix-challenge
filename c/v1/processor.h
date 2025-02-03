#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "source_data.h"

size_t processor_find_mp_index(const char *partNumber, size_t partCodeLength);
void processor_initialize(const SourceData *data);
void processor_clean();

#endif
