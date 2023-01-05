import time
import os

while True:
    start = time.time()
    os.system("python train.py")
    end = time.time()
    if end - start < 45:
        time.sleep(5)