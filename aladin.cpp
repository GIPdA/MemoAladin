#include "aladin.h"
#include <Arduino.h>

#define BUFFER_SIZE 4000

// Write byte to PC
static void write(uint8_t value)
{
  Serial.write(reverse(value));
}
// Write data buffer to PC
static void write(uint8_t const* data, uint16_t size)
{
  while (size--)
    write(*data++);
}
static uint8_t read()
{
  return reverse(Serial.read());
}
static bool available()
{
  return Serial.available();
}

void aladin_print(uint8_t v)
{
  write(v);
}

// Print a bare string (null-terminated) to PC (string length <= 123)
void aladin_print(const char* data)
{
  uint8_t dataSize {0}; // data size
  uint8_t checksum {0};

  // compute data checksum and size
  {
    char const* ptr {data};
    while (*ptr) {
      checksum ^= *ptr++;
      ++dataSize;
    }
  }

  if (!dataSize)
    return;

  checksum ^= dataSize;

  write(dataSize+3); // outer packet size (max 128)
  write(dataSize);   // inner packet size
  write(0);
  write((uint8_t const*)data, dataSize); // data
  write(checksum); // inner packer checksum

  checksum ^= (checksum);
  checksum ^= (dataSize+3);

  write(checksum); // outer packet checksum
}

// Send an "inner" packet to PC (by chunks of 128 bytes)
static bool sendBuffer(const uint8_t* data, uint16_t size)
{
  uint8_t retries {0};

  uint16_t bufferLength {size};
  uint8_t const* dataPtr {data};

  while (bufferLength > 0) {
    uint16_t const sendSize { min(bufferLength, 126) };
    uint8_t checksum {0};

    write(sendSize); // outer packet size
    checksum ^= sendSize; // packet size checksum
    
    // send data, compute checksum
    {
      uint8_t const* ptr {dataPtr};
      for (uint16_t i {0}; i < sendSize; ++i) {
        checksum ^= *ptr;
        write(*ptr++);
      }
    }
    write(checksum); // send outer packet checksum

    uint8_t c {0};
    do {
      uint32_t const timeout { millis() };
      while (!available()) {
        if ((millis()-timeout) > ALADIN_TIMEOUT)
          return false;
        delay(10);
      }

      c = read();
    } while (c == 0x00 || c == 0xff);

    if (c == 0x06) {
      // Packet OK, send next
      dataPtr += sendSize;
      bufferLength -= sendSize;
      retries = 0;
    } else {
      // Packet NAK, retry
      if (retries++ >= ALADIN_MAX_RETRIES)
        return false;
    }
  }

  return true;
}


uint16_t divelog_totalDiveCount(uint8_t const* data)
{
  return (uint16_t(data[0x7f2]) << 8) + data[0x7f3];
}

uint8_t divelog_storedDiveCount(uint8_t const* data)
{
  return data[0x7f5];
}

uint16_t divelog_logbookStartIndex(uint8_t const* data, uint8_t diveIndex)
{
  return (((
    ((data[0x7f4] + 36) % 37) * 12) // Newest logbook index
    - LOGBOOK_LENGTH*diveIndex) % LOGBOOK_RINGBUFFER_LENGTH) // To oldest logs
    + 0x600; // buffer offset (start of logbook ringbuffer)
}

uint16_t divelog_depthProfileEndRingBufferIndex(uint8_t const* data)
{
  return ((data[0x7f6] + (data[0x7f7] >> 1) * 256)) & 0x7ff;
}


// diveIndex: wanted start of profile index from newest (0) to oldest (diveCount-1)
// psize: stores dive profile length if found
// returns: 0xFFFF if not found, index of start of profile data
uint16_t divelog_findDepthProfileStartIndex(uint8_t const* data, uint16_t* psize, uint8_t diveIndex)
{
  // Look for 0xff before this address
  uint16_t const startIndex = divelog_depthProfileEndRingBufferIndex(data);
  uint16_t index = startIndex;
  uint16_t size = 0;

  do {
    size = 0;
    do {
      if (index == 0)
        index = 0x600;
      
      --index;
      ++size;
    } while (data[index] != 0xff && index != startIndex);
  } while (diveIndex-- > 0);

  if (index == startIndex)
    return 0xFFFF;

  if (psize)
    *psize = size;
  
  return (int16_t)index; // will never reach int16 limit
}

