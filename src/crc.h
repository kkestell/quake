/* crc.h */

void CRC_Init(uint16_t *crcvalue);
void CRC_ProcessByte(uint16_t *crcvalue, uint8_t data);
uint16_t CRC_Value(uint16_t crcvalue);
