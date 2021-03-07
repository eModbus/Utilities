# Utilities

A collection of utility classes I wrote for myself.

## Blinker
A class to maintain arbitrary blinking patterns for LEDs.

### Constructor
``Blinker(uint8_t port, bool onState = HIGH);``

Takes the GPIO number the LED is connected to, and optionally the logic level it needs to be set to to light the LED (defaults to HIGH).

Example: ``Blinker myLED(GPIO_NUM_13);``
  
### start()
``uint32_t start(uint16_t pattern = BLINKER_PATTERN, uint32_t interval = BLINKER_DEFAULT);``

In ``interval`` steps, loop over blinking ``pattern``. ``pattern`` will switch on the LED for every '1' bit and off for every '0' bit.
A switch state will last ``interval`` milliseconds.
Leading '0' bits are ignored. 
Example: to switch the LED on and off in even cycles, a ``pattern = 0x2;`` will do. As leading zero bits are skipped, the pattern collapses to the binary ``10`` sequence, which means "one interval on, one interval off and start over". 
To have the LED blink for quarter of a second intervals, the call would be ``myLED.start(0x2, 250);``

### stop()
``void stop();``
Stop the blinking. Another ``start()`` may use different parameters to change the blinking pattern etc.

### update()
``void update();``

This is the polling call to check if the blinking pattern needs to be advanced a step.
It is best placed in your ``loop()`` to be called frequently.

## Buttoner
A class to watch push buttons connected to a GPIO for clicks, double clicks and long presses.

### Constructor
``Buttoner(int port, bool onState = HIGH, bool pullUp = false, uint32_t queueSize = 4);``

The arguments are
- ``port``: GPIO number (mandatory)
- ``onState``: logic level of the GPIO when the button is pressed (default=HIGH)
- ``pullUp``: set to true to have the GPIO configured as INPUT_PULLUP (default=no pull-up)
- ``queueSize``: number of events to keep (0=unlimited, default=4)

### update()
``int update();``

This is the polling function to read the button state and generate events. 
This function needs to be called frequently!
It returns the number of events currently held in queue or -1, if the sampling period is not yet finished.

### getEvent()
``ButtonEvent getEvent();``

Pull the first (oldest) event from queue, deleting it from the queue. 
Events are:
- ``BE_NONE``: no event currently held in queue
- ``BE_CLICK``: a single click has been registered
- ``BE_DOUBLECLICK``: a double click was detected
- ``BE_PRESS``: the button was held for a while

### peekEvent()
``ButtonEvent peekEvent();``

Like ``getEvent()``, but the event is left untouched on the queue for repeated inquiries.

### clearEvents()
``void clearEvents();``

Purge all events unseen from queue.

### setTiming()
``void setTiming(uint32_t doubleClickTime, uint32_t pressTime);``

Adjust the times for double clicks and held buttons to your preferences. 
``doubleClickTime`` gives the time to pass at maximum between the first and second click of a double click. 
If the time is exceeded, a single click is assumed.
``pressTime`` gives the time a button needs to be held at minimum to determine a long button press.
**Note**: ``pressTime`` should be longer than ``doubleClickTime`` to avoid confusions.
  
 ### qSize()
 ``uint32_t qSize();``
 
 Get the number of events currently held in queue.

## TelnetLog and TelnetLogAsync
A class to duplicate output to all connected Telnet clients (i.e. do logging over Telnet). 
As ``TelnetLog`` is derived from ``Print``, all well-known print and write functions are supported.
Instead of the notorious ``Serial.print()`` calls, ``TelnetLog`` functions may be used in the same manner.

TelnetLogAsync is using the ESPAsyncTCP library by me-no-dev, which is especially well-suited to be used on ESP8266 MCUs.
The Async variety is requiring the ``RingBuf`` tool internally (see below). Please note that this means additional memory consumption.

### Constructor
``TelnetLog(uint16_t port, uint8_t maxClients);``

``port`` usually will be 23 for Telnet, but may be chosen freely.
``maxClients`` sets the maximum number of concurrent client connections accepted.

## begin()
``void begin(const char *label);``

Start the Telnet server. New connected clients will be greeted with the label ("Bridge-Test" in the example below) and some additional data:
```
Welcome to 'Bridge-Test'!
Millis since start: 19274
Free Heap RAM: 251448
Server IP: 192.168.178.74
----------------------------------------------------------------
```

### end()
``void end();``

Stops the Telnet server. Existing connections will be closed.

### update()
``void update();``

Non-Async only: this needs to be called frequently, as it is maintaining the list of connected clients. New clients are accepted, closed connections are cleaned up.
The Async version does not require this call but does poll internally.

### isActive()
``bool isActive();``

This call returns ``true`` if at least one client connection is currently open, and ``false`` else.

## RingBuf
``RingBuf`` is the implementation of a circular buffer for atomic data types (those with a fixed, known sizeof()). 

Example, defining a circular buffer for up to 20 ``int`` values named ``intBuffer``:
```
#include "RingBuf.h"

RingBuf<int> intBuffer(20);
```

