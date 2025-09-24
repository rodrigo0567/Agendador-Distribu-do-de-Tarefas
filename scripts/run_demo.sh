#!/bin/bash

echo "=== DEMO Etapa 2 - Agendador Distribuído ==="

# Compilar projeto
echo "Compilando..."
make clean
make

# Iniciar servidor em background
echo "Iniciando servidor..."
./server &
SERVER_PID=$!
sleep 2

# Executar clientes de teste
echo "Executando testes..."
./tests/stress_test.py

# Cliente interativo
echo "Iniciando cliente interativo (digite 'quit' para sair)"
./client interactive

# Finalizar servidor
echo "Finalizando servidor..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Demo concluído!"
echo "Logs disponíveis em server.log e client.log"