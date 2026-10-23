import serial
import time
import threading
from pynput import keyboard

# ==============================================================================
# RGBGameEngine v2.0 — Live Interactive PC Controller (COM8)
# ==============================================================================
COM_PORT = 'COM8'
BAUD_RATE = 115200

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.05)
    print(f"\n==============================================================")
    print(f" Connected to RGBGameEngine on {COM_PORT} @ {BAUD_RATE} baud!")
    print(f"==============================================================")
    print(f" [1 - 9]        : Switch Scene 1..9 directly")
    print(f" [0]            : Toggle Auto-Cycle ON / OFF")
    print(f" [W / A / S / D]: Move 2D Cursor / Light / Force / Emitter")
    print(f" [+ / -] or [Up/Down] or [I / K] : Increase / Decrease Intensity")
    print(f" [SPACE] or [J] : Trigger Action (Shatter / Slam / Tear / Splash)")
    print(f" [M] or [E]     : Cycle Mode / Shatter Material / Emitter Shape")
    print(f" [B]            : Cycle Particle Behavior (Scene 9)")
    print(f" [R]            : Reset Current Scene Objects")
    print(f" [ESC]          : Quit Controller")
    print(f"==============================================================\n")
except Exception as e:
    print(f"Error connecting to {COM_PORT}: {e}")
    print("Tip: Make sure Arduino IDE Serial Monitor is closed so COM8 is free!")
    exit()

# Background thread to print serial feedback from the ESP32-S3
stop_thread = False
def read_serial():
    while not stop_thread:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"[ESP32] {line}")
        except Exception:
            pass
        time.sleep(0.02)

reader = threading.Thread(target=read_serial, daemon=True)
reader.start()

# Track currently held movement keys so releasing one doesn't stop another
held_move_keys = set()

def send_cmd(ch):
    try:
        ser.write(ch.encode('utf-8'))
    except Exception as e:
        print(f"Serial write error: {e}")

def on_press(key):
    try:
        # Handle special keys (Space, Up/Down/Left/Right arrows, Esc)
        if key == keyboard.Key.space:
            send_cmd(' ')
            return
        elif key == keyboard.Key.up:
            send_cmd('+')
            return
        elif key == keyboard.Key.down:
            send_cmd('-')
            return
        elif key == keyboard.Key.left:
            if 'a' not in held_move_keys:
                held_move_keys.add('a')
                send_cmd('A')
            return
        elif key == keyboard.Key.right:
            if 'd' not in held_move_keys:
                held_move_keys.add('d')
                send_cmd('D')
            return

        if hasattr(key, 'char') and key.char is not None:
            c = key.char.lower()

            # Continuous movement keys (WASD)
            if c in ('w', 'a', 's', 'd'):
                if c not in held_move_keys:
                    held_move_keys.add(c)
                    send_cmd(c.upper())

            # Scene switching (1-9, 0 for auto-cycle, N for next)
            elif c in ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'n'):
                send_cmd(c.upper())

            # Intensity controls (+ / = / i for UP, - / _ / k for DOWN)
            elif c in ('+', '=', 'i'):
                send_cmd('+')
            elif c in ('-', '_', 'k'):
                send_cmd('-')

            # Mode / Material / Emitter / Behavior / Reset / Action
            elif c in ('m', 'e', 'b', 'r', 'j'):
                send_cmd(c.upper())

    except Exception:
        pass

def on_release(key):
    global stop_thread
    if key == keyboard.Key.esc:
        stop_thread = True
        print("Exiting RGBGameEngine Controller...")
        return False

    try:
        released_move = None
        if key == keyboard.Key.left:
            released_move = 'a'
        elif key == keyboard.Key.right:
            released_move = 'd'
        elif hasattr(key, 'char') and key.char is not None:
            c = key.char.lower()
            if c in ('w', 'a', 's', 'd'):
                released_move = c

        if released_move and released_move in held_move_keys:
            held_move_keys.remove(released_move)
            if len(held_move_keys) == 0:
                send_cmd('X')  # Stop cursor velocity when no WASD keys are held
            else:
                # Re-send the remaining held direction key
                remaining = next(iter(held_move_keys))
                send_cmd(remaining.upper())
    except Exception:
        pass

with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
    listener.join()

ser.close()
