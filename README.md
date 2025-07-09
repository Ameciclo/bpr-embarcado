# WiFi Range Scanner - ESP8266

Sistema de monitoramento de redes WiFi para bicicletas com upload automático para Firebase.

## Configuração

### 1. Arquivos de Configuração

Edite os arquivos na pasta `data/` antes do upload:

#### `bike.txt`
```
sl01
```
ID único da bicicleta (formato: código 2 letras + número dois dígitos)

#### `timing.txt`
```
5000
30000
```
- Linha 1: Tempo de scan quando em movimento (ms)
- Linha 2: Tempo de scan quando na base (ms)

#### `bases.txt`
```
WiFi-Estacao-Central
senha123
WiFi-Oficina
senha456
WiFi-Deposito
senha789
```
Configurações das bases WiFi (SSID e senha alternados)

#### `firebase.txt`
```
https://seu-projeto-default-rtdb.firebaseio.com
AIzaSyA...SuaChaveAqui
```
URL do Firebase Realtime Database e chave de API

### 2. Upload do Sistema

```bash
# 1. Upload do sistema de arquivos (configurações)
pio run --target uploadfs

# 2. Upload do código
pio run --target upload

# 3. Monitor serial
pio device monitor --baud 115200
```

## Como Usar

### Primeira Configuração

1. **Ativar modo AP**:
   - Pressione e segure o botão FLASH durante o boot, OU
   - Use o menu serial: `m` → `7`

2. **Conectar ao WiFi**: `Bike-sl01` (senha: `12345678`)

3. **Acessar interface**: http://192.168.4.1

4. **Configurar bases WiFi** na página de configurações

### Reconfiguração

- **Via interface web**: Aproxime de uma base WiFi configurada
- **Via menu serial**: Digite `m` para acessar opções

## Indicadores LED

O LED interno indica o status do sistema:

- 🔴 **3 piscadas rápidas + pausa**: Modo AP/Configuração
- 🟡 **2 piscadas rápidas + pausa**: Coletando dados (normal)
- 🟢 **1 piscada lenta + pausa**: Conectado na base

## Menu Serial

Digite `m` no monitor serial para acessar:

1. **Monitorar redes** - Lista WiFi detectados
2. **Verificar conexão com base** - Testa conectividade
3. **Testar conexão Firebase** - Testa upload
4. **Mostrar configurações** - Exibe config atual
5. **Ver dados salvos** - Mostra arquivos coletados
6. **Transferir dados** - Exporta para backup
7. **Ativar modo AP** - Força modo configuração

## Funcionamento

### Coleta de Dados

- Escaneia redes WiFi periodicamente
- Armazena dados em arquivos JSON compactos
- Registra: SSID, BSSID, RSSI, canal, timestamp
- Monitora nível de bateria e status de carregamento

### Detecção de Bases

- Verifica proximidade com qualquer uma das 3 bases (RSSI > -80dBm)
- Conecta automaticamente na primeira base disponível
- Funciona com todas as bases configuradas

### Upload Automático

- Detecta quando está próximo de uma base
- Conecta e sincroniza horário via NTP
- Envia dados para Firebase Realtime Database
- Limpa arquivos após upload bem-sucedido

### Formato de Dados

**Formato compacto salvo localmente:**
```json
[timestamp, realTime, batteryLevel, isCharging, [["SSID","BSSID",rssi,channel]]]
```

**Estrutura no Firebase:**
```json
{
  "bikes": {
    "sl01": {
      "test": {
        "bike": "sl01",
        "time": 1640995200,
        "test": true
      }
    }
  }
}
```

## Interface Web

### Página Inicial
- **Configurações**: Alterar parâmetros do sistema
- **Ver WiFi**: Redes detectadas em tempo real
- **Ver Dados**: Arquivos salvos localmente

### Configurações Seguras
- Alterações via web preservam dados coletados
- Apenas `uploadfs` apaga dados (cuidado!)

## Hardware

### Componentes Suportados
- NodeMCU ESP8266
- ESP8266 genérico

### Conexões Opcionais (não testado)
- **A0**: Divisor de tensão para monitoramento de bateria
  - Bateria+ → Resistor 100kΩ → A0 → Resistor 33kΩ → GND
  - Permite leitura de 0-4.2V (bateria Li-ion)

## Backup de Dados

### Exportação Manual
1. Digite `m` → `6` no menu serial
2. Copie dados entre "INICIO" e "FIM"
3. Cole em arquivo `.json` para backup

### Antes de `uploadfs`
⚠️ **IMPORTANTE**: `uploadfs` apaga todos os dados coletados!
- Sempre faça backup antes de `uploadfs`
- Para mudanças de código: use apenas `upload`

## Troubleshooting

### Problemas Comuns

1. **Modo AP não ativa**:
   - Verifique se botão FLASH está sendo pressionado durante boot
   - Use menu serial: `m` → `7`

2. **Não detecta bases**:
   - Verifique SSID/senha em `bases.txt`
   - Aproxime mais do roteador (RSSI > -80dBm)

3. **Não faz upload**:
   - Teste conexão: menu `m` → `3`
   - Verifique configurações Firebase

4. **Bateria sempre 0%**:
   - Verifique conexão do divisor de tensão no pino A0

### Logs Importantes

```
=== Bike sl01 - 15 redes - Bat: 85.5% ===
Base1 encontrada: VALENCA (RSSI: -73)
Conectado à base VALENCA!
IP obtido: 192.168.1.100
=== UPLOAD FIREBASE ===
Upload OK!
```

## Desenvolvimento

### Estrutura do Projeto
```
wifi-range/
├── data/              # Configurações (uploadfs)
├── data-example/      # Templates de configuração
├── src/main.cpp       # Código principal
└── platformio.ini     # Configuração do projeto
```

### Comandos Úteis
```bash
# Compilar apenas
pio run

# Upload forçando porta
pio run --target upload --upload-port /dev/ttyUSB0

# Monitor com filtros
pio device monitor --baud 115200 --filter esp8266_exception_decoder
```