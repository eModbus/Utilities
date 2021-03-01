// Copyright (c) 2021 miq1 @ gmx . de

#ifndef _RINGBUF_H
#define _RINGBUF_H

#if defined(ESP8266)
#define USE_MUTEX 0
#elif defined(ESP32)
#define USE_MUTEX 1
#endif

#if USE_MUTEX
#include <mutex>   // NOLINT
using std::mutex;
using std::lock_guard;
#define LOCK_GUARD(x,y) std::lock_guard<std::mutex> x(y);
#else
#define LOCK_GUARD(x, y)
#endif

#include <Arduino.h>

// RingBuf implements a circular buffer of chosen size.
// For speed and usability reasons the internal buffer is twice the size of the requested!
// No exceptions thrown at all. Memory allocation failures will result in a const
// buffer pointing to the static "nilBuf"!
// template <typename T>
template <typename T>
class RingBuf {
public:
// Fallback static minimal buffer if memory allocation failed etc.
  static const T nilBuf[2];

// Constructor
// size: required size in T elements
// preserve: if true, no more elements will be added to a full buffer unless older elements are consumed
//           if false, buffer will be rotated until the newest element is added
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

  // size: get number of elements currently in buffer
  // WARNING! due to the nature of the rolling buffer, this size is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else data may be missed.
  size_t size();

  // data: get start address of the elements in buffer
  // WARNING! due to the nature of the rolling buffer, this address is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else old data may be read.
  const T *data();

  // empty: returns true if no elements are in the buffer
  bool empty();

  // valid: returns true if a buffer was allocated and is usable
  // Using the object in bool context returns the same information
  bool valid();
  operator bool();

  // capacity: return number of unused elements in buffer
  // WARNING! due to the nature of the rolling buffer, this size is VOLATILE and needs to be 
  //          read again every time the buffer is used! Else data may be missed.
  size_t capacity();

  // clear: empty the buffer
  bool clear();

  // pop: remove the leading numElements elements from the buffer
  size_t pop(size_t numElements);

  // operator[]: return the element the index is pointing to. If index is
  // outside the currently used area, return 0
  const T operator[](size_t index);

  // safeCopy: get a stable data copy from currently used buffer
  // target: buffer to copy data into
  // len: number of elements requested
  // move: if true, copied elements will be pop()-ped
  // returns number of elements actually transferred
  size_t safeCopy(T *target, size_t len, bool move = false);

  // push_back: add a single element or a buffer of elements to the end of the buffer. 
  // If there is not enough room, the buffer will be rolled until the added elements will fit.
  bool push_back(const T c);
  bool push_back(const T *data, size_t size);

  // Equality comparison: are sizes and contents of two buffers identical?
  bool operator==(RingBuf &r);

  // bufferAdr: return the start address of the underlying, double-sized data buffer.
  // bufferSize: return the real length of the underlying data buffer.
  // Note that this is only sensible in a debug context!
  inline const uint8_t *bufferAdr() { return (uint8_t *)buffer; }
  inline const size_t bufferSize() { return len * elementSize; }

protected:
  T *buffer;           // The data buffer proper (twice the requested size)
  T *begin;            // Pointer to the first element currently used
  T *end;              // Pointer behind the last element currently used
  size_t len;                // Real length of buffer (twice the requested size)
  size_t usable;             // Requested length of the buffer
  bool preserve;             // Flag to hold or discard the oldest elements if elements are added
  size_t elementSize;        // Size of a single buffer element
#if USE_MUTEX
  std::mutex m;              // Mutex to protect pop, clear and push_back operations
#endif
  void setFail();            // Internal function to set the object to nilBuf
};

template <typename T>
const T  RingBuf<T>::nilBuf[2] = { 0, 0 };

// setFail: in case of memory allocation problems, use static nilBuf 
template <typename T>
void RingBuf<T>::setFail() {
  buffer = (T *)RingBuf<T>::nilBuf;
  len = 2 * elementSize;
  usable = 0;
  begin = end = buffer;
}

// valid: return if buffer is a real one
template <typename T>
bool RingBuf<T>::valid() {
  return (buffer && (buffer != RingBuf<T>::nilBuf));
}

