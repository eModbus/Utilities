// Copyright (c) 2021 miq1 @ gmx . de

#ifndef _RINGBUF_H
#define _RINGBUF_H

#if defined(ESP8266)
#define USE_MUTEX 0
#elif defined(ESP32)
#define USE_MUTEX 1
#endif

#if USE_MUTEX
#include <mutex>
using std::mutex;
using std::lock_guard;
#define LOCK_GUARD(x,y) std::lock_guard<std::mutex> x(y);
#else
#define LOCK_GUARD(x, y)
#endif

#include <Arduino.h>

// RingBuf implements a uint8_t circular buffer of chosen size.
// For speed and usability reasons the internal buffer is twice the size of the requested!
// No exceptions thrown at all. Memory allocation failures will result in a 1-byte const
// buffer pointing to the static "nilBuf"!
class RingBuf {
public:
// Fallback static minimal buffer if memory allocation failed etc.
  static const uint8_t nilBuf[2];

// Constructor
// size: required size in bytes
// preserve: if true, no more bytes will be added to a full buffer unless older bytes are consumed
//           if false, buffer will be rotated until the newest byte is added
  explicit RingBuf(size_t size = 256, bool preserve = false);

  // Destructor: takes care of cleaning up the buffer
  ~RingBuf();

  // Copy constructor
  RingBuf(const RingBuf &r);

  // Move constructor
  RingBuf(RingBuf &&r);

  // Copy assignment
  RingBuf& operator=(const RingBuf &r);

  // Move assignment
  RingBuf& operator=(RingBuf &&r);

  // size: get number of bytes currently in buffer
  // WARNING! due to the nature of the rolling buffer, this size is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else data may be missed.
  size_t size();

  // data: get start address of the bytes in buffer
  // WARNING! due to the nature of the rolling buffer, this address is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else old data may be read.
  const uint8_t *data();

  // empty: returns true if no bytes are in the buffer
  bool empty();

  // valid: returns true if a buffer was allocated and is usable
  // Using the object in bool context returns the same information
  bool valid();
  operator bool();

  // capacity: return number of unused bytes in buffer
  // WARNING! due to the nature of the rolling buffer, this size is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else data may be missed.
  size_t capacity();

  // clear: empty the buffer
  bool clear();

  // pop: remove the leading numBytes bytes from the buffer
  size_t pop(size_t numBytes);

  // push_back: add a single byte or a buffer of bytes to the end of the buffer. 
  // If there is not enough room, the buffer will be rolled until the added bytes will fit.
  bool push_back(const uint8_t c);
  bool push_back(const uint8_t *data, size_t size);

  // Equality comparison: are sizes and contents of two buffers identical?
  bool operator==(RingBuf &r);

  // bufferAdr: return the start address of the underlying, double-sized data buffer.
  // bufferSize: return the real length of the underlying data buffer.
  // Note that this is only sensible in a debug context!
  inline const uint8_t *bufferAdr() { return buffer; }
  inline const size_t bufferSize() { return len; }

protected:
  uint8_t *buffer;           // The data buffer proper (twice the requested size)
  uint8_t *begin;            // Pointer to the first byte currently used
  uint8_t *end;              // Pointer behind the last byte currently used
  size_t len;                // Real length of buffer (twice the requested size)
  size_t usable;             // Requested length of the buffer
  bool preserve;             // Flag to hold or discard the oldest bytes if bytes are added
#if USE_MUTEX
  std::mutex m;              // Mutex to protect pop, clear and push_back operations
#endif
  void setFail();            // Internal function to set the object to nilBuf
};
#endif
