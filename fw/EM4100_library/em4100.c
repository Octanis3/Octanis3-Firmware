/*
 * em4100.c
 *
 *  Created on: 20 Nov 2017
 *      Author: raffael
 *      Inspired by https://github.com/Helmars/Arduino_RFID/blob/master/RFID_reader/RFID_reader.ino
 */

#include "../Board.h"

void wait(uint8_t t){
  uint8_t counter=0,last=0,next;
  do {
    next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
  } while (counter<t);
}

uint8_t em4100_read_bit(uint8_t time1,uint8_t time2){
  wait(time1);
  uint8_t counter=0,last=0,state1=RFID_PIN&RFID_PIN_MASK,state2;
  do {
    uint8_t next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
    state2=RFID_PIN&RFID_PIN_MASK;
  } while (counter<=time2 && state1==state2);
  if (state1==state2) return 2;
  if (state2==0) return 0; else return 1;
}

int8_t read4Bits(uint8_t time1,uint8_t time2){
  uint8_t number=0,parity=0;
  for (uint8_t i=4;i>0;i--){
    uint8_t bit=readBit(time1,time2);
    if (bit==2) return -1;
    number=(number<<1)+bit;
    parity+=bit;
  }
  uint8_t bit=readBit(time1,time2);
  if (bit==2 || (parity&1)!=bit) return -1;
  cparity^=number;
  return number;
}

uint8_t readCard(){

  cparity=0;
  for (uint8_t i=0;i<5;i++){
    int8_t n1=read4Bits(halfbit+offset,halfbit),
           n2=read4Bits(halfbit+offset,halfbit);
    if (n1<0 || n2<0) return 1;
    numbers[i]=(n1<<4)+n2;
  }
  uint8_t cp=0;
  for (uint8_t i=4;i>0;i--){
    uint8_t bit=readBit(halfbit+offset,halfbit);
    if (bit==2) return 1;
    cp=(cp<<1)+bit;
  }
  if (cparity!=cp) return 1;
  if (readBit(halfbit+offset,halfbit)!=0) return 1;
  return 0;
}
