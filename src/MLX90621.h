#ifndef MLX90621_H

#define MLX90621_H

void readEEPROM();
void writeConfiguration();
uint16_t readConfiguration();
void   writeOSCTrim();
void calculateConstants();
void readPTAT();
void readIR();
float getAmbient();
void readCPIX();
void calculateTA(void);
void calculateTO();
void I2Cscan();
void writeWord(uint8_t address, uint8_t subAddress, uint16_t data);
uint16_t readWord(uint8_t address, uint8_t subAddress);
void writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
uint8_t readByte(uint8_t address, uint8_t subAddress);
void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest);


void MLX90621Setup();
void MLX90621GetImage();

#endif