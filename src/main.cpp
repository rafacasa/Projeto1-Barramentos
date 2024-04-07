#include <Arduino.h>

#include "Crc16.h"    //Biblioteca de cálculo do CRC16
Crc16 crc;            // variavel Crc

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



// Variáveis Globais
byte receivedData[20]; // Salva o quadro Modbus recebido
bool broadcast;        // Informa se o último quadro recebido foi um broadcast ou não
//TODO Lembrar de testar se eh broadcast em todas as funcoes de envio de respostas (excessao ou nao)

// put function declarations here:
bool quadroModbusDisponivel();
bool checaEnderecoQuadro();
uint8_t executaSolicitacao();
int lerQuadroModbus();

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);
}

void loop() {
  int funcao_solicitada;
  // Verifica se há um quadro modbus disponivel
  if (quadroModbusDisponivel()) {
    funcao_solicitada = lerQuadroModbus();
    
  }
}

// Função que verifica se há um quadro modbus disponivel para ser lido na porta Serial
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

// Função que lê o quadro Modbus recebido e verifica erros de crc, endereço e função
// Retorna a indice da funcao solicitada ou -1 caso tenha algum erro
int lerQuadroModbus() {
  uint16_t bytesRecebidos, valueCrc;
  uint8_t codigoExcessao;

  // Obtem o tamanho do quadro
  bytesRecebidos = Serial.available();

  // copia os bytes para o vetor recebido  
  Serial.readBytes(receivedData, bytesRecebidos);

  // calcula o CRC16 dos bytes recebidos, incluindo o proprio CRC recebido
  // se o valor calculado == 0, indica que o quadro esta ok
  valueCrc = crc.Modbus(receivedData, 0, bytesRecebidos);

  // Encerra execução quando há erros de crc no quadro
  if (valueCrc != 0) {
    return -1;
  }

  // Encerra execução quando o endereço do quadro não é este dispositivo
  if (!checaEnderecoQuadro()) {
    return -1;
  }

  // Tenta executar a solicitação e obtem o código de excessão caso não foi possível
  codigoExcessao = executaSolicitacao();

  switch (codigoExcessao) {
    case 1:
      // TODO Enviar excessão funcao não suportada
      break;
    case 2:
      // TODO Enviar excessão endereco invalido
      break;
    case 3:
      // TODO Enviar excessao dados do registrador invalidos
      break;
    case 0:
      // TODO Enviar resposta de confirmacao da execucao
      break;
    default:
      break;
  }

}

// Função que checa o endereço do comando Modbus e retorna True caso seja o destinatário
bool checaEnderecoQuadro() {
  uint8_t endereco_recebido = receivedData[0];

  // Checa o endereço de broadcast
  if (endereco_recebido == 0x00) {
    broadcast = true;
    return true;
  }

  return endereco_recebido == ENDERECO_ESCRAVO;
}

// Executa a solicitação enviada, caso seja possível.
// Retorna 0 caso a solicitação foi executada com sucesso
// Retorna o código de excessão caso houve algum erro:
//  1 - Funcao nao suportada
//  2 - Endereço inválido
//  3 - Dados do registrador inválidos
uint8_t executaSolicitacao() {
  uint8_t funcao_solicitada = receivedData[1];

  switch (funcao_solicitada) {
    case 0x10: // Write Multiple Registers
      return executaWriteMultipleRegisters();
      break;
    
    default: // Função não suportada
      return 1;
      break;
  }
}

uint8_t executaWriteMultipleRegisters() {
  uint16_t quantidade_registradores, endereco_inicial;
  uint8_t contagem_bytes;

  // Obtêm o campo Quantidade de resgistradores do quadro Modbus recebido
  quantidade_registradores = receivedData[4]; // MSB
  quantidade_registradores <<= 8;
  quantidade_registradores += receivedData[5] & 0xff; // LSB

  // Verifica se a quantidade de resgistradores sendo alterados pela solicitação. Gera a excessão 3.
  if (quantidade_registradores < 1 || quantidade_registradores > 8) {
    return 3;
  }

  // Obtêm o campo Contagem de bytes do quadro Modbus recebido
  contagem_bytes = receivedData[6];

  // Verifica se a quantidade de bytes informada no quadro Modbus é compativel com a solicitacao
  // 2 * quantidade de registradores a serem alterados
  // Gera a excessão 3
  if (contagem_bytes != quantidade_registradores * 2) {
    return 3;
  }

  // Obtêm o campo Endereço do primeiro registrador do quadro Modbus recebido
  endereco_inicial = receivedData[2]; // MSB
  endereco_inicial <<= 8;
  endereco_inicial += receivedData[3] & 0xff; // LSB

  // Verifica se o endereço inicial está entre os endereços disponíveis
  // Gera a excessão 2
  if (endereco_inicial < 0x0010 || endereco_inicial > 0x0017) {
    return 2;
  }

  // Verifica se todos os endereços de registradoes são disponíveis
  // Gera a excessão 2
  if (endereco_inicial + quantidade_registradores > 0x0017) {
    return 2;
  }

  // TODO Checar se os valores a serem escritos estão entre 0 e 1023 - Excessao 4

  // TODO Escrever os valores nos registradores (Criar funcao que recebe o endereco e o valor e ela escreve no registrador)
}