bool isLogbookIndexValid(uint16_t index)
{
  return index >= LOGBOOK_RINGBUFFER_START && index < LOGBOOK_RINGBUFFER_END;
}

bool isDepthProfileIndexValid(uint16_t index)
{
  return index >= DEPTHPROFILE_RINGBUFFER_START && index < DEPTHPROFILE_RINGBUFFER_END;
}

// logIndex: log book start index in data, must be a valid logbook ringbuffer index
// returns length written
static uint16_t writeLogBook(uint8_t* buffer, uint8_t const* data, uint16_t logIndex)
{
  for (uint8_t i {0}; i < 12; ++i) {
    uint16_t index = logIndex+i;
    if (i >= 7 && i <= 10) {
      // timestamp is reversed (4 bytes)
      index = logIndex + (10 - (i-7));
    }

    if (index >= 0x7bc)
      index -= LOGBOOK_RINGBUFFER_LENGTH;

    buffer[i] = data[index % 0x7bc];
  }
  return LOGBOOK_LENGTH;
}

static uint16_t writeDepthProfile(uint8_t* buffer, uint8_t const* data, uint16_t logIndex, uint16_t size)
{
  for (uint16_t i {0}, pi {logIndex+1}; i < size; ++i, ++pi) {
    if (pi >= 0x600)
      pi = 0;
    buffer[i] = data[pi];
  }
  return size;
}

static uint16_t writeAladinSerialCode(uint8_t* buffer, uint8_t const* data)
{
  // serial
  *buffer++ = data[0x7ed];
  *buffer++ = data[0x7ee];
  *buffer++ = data[0x7ef];
  // aladin code
  *buffer++ = data[0x7bc];
  return 4;
}


static uint8_t computeChecksum(uint8_t const* data, uint16_t size)
{
  uint8_t checksum {0};
  while (size--) {
    checksum ^= *data++;
  }
  return checksum;
}


Errors sendAladin(uint8_t const* data)
{
  static uint8_t buffer[BUFFER_SIZE] = {0};

  uint8_t* bufferPtr { buffer };

  bufferPtr += 2; // inner packet size

  // -- header --
  *bufferPtr++ = 'U';
  // time
  *bufferPtr++ = data[0x7fb];
  *bufferPtr++ = data[0x7fa];
  *bufferPtr++ = data[0x7f9];
  *bufferPtr++ = data[0x7f8];

  //uint16_t const totalDiveCount { divelog_totalDiveCount(data) };
  uint8_t const diveCount { divelog_storedDiveCount(data) };

  if (diveCount == 0)
    return DiveCountNullError;

  // -- diving record --
  for (uint8_t currentDiveIndex {0}; currentDiveIndex < diveCount; ++currentDiveIndex) {
    // Write Aladin serial and type code
    bufferPtr += writeAladinSerialCode(bufferPtr, data);
    
    // Write logbook
    uint16_t const logbookIndex { divelog_logbookStartIndex(data, currentDiveIndex) };
    if (!isLogbookIndexValid(logbookIndex))
      return DataError;

    bufferPtr += writeLogBook(bufferPtr, data, logbookIndex);

    // Write depth profile
    uint16_t diveProfileLength {0};
    uint16_t const diveProfileIndex { divelog_findDepthProfileStartIndex(data, &diveProfileLength, currentDiveIndex) };

    if (!isDepthProfileIndexValid(diveProfileIndex))
      return DataError;

    if (diveProfileLength > 0)
      --diveProfileLength; // Ignore header byte (0xFF)

    // Write dive profile length
    *bufferPtr++ = diveProfileLength;
    *bufferPtr++ = (diveProfileLength>>8);

    // Write dive profile data if any
    if (diveProfileLength > 0)
      bufferPtr += writeDepthProfile(bufferPtr, data, diveProfileIndex, diveProfileLength);
  }

  // Prep packet: data size (without packet size) + checksum
  uint16_t const bufferSize { bufferPtr - buffer - 2 };

  // Add packet size
  buffer[0] = bufferSize;
  buffer[1] = bufferSize>>8;
  
  // Add checksum (includes packet size)
  *bufferPtr = computeChecksum(buffer, bufferSize+2);

  // Send data (+ packet size + checksum)
  if (!sendBuffer(buffer, bufferSize+3))
    return SendError;

  return NoError;
}