// operator bool: same as valid()
template <typename T>
RingBuf<T>::operator bool() {
  return valid();
}

// Constructor: allocate a buffer twice the requested size
template <typename T>
RingBuf<T>::RingBuf(size_t size, bool p) :
  len(size * 2),
  usable(size),
  preserve(p),
  elementSize(sizeof(T)) {
  // Allocate memory
  buffer = new T[len];
  // Failed?
  if (!buffer) setFail();
  else clear();
}

// Destructor: free allocated memory, if any
template <typename T>
RingBuf<T>::~RingBuf() {
  // Do we have a valid buffer?
  if (valid()) {
    // Yes, free it
    delete buffer;
  }
}

// Copy constructor: take over everything
template <typename T>
RingBuf<T>::RingBuf(const RingBuf &r) {
  // Is the assigned RingBuf valid?
  if (r.buffer && (r.buffer != RingBuf<T>::nilBuf)) {
    // Yes. Try to allocate a copy
    buffer = new T[r.len];
    // Succeeded?
    if (buffer) {
      // Yes. copy over data
      len = r.len;
      memcpy(buffer, r.buffer, len * r.elementSize);
      begin = buffer + (r.begin - r.buffer);
      end = buffer + (r.end - r.buffer);
      preserve = r.preserve;
      usable = r.usable;
      elementSize = r.elementSize;
    } else {
      setFail();
    }
  } else {
    setFail();
  }
}

// Move constructor
template <typename T>
RingBuf<T>::RingBuf(RingBuf &&r) {
  // Is the assigned RingBuf valid?
  if (r.buffer && (r.buffer != RingBuf<T>::nilBuf)) {
    // Yes. Take over the data
    buffer = r.buffer;
    len = r.len;
    begin = buffer + (r.begin - r.buffer);
    end = buffer + (r.end - r.buffer);
    preserve = r.preserve;
    usable = r.usable;
    elementSize = r.elementSize;
    r.buffer = nullptr;
  } else {
    setFail();
  }
}

// Assignment
template <typename T>
RingBuf<T>& RingBuf<T>::operator=(const RingBuf<T> &r) {
  if (valid()) {
    // Is the source a real RingBuf?
    if (r.buffer && (r.buffer != RingBuf<T>::nilBuf)) {
      // Yes. Copy over the data
      clear();
      push_back(r.begin, r.end - r.begin);
    }
  }
  return *this;
}

// Move assignment
template <typename T>
RingBuf<T>& RingBuf<T>::operator=(RingBuf<T> &&r) {
  if (valid()) {
    // Is the source a real RingBuf?
    if (r.buffer && (r.buffer != RingBuf<T>::nilBuf)) {
      // Yes. Copy over the data
      clear();
      push_back(r.begin, r.end - r.begin);
      r.buffer = nullptr;
    }
  }
  return *this;
}

// size: number of elements used in the buffer
template <typename T>
size_t RingBuf<T>::size() {
  return end - begin;
}

// data: get start of used data area
template <typename T>
const T *RingBuf<T>::data() {
  return begin;
}

// empty: is any data in buffer?
template <typename T>
bool RingBuf<T>::empty() {
  return ((size() == 0) || !valid());
}

// capacity: return remaining usable size
template <typename T>
size_t RingBuf<T>::capacity() {
  if (!valid()) return 0;
  return usable - size();
}

// clear: forget about contents
template <typename T>
bool RingBuf<T>::clear() {
  if (!valid()) return false;
  LOCK_GUARD(cLock, m);
  begin = end = buffer;
  return true;
}

// pop: remove elements from the beginning of the buffer
template <typename T>
size_t RingBuf<T>::pop(size_t numElements) {
  if (!valid()) return 0;
  // Do we have data in the buffer?
  if (size()) {
    // Yes. Is the requested number of elements larger than the used buffer?
    if (numElements >= size()) {
      // Yes. clear the buffer
      size_t n = size();
      clear();
      return n;
    } else {
      LOCK_GUARD(cLock, m);
      // No, the buffer needs to be emptied partly only
      begin += numElements;
      // Is begin now pointing into the upper half?
      if (begin >= (buffer + usable)) {
        // Yes. Move begin and end down again
        begin -= usable;
        end -= usable;
      }
    }
    return numElements;
  }
  return 0;
}

