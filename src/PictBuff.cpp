unsigned char jpg_pict_buff[1024];
int jpg_pict_buff_index=0;
void write_jpg_pict_buff(unsigned char byte) {
  jpg_pict_buff[jpg_pict_buff_index++]=byte;
  //Serial.print(byte);
}
void reset_jpg_pict_buff() {
  jpg_pict_buff_index=0;
}
int get_jpg_size() {
  return jpg_pict_buff_index;
}

void *get_jpg_pict_buff() {
  return jpg_pict_buff;
}