# Shuttle Panels

Collection of code to run 'shuttle panels' - physical panels that represent parts of a shuttle (used in a LARP), which contain puzzles to physrep repairing these shuttles.

## The panels

NB: Currently only the connector panel exists

### Connector panel

35x50 panel with 100 banana plug sockets and 96 leds.

Uses a raspberry pi zero (or zeroW), seven MCP23017 chips, WS2812B leds (neopixels), and an i2s chip plus amplifier.

The MCP23017s interface with bananaplug connectors which the software reads to see which sockets are joined by a cable.  This then runs a mastermind-like puzzle where the LEDs give clues to the game.

### Access panel

NB: To be designed, subject to change.

Small panel to provide a puzzle for opening sci-fi doors.
