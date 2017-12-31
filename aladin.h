#pragma once

#include <stdint.h>
#include "utils.h"

#define LOGBOOK_LENGTH 12
#define LOGBOOK_RINGBUFFER_LENGTH (0x7bc-0x600)
#define LOGBOOK_RINGBUFFER_START (0x600)
#define LOGBOOK_RINGBUFFER_END (0x7bc) // not included

#define DEPTHPROFILE_RINGBUFFER_LENGTH (0x600)
#define DEPTHPROFILE_RINGBUFFER_START (0x000)
#define DEPTHPROFILE_RINGBUFFER_END (0x600) // not included

#define ALADIN_TIMEOUT 10000
#define ALADIN_MAX_RETRIES 10

enum Errors {
  NoError = 0,
  DiveCountNullError = -1,
  DataError = -2,
  SendError = -3
};

uint16_t divelog_totalDiveCount(uint8_t const* data);
uint8_t divelog_storedDiveCount(uint8_t const* data);
uint16_t divelog_logbookStartIndex(uint8_t const* data, uint8_t diveIndex);
uint16_t divelog_depthProfileEndRingBufferIndex(uint8_t const* data);
uint16_t divelog_findDepthProfileStartIndex(uint8_t const* data, uint16_t* psize, uint8_t diveIndex);
bool isLogbookIndexValid(uint16_t index);
bool isDepthProfileIndexValid(uint16_t index);

Errors sendAladin(uint8_t const* data);
void aladin_print(const char* data);
void aladin_print(uint8_t v);

