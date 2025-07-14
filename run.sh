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
    echo "0) Aguardar dispositivo"
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

wait_for_device() {
    echo "Aguardando ESP8266..."
    echo "Pressione ENTER quando o dispositivo estiver conectado, ou 'q' para cancelar"
    
    while true; do
        # Verificar se há entrada do usuário
        if read -t 1 -n 1 input 2>/dev/null; then
            if [[ $input == "q" ]] || [[ $input == "Q" ]]; then
                echo "\nCancelado pelo usuário"
                return 1
            fi
        fi
        
        # Verificar se dispositivo apareceu
        CURRENT_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
        if [ -n "$CURRENT_PORT" ]; then
            PORT="$CURRENT_PORT"
            echo "\n✓ ESP8266 detectado em: $PORT"
            return 0
        fi
        
        echo -n "."
    done
}

while true; do
    show_menu
    read -r choice
    
    case $choice in
        1)
            echo "=== COMPILAR E UPLOAD ==="
            echo "Compilando primeiro..."
            pio run
            if [ $? -eq 0 ]; then
                echo "✓ Compilação OK! Verificando porta para upload..."
                # Re-verificar porta antes do upload
                CURRENT_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
                if [ -n "$CURRENT_PORT" ]; then
                    PORT="$CURRENT_PORT"
                    echo "Porta detectada: $PORT"
                    echo "Iniciando upload..."
                    pio run --target upload --upload-port $PORT
                    echo "Upload concluído!"
                else
                    echo "✗ ESP8266 não encontrado."
                    echo "Reconecte o cabo USB e pressione ENTER, ou 'q' para cancelar"
                    if wait_for_device; then
                        echo "Tentando upload novamente..."
                        pio run --target upload --upload-port $PORT
                        echo "Upload concluído!"
                    fi
                fi
            else
                echo "✗ Erro na compilação"
            fi
            ;;
        2)
            echo "=== UPLOAD SISTEMA DE ARQUIVOS ==="
            echo "⚠ ATENÇÃO: Isso apagará todos os dados coletados!"
            echo -n "Continuar? (s/N): "
            read -r confirm
            if [[ $confirm =~ ^[Ss]$ ]]; then
                # Re-verificar porta antes do upload
                CURRENT_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
                if [ -n "$CURRENT_PORT" ]; then
                    PORT="$CURRENT_PORT"
                    echo "Porta detectada: $PORT"
                    echo "Fazendo upload do sistema de arquivos..."
                    pio run --target uploadfs --upload-port $PORT
                    echo "Upload FS concluído!"
                else
                    echo "✗ ESP8266 não encontrado. Reconecte o cabo USB."
                fi
            else
                echo "Upload cancelado"
            fi
            ;;
        3)
            echo "=== MONITOR SERIAL ==="
            # Re-verificar porta antes do monitor
            CURRENT_PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
            if [ -n "$CURRENT_PORT" ]; then
                PORT="$CURRENT_PORT"
                echo "Conectando ao monitor serial em $PORT..."
                echo "Para sair: Ctrl+C"
                echo "Para menu da bike: digite 'm'"
                echo "=========================="
                pio device monitor --baud 115200 --port $PORT
            else
                echo "✗ ESP8266 não encontrado. Reconecte o cabo USB."
            fi
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
        0)
            echo "=== AGUARDAR DISPOSITIVO ==="
            wait_for_device
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