For speed and convenience reasons the internal buffer is **twice** the size of the requested buffer, so please be sure to have an eye on your remaining memory.
The interface loosely follows that of ``std::vector``, but has some additions and omissions.
``RingBuf`` is supporting the Move&& variants for copy constructors and assignments.

**Note:** Due to the nature of the "rolling" buffer, the data in it may be highly volatile. 
Any function to get the data from the buffer will only have a snapshot at the time the access is made.
The buffer may be different in the next milliseconds.
So **NEVER** copy the address or size into any local variable to use it, but **ALWAYS** use the ``data()``, ``size()``, ``capacity()`` etc. calls!

### Constructor
``RingBuf<typename>(size_t size = 256, bool preserve = false);``

The constructor takes the required usable size (remember the internal buffer is **twice** this size!) as first argument.
The second argument controls the strategy to handle a completely filled buffer.
If set to ``true``, a full buffer will not accept any further data unless some is removed before (with ``pop()``), hence preserving the oldest data.
The opposite (``false``) is the default and will discard the oldest data to make room for the new.

``typename`` must be an atomic type, like ``float``, ``uint8_t`` or the like.

If the buffer allocation will fail, the ``RingBuf`` object will point to the const ``RingBuf::nilBuf`` buffer and will not be usable at all.

### Assignment
``RingBuf& operator=(const RingBuf &r);`` and ``RingBuf& operator=(RingBuf &&r);``

Asignment is done for the actual content of the buffer, so the buffer of the recipient and that of the originator may be of different sizes. 
If the recipient's buffer is too small to hold all data from the source, only the fitting newest data is copied.

### size()
``size_t size();``

Get number of elements currently in buffer. 
Due to the nature of the rolling buffer, this size is VOLATILE and needs to be read again every time the buffer is used! Else data may be missed.

### data()
``const typename *data();``

Get start address of the elements in buffer.
Due to the nature of the rolling buffer, this address is VOLATILE and needs to be read again every time the buffer is used! Else old data may be read.
A read usually will be done like ``memcpy(target, ringbuf.data(), ringbuf.size() * sizeof(typename));`` to not spend too much time between reading the start address and buffer size and the read proper.

### empty()
``bool empty();``
Returns true if no elements are in the buffer.

### valid()
``bool valid();``
``operator bool();``

``valid`` returns true if a buffer was correctly allocated and is usable.
Using a ``RingBuf`` object in bool context returns the same information.

### capacity()
``size_t capacity();``

This function returns the number of currently unused elements in buffer.
Due to the nature of the rolling buffer, this size is VOLATILE and needs to be read again every time the buffer is used! Else data may be missed.

### clear()
``bool clear();``

This call is used to completely empty the buffer. Afterwards, the size will be 0.

### pop()
``size_t pop(size_t numElements);``

The buffer will keep its contents until one of the following is happening:
- newer data is put in, forcing out older data if necessary
- ``clear()`` is called to empty the buffer
- the leading ``numElements`` bytes are removed from the buffer with this ``pop()`` call.

Normally, a decently sized, flowing buffer will fill at the end, while being read from the front.
After a read the reading application will remove the read elements with a ``pop()`` to not read it again.

**Note:** as there are some milliseconds between a read (using ``data()`` and a size) and a subsequent ``pop()``, the buffer may have been changed in the meantime already.
So the ``pop()`` may remove elements not yet read, if the buffer is too small and/or rolling quickly!

### operator[]
``const typename operator[](size_t index);``

An expression like ``ringbuffer[i]`` will return the ``i``th element of the current buffer. If there is no element number ``i``, a zero will be returned.

### Iterator
``RingBuf`` is supporting a simple forward iterator, so ``begin()``, ``end()`` and range for loops are available.
Please note, though, that accessing the data by iterator may also be affected by another task modifying the data at the same time.
The iterator is no ``const_iterator``, so modifying the data is possible, but not advisable.

### safeCopy()
``size_t safeCopy(typename *target, size_t len, bool move = false);``

``safeCopy`` is the only way to get a stable data copy from the currently used buffer, as it blocks all modifications until the copy has been made.
This has consequences, though:
- you will need another target buffer to copy the data into - using additional memory.
- other tasks trying to update the buffer will be held

``target`` gives the start address of the buffer to copy into.
``len`` is the number of elements requested. If the actual size is below the given ``len``, less elements will be copied.
``move``, when given and set to ``true``, will do a ``pop()`` of the copied elements afterwards.
The function will return the number of elements actually copied.

### push_back()
``bool push_back(const typename c);``
``bool push_back(const typename *data, size_t size);``

``push_back`` is used to add data to the buffer. 
While the first variant will append a single element to the buffer, the second will add a block of data.
If the data provided is larger than the buffer can fit, only the last elements will be left in the buffer, discarding all preceeding.

### Comparison (equality)
``bool operator==(RingBuf &r);``

If both sizes and contents of two buffers are identical will return ``true``

### Debug functions
``const uint8_t *bufferAdr();``
``const size_t bufferSize();``

These two calls must be handled with care only. They will provide the starting address and internal size of the buffer, regardless of current usage.
While this does not make any sense in normal use, it may help detecting issues in debug situations.
