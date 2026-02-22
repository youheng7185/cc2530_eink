#!/usr/bin/env python3
"""
eink.py — CC2530 e-ink display tool

Commands:
  python eink.py clear              — clear display
  python eink.py send <file.bin>    — upload framebuffer to MCU RAM
  python eink.py write              — display the buffer currently in MCU RAM
  python eink.py show  <file.bin>   — send + write in one step

Protocol:
  CMD_SEND  (0x69) + 2756 bytes  → MCU stores in __xdata framebuffer, ACKs
  CMD_WRITE (0x57)               → MCU sends framebuffer to EPD, ACKs when done
  CMD_CLEAR (0x43)               → MCU clears EPD, ACKs when done
  ACK = 0x06
"""

import serial
import sys
import time

# ── config ────────────────────────────────────────────────────────────────────
PORT            = "/dev/ttyACM1"
BAUD            = 921600
FRAMEBUFFER_SIZE = 2756

CMD_SEND  = 0x69   # upload buffer to MCU RAM only
CMD_WRITE = 0x57   # send MCU RAM buffer to EPD
CMD_CLEAR = 0x43   # clear EPD

ACK           = 0x06
ACK_TIMEOUT   = 30     # seconds — EPD refresh takes ~3s
CHUNK_SIZE    = 64     # bytes per chunk during framebuffer upload
CHUNK_DELAY   = 0.002  # 2ms between chunks — gives CC2530 time to read each

# ── serial helpers ────────────────────────────────────────────────────────────
def open_serial():
    return serial.Serial(
        port=PORT,
        baudrate=BAUD,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=ACK_TIMEOUT,
    )

def wait_ack(ser, label=""):
    b = ser.read(1)
    if not b:
        raise TimeoutError(f"No ACK received{' for ' + label if label else ''}")
    if b[0] != ACK:
        raise RuntimeError(f"Expected ACK 0x06, got 0x{b[0]:02x}{' (' + label + ')' if label else ''}")
    print(f"  ✓ ACK{' [' + label + ']' if label else ''}")

# ── commands ──────────────────────────────────────────────────────────────────
def cmd_send(ser, framebuffer):
    """Upload framebuffer bytes to MCU RAM. Does NOT update the display."""
    if len(framebuffer) != FRAMEBUFFER_SIZE:
        raise ValueError(f"Expected {FRAMEBUFFER_SIZE} bytes, got {len(framebuffer)}")

    print(f"[send] uploading {FRAMEBUFFER_SIZE} bytes to MCU RAM...")
    ser.write(bytes([CMD_SEND]))
    ser.flush()

    wait_ack(ser, "ready for data")   # MCU ACK 1: ready to receive

    sent = 0
    while sent < FRAMEBUFFER_SIZE:
        chunk = framebuffer[sent:sent + CHUNK_SIZE]
        ser.write(chunk)
        ser.flush()
        sent += len(chunk)
        time.sleep(CHUNK_DELAY)
        if sent % 512 == 0 or sent >= FRAMEBUFFER_SIZE:
            print(f"  {sent}/{FRAMEBUFFER_SIZE} bytes", end="\r")

    print()
    wait_ack(ser, "buffer stored")    # MCU ACK 2: all bytes received
    print("[send] done")

def cmd_write(ser):
    """Tell MCU to push its RAM buffer to the EPD."""
    print("[write] sending buffer to display...")
    ser.write(bytes([CMD_WRITE]))
    ser.flush()
    wait_ack(ser, "EPD starting")     # MCU ACK 1: command received
    wait_ack(ser, "EPD done")         # MCU ACK 2: refresh complete
    print("[write] display updated")

def cmd_clear(ser):
    """Clear the display."""
    print("[clear] clearing display...")
    ser.write(bytes([CMD_CLEAR]))
    ser.flush()
    wait_ack(ser, "clear starting")   # MCU ACK 1: command received
    wait_ack(ser, "clear done")       # MCU ACK 2: refresh complete
    print("[clear] done")

# ── main ──────────────────────────────────────────────────────────────────────
USAGE = """
Usage:
  python eink.py clear              clear the display
  python eink.py send  <file.bin>   upload image to MCU RAM
  python eink.py write              push MCU RAM to display
  python eink.py show  <file.bin>   send + write (upload and display)
"""

def main():
    if len(sys.argv) < 2:
        print(USAGE)
        sys.exit(1)

    command = sys.argv[1].lower()

    with open_serial() as ser:
        time.sleep(0.5)   # let CDC enumerate
        ser.reset_input_buffer()

        if command == "clear":
            cmd_clear(ser)

        elif command == "send":
            if len(sys.argv) < 3:
                print("Error: send requires a binary file argument")
                sys.exit(1)
            with open(sys.argv[2], "rb") as f:
                fb = f.read()
            cmd_send(ser, fb)

        elif command == "write":
            cmd_write(ser)

        elif command == "show":
            if len(sys.argv) < 3:
                print("Error: show requires a binary file argument")
                sys.exit(1)
            with open(sys.argv[2], "rb") as f:
                fb = f.read()
            cmd_send(ser, fb)
            cmd_write(ser)

        else:
            print(f"Unknown command: {command}")
            print(USAGE)
            sys.exit(1)

if __name__ == "__main__":
    main()
