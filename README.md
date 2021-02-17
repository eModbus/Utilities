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

## TelnetLog
A class to duplicate output to all connected Telnet clients (i.e. do logging over Telnet). 
As ``TelnetLog`` is derived from ``Print``, all well-known print and write functions are supported.
Instead of the notorious ``Serial.print()`` calls, ``TelnetLog`` functions may be used in the same manner.

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

This needs to be called frequently, as it is maintaining the list of connected clients. New clients are accepted, closed connections are cleaned up.

### isActive()
``bool isActive();``

This call returns ``true`` if at least one client connection is currently open, and ``false`` else.
