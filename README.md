# estetoscopio-esp32

Estetoscópio digital experimental: um ESP32-C3 Super Mini amostra o sinal de um microfone de ausculta e transmite os valores crus por BLE; um app web (Chrome/Android, Web Bluetooth) faz toda a filtragem e análise em tempo real.

> **Aviso:** projeto acadêmico (Engenharia Elétrica, UFMG). Não é dispositivo médico e não deve ser usado para diagnóstico.

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `index.html` | App web (arquivo único). Publicado via GitHub Pages. |
| `firmware/esp32c3_ausculta.ino` | Firmware mínimo: amostra o ADC e envia por BLE. |
| `sim/*.wav` | Gravações reais (4 kHz, banda 20–1000 Hz) usadas pelo botão **Simular**. |

## Como usar o app

1. Abra a página publicada no **Chrome** do Android (Firefox e Samsung Internet não suportam Web Bluetooth).
2. Menu ⋮ → *Adicionar à tela inicial* para usar como app.
3. Selecione a taxa de amostragem igual à do firmware (padrão 4 kHz) e toque em **Conectar BLE** → `Ausculta-C3`.
4. Escolha o modo (Respiração / Batimento), a posição no tórax e **Iniciar medição**.
5. Sem o ESP, o botão **Simular** gera um sinal sintético (coração + pulmão sobrepostos) para testar a interface.

## Divisão de responsabilidades

**ESP32-C3** — apenas:
- lê o ADC (12 bits, ADC1) a `FS` Hz com período fixo;
- empacota 120 amostras + contador de sequência e envia por notificação BLE (MTU 247).

**App** — tudo o resto:
- filtro Butterworth 4ª ordem por modo: 100–1000 Hz (respiração) ou 20–150 Hz (batimento);
- envelope RMS (50 Hz), detecção de ciclos, frequência respiratória/cardíaca, razão I:E, regularidade do ritmo;
- fonocardiograma, espectrograma do sinal bruto (até FS/2), diagnóstico do link (amostras/s, perdas, saturação do ADC);
- sessões cronometradas com posição anatômica, exportação CSV e WAV do sinal bruto.

## Protocolo BLE

| Item | Valor |
|---|---|
| Nome do dispositivo | `Ausculta-C3` |
| Serviço | `0000ff00-0000-1000-8000-00805f9b34fb` |
| Característica DATA (Notify) | `0000ff01-0000-1000-8000-00805f9b34fb` |
| Pacote (little-endian) | `seq uint16` + `N × uint16` (ADC cru 0–4095), N = (len − 2)/2 |
| Taxa | fixa no firmware (`FS`, padrão 4000 Hz); selecionável no app |

## Hardware (condicionamento analógico)

- Microfone → amplificador → passa-alta ≈ 20 Hz → passa-baixa ≈ 1 kHz → ADC.
- Polarizar o sinal em ≈ 1,25 V e limitar a 0–2,5 V (atenuação 11 dB do ADC).
- Usar apenas pinos do ADC1 (GPIO0–4); o ADC2 não funciona com o rádio ativo.
- O campo "saturações" no app indica ganho excessivo.

## Firmware

Arduino IDE, placa *ESP32C3 Dev Module* (USB CDC On Boot: Enabled), biblioteca **NimBLE-Arduino**. Ajuste `ADC_PIN` e `FS` em `firmware/esp32c3_ausculta.ino`.

## Gravações de simulação (fontes)

- `sim/heart_normal_61bpm.wav` — PhysioNet/CinC Challenge 2016, registro `a0007` (classificado normal). Liu C. et al., "An open access database for the evaluation of heart sound algorithms", *Physiol. Meas.* 37(12), 2016. Dados abertos (PhysioNet, ODC-By).
- `sim/lung_normal_21rpm.wav` — ICBHI 2017 Respiratory Sound Database, registro `152_1b1_Al_sc_Meditron` (sujeito saudável, campo anterior esquerdo). Rocha B.M. et al., "An open access database for the evaluation of respiratory sound classification algorithms", *Physiol. Meas.* 40(3), 2019. Uso para fins de pesquisa/educação com atribuição.

Ambas reamostradas para 4 kHz e limitadas a 20–1000 Hz para reproduzir o que sai do condicionamento analógico.
