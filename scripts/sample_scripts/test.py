#!/usr/bin/env python3
import time
import math

print("=== Python Script ===")
print("Tempo atual:", time.time())
print("Raiz quadrada de 144:", math.sqrt(144))

for i in range(3):
    print(f"Contagem: {i+1}")
    time.sleep(0.5)

print("Script concluído!")
