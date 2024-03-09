#include <Arduino.h>

// Constantes do projeto
#define BAUDRATE 9600 //Taxa de comunicação serial
#define ENDERECO_ESCRAVO 0x01 //Endereço deste escravo na rede Modbus
#define RS_485_ENABLE_PIN 2  //pino que ativa e desativa o transmissor rs485

/* Tempo de fim de frame Modbus RTU em ms          
 * ( 3.5 byte * 11 bits/byte (8-N-2) * 1000 ms/s ) / 9600 bits/s ~= 4 ms
 */
#define END_FRAME_TIME (3.5 * 11 * 1000) / BAUDRATE + 1 

//TODO define com os endereços dos registradores modbus para as saidas

//TODO define para os pinos utilizados para a comunicação com o módulo


//TODO Fazer interrupcoes por timer para ler botoes e atualizar o display
//TODO No loop rodar funcoes para esperar comunicações modbus

// put function declarations here:
bool quadroModbusDisponivel();

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
bool quadroModbusDisponivel() {
  long milisegundos;      // Variável para guardar tempo desde ultima recepcáo de bytes
  int bytesAvailable;     // Variável para guardar a quantidade de bytes do quadro modbus
  bool endFrame = false;  // flag que indica que o quadro chegou

  if (Serial.available() > 0) {
    bytesAvailable = Serial.available(); // Quantos bytes estão disponíveis
    milisegundos = millis(); // Tempo atual

    // O quadro Modbus RTU termina quando duas condicoes sao satisfeitas:
    // 1. nao sao recebidos mais bytes e
    // 2. o tempo decorrido sem recepcao supera 3.5 tempos de byte
    
    while (!endFrame) {
      // Se aumentou a quantidade de bytes disponíveis - chegou um byte novo
      // Entao se reseta o tempo da última mensagem
      if (Serial.available() != bytesAvailable) {
        bytesAvailable = Serial.available();
        milisegundos = millis();
      } else {
        // Se o tempo desde a ultima mensagem é maior que o END TIME - o quadro foi recebido
        if ((millis() - milisegundos) > END_FRAME_TIME) {
          endFrame = true;
        }
      }
    }
  }
  return endFrame;
}