// push_back(single element): add one element to the buffer, potentially discarding previous ones
template <typename T>
bool RingBuf<T>::push_back(const T c) {
  if (!valid()) return false;
  {
    LOCK_GUARD(cLock, m);
    // No more space?
    if (capacity() == 0) {
      // No, we need to drop something
      // Are we to keep the oldest data?
      if (preserve) {
        // Yes. The new element will be dropped to leave the buffer untouched
        return false;
      }
      // We need to drop the oldest element begin is pointing to
      begin++;
      // Overflow?
      if (begin >= (buffer + usable)) {
        begin = buffer;
        end = begin + usable - 1;
      }
    }
    // Now add the element
    *end = c;
    // Is end pointing to the second half?
    if (end >= (buffer + usable)) {
      // Yes. Add the element to the lower half as well
      *(end - usable) = c;
    } else {
      // No, we need to add it to the upper half
      *(end + usable) = c;
    }
    end++;
  }
  return true;
}

// push_back(element buffer): add a batch of elements to the buffer
template <typename T>
bool RingBuf<T>::push_back(const T *data, size_t size) {
  if (!valid()) return false;
  // Do not process nullptr or zero lengths
  if (!data || size == 0) return false;
  // Avoid self-referencing pushes
  if (data >= buffer && data <= (buffer + len)) return false;
  {
    LOCK_GUARD(cLock, m);
    // Is the size to be added fitting the capacity?
    if (size > capacity()) {
      // No. We need to make room first
      // Are we allowed to do that?
      if (preserve) {
        // No. deny the push_back
        return false;
      }
      // Adjust data to the maximum usable size
      if (size > usable) {
        data += (size - usable);
        size = usable;
      }
      // Make room for the data
      begin += size - capacity();
      if (begin >= (buffer + usable)) {
        begin -= usable;
        end -= usable;
      }
    }
    // Yes. copy it in
    memcpy(end, data, size * elementSize);
    // Also copy to the other half. Are we in the upper?
    if (end >= (buffer + usable)) {
      // Yes, simply copy it into the lower
      memcpy(end - usable, data, size * elementSize);
    } else {
      // Special case: end + usable + size could be more than buffer + len can take.
      // In this case we will have to split the data to have the overlap copied to
      // the start of buffer!
      if ((end + usable + size) <= (buffer + len)) {
        // All fine, it still fits
        memcpy(end + usable, data, size * elementSize);
      } else {
        // Does not fit completely, we need to do a split copy
        // First part up to buffer + len
        uint16_t firstPart = len - (end - buffer + usable);
        memcpy(end + usable, data, firstPart * elementSize);
        // Second part (remainder) 
        memcpy(buffer, data + firstPart, (size - firstPart) * elementSize);
      }
    }
    end += size;
  }
  return true;
}

// operator[]: return the element the index is pointing to. If index is
// outside the currently used area, return 0
template <typename T>
const T RingBuf<T>::operator[](size_t index) {
  if (!valid()) return 0;
  if (index >= 0 && index < size()) {
    return *(begin + index);
  }
  return 0;
}

// safeCopy: get a stable data copy from currently used buffer
// target: buffer to copy data into
// len: number of elements requested
// move: if true, copied elements will be pop()-ped
// returns number of elements actually transferred
template <typename T>
size_t RingBuf<T>::safeCopy(T *target, size_t len, bool move) {
  if (!valid()) return 0;
  if (!target) return 0;
  {
    LOCK_GUARD(cLock, m);
    if (len > size()) len = size();
    memcpy(target, begin, len * elementSize);
  }
  if (move) pop(len);
  return len;
}

// Equality: sizes and contents must be identical
template <typename T>
bool RingBuf<T>::operator==(RingBuf<T> &r) {
  if (!valid() || !r.valid()) return false;
  if (size() != r.size()) return false;
  if (memcmp(begin, r.begin, size() * elementSize)) return false;
  return true;
}
#endif
