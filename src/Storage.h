#ifndef STORAGE_H
#define STORAGE_H
void StorageWriteFile(const char * path,const uint8_t * buff, size_t size);
void StorageSetup();
void StorageTest();
#endif