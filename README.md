# Controle de resistência com termistor (Arduino)

Sketch em `controle_resistencia/controle_resistencia.ino` que:

- lê a temperatura de um termistor NTC e imprime no Monitor Serial (9600 baud) a cada 1 s;
- mantém a resistência em **100 °C** com controle liga/desliga (histerese de ±1 °C);
- desliga a resistência se o termistor estiver desconectado/em curto ou se passar de 120 °C.

## Ligações

| Componente | Ligação |
|---|---|
| NTC 10k | entre 5V e A0 |
| Resistor 10k | entre A0 e GND |
| Relé / SSR / MOSFET | sinal no pino 8 |

A resistência deve ser acionada por um relé, SSR ou MOSFET adequado à corrente e tensão dela — **nunca** direto no pino do Arduino.

## Ajustes

No início do sketch:

- `COEF_BETA`, `R_NOMINAL`: valores do datasheet do seu termistor (padrão: 10k, B=3950);
- `SETPOINT`, `HISTERESE`, `TEMP_SEGURANCA`;
- `RELE_ATIVO_EM_BAIXO = true` se o seu módulo de relé aciona com nível baixo.

Exemplo de saída:

```
Temperatura: 97.8 C | Setpoint: 100.0 C | Resistencia: LIGADA
```
