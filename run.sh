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
    echo "9) Auto-detectar porta"
    echo "q) Sair"
    echo "=================================="
    echo -n "Escolha uma opção: "
}

check_port() {
    echo "Verificando portas USB disponíveis..."
    
    # Verificar portas USB seriais
    USB_PORTS=$(ls /dev/ttyUSB* 2>/dev/null)
    ACM_PORTS=$(ls /dev/ttyACM* 2>/dev/null)
    
    if [ -n "$USB_PORTS" ] || [ -n "$ACM_PORTS" ]; then
        echo "✓ Portas encontradas:"
        [ -n "$USB_PORTS" ] && echo "USB: $USB_PORTS"
        [ -n "$ACM_PORTS" ] && echo "ACM: $ACM_PORTS"
        
        echo "Porta configurada: $PORT"
        if [ -e "$PORT" ]; then
            echo "✓ Porta $PORT está disponível"
            # Verificar se é acessível
            if [ -r "$PORT" ] && [ -w "$PORT" ]; then
                echo "✓ Porta $PORT tem permissões corretas"
            else
                echo "⚠ Porta $PORT sem permissões (execute opção 5)"
            fi
        else
            echo "⚠ Porta $PORT não encontrada"
            FIRST_PORT=$(echo "$USB_PORTS $ACM_PORTS" | awk '{print $1}')
            [ -n "$FIRST_PORT" ] && echo "Sugestão: use $FIRST_PORT"
        fi
    else
        echo "✗ Nenhuma porta USB encontrada"
        echo "Verifique se o ESP8266 está conectado"
        echo "Dica: Desconecte e reconecte o cabo USB"
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
        9)
            echo "=== AUTO-DETECTAR PORTA ==="
            DETECTED_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
            if [ -n "$DETECTED_PORT" ]; then
                PORT="$DETECTED_PORT"
                echo "✓ Porta detectada e configurada: $PORT"
            else
                echo "✗ Nenhuma porta encontrada"
            fi
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