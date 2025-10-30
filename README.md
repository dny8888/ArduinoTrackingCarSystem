# 🚗 Rastreador Veicular com Arduino, SIM808 e MPU6050

## 📖 Descrição Geral

Este projeto implementa um **rastreador veicular autônomo** baseado em **Arduino**, que utiliza o módulo **SIM808** (GSM + GPS) e o sensor **MPU-6050** (acelerômetro e giroscópio).  
O sistema permite:

- Envio da **localização GPS** via **SMS**;  
- Rastreamento automático em intervalos configuráveis;  
- Detecção de **movimentação** via acelerômetro;  
- Configuração e controle remoto através de **mensagens SMS**.

O rastreador é ideal para aplicações de monitoramento de veículos, frotas ou objetos móveis, com comunicação via rede GSM.

---

## ⚙️ Componentes Utilizados

| Componente | Função | Pinos Utilizados |
|-------------|---------|------------------|
| **Arduino UNO / Nano** | Microcontrolador principal | - |
| **SIM808 (GSM + GPS)** | Comunicação celular e localização | RX: 2 / TX: 3 / RST: 4 |
| **MPU-6050** | Sensor de aceleração e movimento | I²C (SDA/SCL) |
| **LED (OK_LED)** | Indicação de setup concluído | Pino 8 |

---

## 🧩 Funcionalidades Principais

| Função | Descrição |
|--------|------------|
| **Rastreamento Automático** | Envia localização em intervalos configuráveis via SMS. |
| **Localização sob Demanda** | Retorna a posição GPS ao receber o comando `L` via SMS. |
| **Ativar/Desativar Rastreamento** | Controlado pelo comando `R`. |
| **Configuração de Intervalo de Rastreamento** | Ajustável com o comando `Tn` (onde *n* = 1 a 9 minutos). |
| **Detecção de Movimento** | Envia localização se o veículo for movido (MPU-6050). |
| **Menu de Ajuda** | Exibido ao receber um comando inválido. |

---

## 📡 Comandos por SMS

| Comando | Ação | Exemplo |
|----------|------|----------|
| `R` | Ativa/desativa o rastreamento automático. | `R` |
| `L` | Solicita a localização atual. | `L` |
| `Tn` | Define o intervalo de envio (minutos). | `T5` (a cada 5 minutos) |
| (outros) | Mostra o menu principal com instruções. | `?` |

---

## 🧠 Estrutura de Dados

### Estrutura `GPSData`
Armazena as coordenadas obtidas do GPS.

```c
typedef struct gpsData {
  float latitude;
  float longitude;
} GPSData;
````

### Estrutura `Message`

Representa uma mensagem SMS recebida.

```c
typedef struct message {
  int8_t index;
  char number[15];
  char text[70];
  uint16_t *lenght;
} Message;
```

---

## 🔩 Fluxo de Execução

### `setup()`

1. Inicializa a comunicação serial e o módulo SIM808.
2. Ativa o GPS e aguarda o primeiro sinal.
3. Inicializa o sensor MPU-6050.
4. Configura notificações automáticas de SMS (`AT+CNMI=2,1`).
5. Acende o LED de confirmação (pino 8).

### `loop()`

1. Verifica se o intervalo de rastreamento foi atingido.
2. Lê novas mensagens SMS e interpreta os comandos.
3. Detecta movimento via acelerômetro.
4. Envia a localização conforme os gatilhos:

   * Tempo decorrido;
   * Movimento detectado;
   * Solicitação via SMS.

---

## 🧭 Lógica de Rastreamento

| Gatilho                    | Condição                            | Ação                       |
| -------------------------- | ----------------------------------- | -------------------------- |
| **Intervalo de tempo**     | Rastreamento ativo e tempo excedido | Envia localização          |
| **Movimentação detectada** | Acelerômetro > 1.5g                 | Envia localização imediata |
| **Comando SMS `L`**        | Recebido via SMS                    | Envia localização          |
| **Comando SMS `R`**        | Alterna o modo de rastreamento      | Liga/Desliga rastreamento  |

---

## 🔌 Conexões e Pinos

| Componente  | Arduino | Descrição                    |
| ----------- | ------- | ---------------------------- |
| SIM808 RX   | 3       | Transmissão serial           |
| SIM808 TX   | 2       | Recepção serial              |
| SIM808 RST  | 4       | Reset do módulo              |
| MPU6050 SDA | A4      | Comunicação I²C              |
| MPU6050 SCL | A5      | Comunicação I²C              |
| LED         | 8       | Indicação de setup concluído |

---

## 🧰 Funções Principais

| Função                             | Descrição                                             |
| ---------------------------------- | ----------------------------------------------------- |
| `deleteAllSMS()`                   | Apaga todas as mensagens SMS armazenadas no SIM card. |
| `getSMS(Message *msg)`             | Lê e armazena o conteúdo de uma SMS recebida.         |
| `sendActualLocation(char *sendTo)` | Obtém a posição GPS e envia via SMS.                  |
| `getAccMove()`                     | Detecta movimentação a partir do acelerômetro.        |

---

## 📤 Formato da Mensagem de Localização

Exemplo de mensagem SMS enviada:

```
Latitude: -25.4284
Longitude: -49.2733
http://maps.google.com/maps?q=-25.4284,-49.2733
```

---

## 🧪 Exemplo de Execução no Monitor Serial

```
Rastreador
Iniciando..(Pode levar alguns segundos)
SIM808 OK
Testando GPS.
Aguardando sinal dos satélites...
Setup Finalizado.
delayTrigger: 1
setTrackingTrigger: 1
smsTrigger: 0
accTrigger: 0
trackingDelayMin: 10
```

---

## ⚠️ Observações

* O módulo **SIM808** requer uma antena GSM e GPS conectadas corretamente.
* O chip SIM deve estar ativo e possuir créditos para envio de SMS.
* O sensor **MPU-6050** deve estar firmemente fixado para medições precisas.
* Utilize fonte de alimentação estável (mínimo 2A) para o módulo SIM808.

---

## 🧾 Licença

Este projeto é distribuído sob a licença **MIT**, permitindo uso, modificação e distribuição livre, desde que o crédito ao autor original seja mantido.

---

## 👤 Autor

**Daniel Belmiro Pereira**
📧 [LinkedIn](https://www.linkedin.com/in/daniel-belmiro-pereira/)

---

