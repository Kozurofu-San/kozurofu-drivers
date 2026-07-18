import numpy as np

F_CPU = 16e6        # Hz
TIMER_PERIOD = 1000 # us
MAX_COUNTER = 256   # 8 bits
prescalers = np.array([1, 8, 32, 64, 128, 256, 1024])
prescalers = np.array([1, 8, 64, 256, 1024])
cs = 0
for div in prescalers:
    cs += 1
    period = F_CPU / (1e6 / TIMER_PERIOD) / div
    if period < MAX_COUNTER:
        break
print(f"TIMER_DIV {div} CS {cs} TIMER_CNT {period - 1}")

F_CPU = 16000000
UART_SPEED = 115200
UART_BAUDRATE = F_CPU // (UART_SPEED * 8) - 1
# print(UART_BAUDRATE)