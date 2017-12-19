#include "../include/simulator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

// global variable for A
static int seq_num;
struct pkt* APacket;
int isTxnComp;
struct msg msgBuf[1000]; // implement cyclic buffer
int msgBufStart; // check and change to 0 when buf end reached
int msgBufEnd;

// global variable for B
static int ack_num;
struct pkt* ack;

// common methods

int sumtotal(int x, int y){
  int carry = 0;
  while(y != 0){
      carry = x & y;
      x = x ^ y;
      y = carry << 1;
  }

  return x;
}

int get_checksum(struct pkt packet){
  int sum = 0;
  int carry = 0;
  int op2 = 0;
  for(int i=0; i<20; i++){
    op2 = packet.payload[i];
    sum = sumtotal(sum, op2);
  }

  op2 = packet.seqnum;
  sum = sumtotal(sum, op2);

  op2 = packet.acknum;
  sum = sumtotal(sum, op2);

  // take ones complement
  sum = ~sum;

  return sum;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  // if no message is in transition
  printf("a_output\n");
  if(isTxnComp == 1){
    memset(APacket->payload, 0, 20);
    strcpy(APacket->payload, message.data);
    APacket->seqnum = seq_num;
    APacket->acknum = 0;
    APacket->checksum = get_checksum(*APacket);
    // send and start the timer
    tolayer3(0, *APacket);
    starttimer(0, 13);
    seq_num = ~seq_num;
    isTxnComp = 0;
  }else{
    printf("buffereing\n");
    msgBuf[msgBufEnd] = message;
    msgBufEnd++;
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  printf("a_input\n");
  stoptimer(0);
  int checksum = get_checksum(packet);

  // if acknum is same as current seq_num it is old ack ie current packet was not delivered
  if(checksum == packet.checksum && packet.acknum != seq_num){
    isTxnComp = 1;
    if(msgBufEnd > msgBufStart){
      printf("buffered\n");
      A_output(msgBuf[msgBufStart]);
      msgBufStart++;
    }
  }else{
    isTxnComp = 0;
    tolayer3(0, *APacket);
    starttimer(0, 13);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("timer_interrrupt\n");
  isTxnComp = 0;
  tolayer3(0, *APacket);
  starttimer(0,13);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  seq_num = 0;
  isTxnComp = 1;
  msgBufStart = 0;
  msgBufEnd = 0;
  APacket = (struct pkt*)calloc(1, sizeof(struct pkt));
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  int checksum = get_checksum(packet);

  // check duplicate packets as well
  if(checksum == packet.checksum && packet.seqnum != ack_num){

    // data is correct, deliver to layer5
    tolayer5(1, packet.payload);

    // send acknowledgment
    memset(ack->payload, 0, 20);
    ack->seqnum = 0;
    ack->acknum = packet.seqnum;
    ack->checksum = get_checksum(*ack);

    // set ack num 
    ack_num = ack->acknum;

    // send ack
    tolayer3(1, *ack);

  }else{
    // send old acknowledgement
    tolayer3(1, *ack);
  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  ack_num = 1;
  ack = (struct pkt*)calloc(1,sizeof(struct pkt));
}




