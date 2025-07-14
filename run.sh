#!/bin/bash

# WiFi Range Scanner - ESP8266
# Script de comandos principais

PORT="/dev/ttyUSB0"

show_menu() {
    echo "=================================="
    echo "  WiFi Range Scanner - ESP8266"
    echo "=================================="
    echo "1) Compilar e fazer upload"
    echo "2) Upload do sistema de arquivos (uploadfs)"
    echo "3) Monitor serial"
    echo "4) Verificar conexão com porta"
    echo "5) Configurar permissões USB"
    echo "6) Upload forçando porta específica"
    echo "7) Compilar apenas (sem upload)"
    echo "8) Limpar build"
    echo "q) Sair"
    echo "=================================="
    echo -n "Escolha uma opção: "
}

check_port() {
    echo "Verificando portas USB disponíveis..."
    if ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null; then
        echo "✓ Portas encontradas"
        echo "Porta configurada: $PORT"
        if [ -e "$PORT" ]; then
            echo "✓ Porta $PORT está disponível"
        else
            echo "⚠ Porta $PORT não encontrada"
            echo "Portas disponíveis:"
            ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || echo "Nenhuma porta USB encontrada"
        fi
    else
        echo "✗ Nenhuma porta USB encontrada"
        echo "Verifique se o ESP8266 está conectado"
    fi
}

configure_permissions() {
    echo "Configurando permissões USB..."
    echo "Adicionando usuário ao grupo dialout..."
    sudo usermod -a -G dialout $USER
    echo "Aplicando mudanças de grupo..."
    newgrp dialout
    echo "✓ Configuração concluída"
    echo "⚠ Pode ser necessário fazer logout/login para aplicar as mudanças"
}

while true; do
    show_menu
    read -r choice
    
    case $choice in
        1)
            echo "=== COMPILAR E UPLOAD ==="
            check_port
            echo "Iniciando upload..."
            pio run --target upload --upload-port $PORT
            echo "Upload concluído!"
            ;;
        2)
            echo "=== UPLOAD SISTEMA DE ARQUIVOS ==="
            echo "⚠ ATENÇÃO: Isso apagará todos os dados coletados!"
            echo -n "Continuar? (s/N): "
            read -r confirm
            if [[ $confirm =~ ^[Ss]$ ]]; then
                check_port
                echo "Fazendo upload do sistema de arquivos..."
                pio run --target uploadfs --upload-port $PORT
                echo "Upload FS concluído!"
            else
                echo "Upload cancelado"
            fi
            ;;
        3)
            echo "=== MONITOR SERIAL ==="
            echo "Conectando ao monitor serial..."
            echo "Para sair: Ctrl+C"
            echo "Para menu da bike: digite 'm'"
            echo "=========================="
            pio device monitor --baud 115200 --port $PORT
            ;;
        4)
            echo "=== VERIFICAR CONEXÃO ==="
            check_port
            ;;
        5)
            echo "=== CONFIGURAR PERMISSÕES ==="
            configure_permissions
            ;;
        6)
            echo "=== UPLOAD COM PORTA ESPECÍFICA ==="
            echo -n "Digite a porta (ex: /dev/ttyUSB0): "
            read -r custom_port
            if [ -e "$custom_port" ]; then
                echo "Fazendo upload para $custom_port..."
                pio run --target upload --upload-port $custom_port
            else
                echo "✗ Porta $custom_port não encontrada"
            fi
            ;;
        7)
            echo "=== COMPILAR APENAS ==="
            echo "Compilando projeto..."
            pio run
            echo "Compilação concluída!"
            ;;
        8)
            echo "=== LIMPAR BUILD ==="
            echo "Limpando arquivos de build..."
            pio run --target clean
            echo "Build limpo!"
            ;;
        q|Q)
            echo "Saindo..."
            exit 0
            ;;
        *)
            echo "Opção inválida!"
            ;;
    esac
    
    echo ""
    echo "Pressione Enter para continuar..."
    read -r
    clear
done