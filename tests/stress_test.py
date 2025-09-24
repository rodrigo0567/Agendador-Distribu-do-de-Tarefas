#!/usr/bin/env python3
import socket
import threading
import time
import random

SERVER_HOST = 'localhost'
SERVER_PORT = 8080

scripts = [
    "python -c \"print('Hello World')\"",
    "python -c \"import time; time.sleep(2); print('Done')\"",
    "lua -e \"print('Lua script running')\"",
    "python -c \"for i in range(3): print(f'Line {i}')\""
]

def client_worker(client_id):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_HOST, SERVER_PORT))
        
        script = random.choice(scripts)
        message = f"JOB:{script}"
        
        print(f"Client {client_id} sending: {message}")
        sock.send(message.encode())
        
        response = sock.recv(1024).decode()
        print(f"Client {client_id} received: {response}")
        
        sock.close()
    except Exception as e:
        print(f"Client {client_id} error: {e}")

def stress_test(num_clients=5):
    threads = []
    
    print(f"Starting stress test with {num_clients} clients...")
    
    for i in range(num_clients):
        t = threading.Thread(target=client_worker, args=(i+1,))
        threads.append(t)
        t.start()
        time.sleep(0.1)  # Pequeno delay entre conexões
    
    for t in threads:
        t.join()
    
    print("Stress test completed!")

if __name__ == "__main__":
    stress_test(10)