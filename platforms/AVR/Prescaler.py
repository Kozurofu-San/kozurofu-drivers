import numpy as np

prescalers = np.array([1, 8, 64, 256, 1024])
for i in prescalers:
    period = 16e6 / 1e3 / i
    if period < 256:
        print(i)
        break

prescalers = np.array([1, 8, 32, 64, 128, 256, 1024])
for i in prescalers:
    period = 16e6 / 1e3 / i
    if period < 256:
        print(i)
        break

F_CPU = 16000000
UART_SPEED = 115200
UART_BAUDRATE = F_CPU // (UART_SPEED * 8) - 1
print(UART_BAUDRATE)