// Copyright (c) 2021 miq1 @ gmx . de

#include "RingBuf.h"

const uint8_t  RingBuf::nilBuf[2] = { 0, 0 };

// setFail: in case of memory allocation problems, use static nilBuf 
void RingBuf::setFail() {
  buffer = (uint8_t *)RingBuf::nilBuf;
  len = 2;
  usable = 0;
  begin = end = buffer;
}

// valid: return if buffer is a real one
bool RingBuf::valid() {
  return (buffer && (buffer != RingBuf::nilBuf));
}

// operator bool: same as valid()
RingBuf::operator bool() {
  return valid();
}

// Constructor: allocate a buffer twice the requested size
RingBuf::RingBuf(size_t size, bool p) :
  len(size * 2),
  usable(size),
  preserve(p) {
  // Allocate memory
  buffer = new uint8_t[len];
  // Failed?
  if (!buffer) setFail();
  else clear();
}

// Destructor: free allocated memory, if any
RingBuf::~RingBuf() {
  // Do we have a valid buffer?
  if (valid()) {
    // Yes, free it
    delete buffer;
  }
}

// Copy constructor: take over everything
RingBuf::RingBuf(const RingBuf &r) {
  // Is the assigned RingBuf valid?
  if (r.buffer && (r.buffer != RingBuf::nilBuf)) {
    // Yes. Try to allocate a copy
    buffer = new uint8_t[r.len];
    // Succeeded?
    if (buffer) {
      // Yes. copy over data
      len = r.len;
      memcpy(buffer, r.buffer, len);
      begin = buffer + (r.begin - r.buffer);
      end = buffer + (r.end - r.buffer);
      preserve = r.preserve;
      usable = r.usable;
    } else {
      setFail();
    }
  } else {
    setFail();
  }
}

// Move constructor
RingBuf::RingBuf(RingBuf &&r) {
  // Is the assigned RingBuf valid?
  if (r.buffer && (r.buffer != RingBuf::nilBuf)) {
    // Yes. Take over the data
    buffer = r.buffer;
    len = r.len;
    begin = buffer + (r.begin - r.buffer);
    end = buffer + (r.end - r.buffer);
    preserve = r.preserve;
    usable = r.usable;
    r.setFail();
  } else {
    setFail();
  }
}

// Assignment
RingBuf& RingBuf::operator=(const RingBuf &r) {
  if (valid()) {
    // Is the source a real RingBuf?
    if (r.buffer && (r.buffer != RingBuf::nilBuf)) {
      // Yes. Copy over the data
      clear();
      push_back(r.begin, r.end - r.begin);
    }
  }
  return *this;
}

// Move assignment
RingBuf& RingBuf::operator=(RingBuf &&r) {
  if (valid()) {
    // Is the source a real RingBuf?
    if (r.buffer && (r.buffer != RingBuf::nilBuf)) {
      // Yes. Copy over the data
      clear();
      push_back(r.begin, r.end - r.begin);
      r.setFail();
    }
  }
  return *this;
}

// size: number of bytes used in the buffer
size_t RingBuf::size() {
  return end - begin;
}

// data: get start of used data area
const uint8_t *RingBuf::data() {
  return begin;
}

// empty: is any data in buffer?
bool RingBuf::empty() {
  return ((size() == 0) || !valid());
}

// capacity: return remaining usable size
size_t RingBuf::capacity() {
  if (!valid()) return 0;
  return usable - size();
}

// clear: forget about contents
bool RingBuf::clear() {
  if (!valid()) return false;
  LOCK_GUARD(cLock, m);
  begin = end = buffer;
  return true;
}

// pop: remove bytes from the beginning of the buffer
size_t RingBuf::pop(size_t numBytes) {
  if (!valid()) return 0;
  // Do we have data in the buffer?
  if (size()) {
    // Yes. Is the requested number of bytes larger than the used buffer?
    if (numBytes >= size()) {
      // Yes. clear the buffer
      size_t n = size();
      clear();
      return n;
    } else {
      LOCK_GUARD(cLock, m);
      // No, the buffer needs to be emptied partly only
      begin += numBytes;
      // Is begin now pointing into the upper half?
      if (begin >= (buffer + usable)) {
        // Yes. Move begin and end down again
        begin -= usable;
        end -= usable;
      }
    }
    return numBytes;
  }
  return 0;
}

// push_back(single byte): add one byte to the buffer, potentially discarding previous ones
bool RingBuf::push_back(const uint8_t c) {
  if (!valid()) return false;
  {
    LOCK_GUARD(cLock, m);
    // Are we below the capacity?
    if (capacity()) {
      // Yes. Just add the byte
      *end = c;
      // Is end pointing to the second half?
      if (end >= (buffer + usable)) {
        // Yes. Add the byte to the lower half as well
        *(end - usable) = c;
      } else {
        // No, we need to add it to the upper half
        *(end + usable) = c;
      }
      end++;
      return true;
    } 
    // No, we need to drop something
    // Are we to keep the oldest data?
    if (preserve) {
      // Yes. The new byte will be dropped to leave the buffer untouched
      return false;
    }
    // We need to drop the oldest byte begin is pointing to
    begin++;
    // Overflow?
    if (begin >= (buffer + usable)) {
      begin = buffer;
      end = begin + usable - 1;
    }
  }
  // Recursively call ourselves
  return push_back(c);
}

// push_back(byte buffer): add a batch of bytes to the buffer
bool RingBuf::push_back(const uint8_t *data, size_t size) {
  if (!valid()) return false;
  // Avoid self-referencing pushes
  if (data >= buffer && data <= (buffer + len)) return false;
  {
    LOCK_GUARD(cLock, m);
    // Is the size to be added fitting the capacity?
    if (size <= capacity()) {
      // Yes. copy it in
      memcpy(end, data, size);
      if (end >= (buffer + usable)) {
        memcpy(end - usable, data, size);
      } else {
        // Special case: end + usable + size could be more than buffer + len can take.
        // In this case we will have to split the data to have the overlap copied to
        // the start of buffer!
        if ((end + usable + size) <= (buffer + len)) {
          // All fine, it still fits
          memcpy(end + usable, data, size);
        } else {
          // Does not fit completely, we need to do a split copy
          // First part up to buffer + len
          uint16_t firstPart = len - (end - buffer + usable);
          memcpy(end + usable, data, firstPart);
          // Second part (remainder) 
          memcpy(buffer, data + firstPart, size - firstPart);
        }
      }
      end += size;
      return true;
    }
    // No, we need more room, discarding older bytes for it.
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
    begin += size;
    if (begin >= (buffer + usable)) {
      begin -= usable;
      end -= usable;
    }
  }
  // Recursively call ourselves
  return push_back(data, size);
}

// Equality: sizes and contents must be identical
bool RingBuf::operator==(RingBuf &r) {
  if (!valid() || !r.valid()) return false;
  if (size() != r.size()) return false;
  if (memcmp(begin, r.begin, size())) return false;
  return true;
}
