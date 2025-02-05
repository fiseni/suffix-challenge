#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "source_data.h"

const StringView *processor_find_match(const StringView *partCode);
void processor_initialize(const SourceData *data);
void processor_clean();

#endif
