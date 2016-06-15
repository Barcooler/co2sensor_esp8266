// Based on https://github.com/fe-c/MT8060-data-read code
// All rights for reading code owned https://geektimes.ru/users/fedorro/
// and https://github.com/revspace

#include "mt8060_decoder.h"
#define MT8060_MAX_MS  2 // Таймаут по которому считаем, что началось новое сообщение
#define MT8060_MSG_LEN 5 // В одном полном сообщении 5 байт
#define MT8060_MSG_TYPE_BYTE_IDX          0
#define MT8060_MSG_VAL_HIGH_BYTE_IDX      1
#define MT8060_MSG_VAL_LOW_BYTE_IDX       2
#define MT8060_MSG_CHECKSUM_BYTE_IDX      3
#define MT8060_MSG_CR_BYTE_IDX            4
#define BITS_IN_BYTE                      8

static uint8_t buffer[MT8060_MSG_LEN]; // Буфер для хранения считанных данных
static int num_bits = 0;
static unsigned long prev_ms;

static mt8060_message _msg;
static mt8060_message *msg = &_msg;


void mt8060_decode(void) // Декодирует сообщение
{

  uint8_t checksum = buffer[MT8060_MSG_TYPE_BYTE_IDX] + buffer[MT8060_MSG_VAL_HIGH_BYTE_IDX] + buffer[MT8060_MSG_VAL_LOW_BYTE_IDX];   // Вычисление контрольной суммы
  msg->checksumIsValid = (checksum == buffer[MT8060_MSG_CHECKSUM_BYTE_IDX] && buffer[MT8060_MSG_CR_BYTE_IDX] == 0xD); // Проверка контрольной суммы
  if (!msg->checksumIsValid) {
    return;
  }
  msg->type = (dataType)buffer[MT8060_MSG_TYPE_BYTE_IDX]; // Получение типа показателя
  msg->value = buffer[MT8060_MSG_VAL_HIGH_BYTE_IDX] << BITS_IN_BYTE | buffer[MT8060_MSG_VAL_LOW_BYTE_IDX]; // Получение значения показателя
}


// Вызывается на каждый задний фронт тактового сигнала, возвращает ссылку на структуру сообщения, если оно полностью считано
mt8060_message* mt8060_process(unsigned long ms, bool data)
{
  if ((ms - prev_ms) > MT8060_MAX_MS) {
    num_bits = 0;
  }
  prev_ms = ms;

  if (num_bits < MT8060_MSG_LEN * BITS_IN_BYTE) {
    int idx = num_bits / BITS_IN_BYTE;
    buffer[idx] = (buffer[idx] << 1) | (data ? 1 : 0);
    num_bits++;

    if (num_bits == MT8060_MSG_LEN * BITS_IN_BYTE) {
      mt8060_decode(); //Декодируем сообщение
      return msg; //Возвращаем сообщение
    }
  }
  return nullptr; //Ничего не возвращаем, если сообщение не полное
}
