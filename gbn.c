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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
// Global variables for A
// final code
int base; // keeps track of minimum unacknowledged packet
int next_seq_num; // next available sequence
struct msg msgBuf[1000]; // buffer for messages
int msgBufStart; // index of valid messages
int msgBufEnd; // next available index to buffer messages
struct pkt sentBuf[1000]; // store sent messages for retransmission
int windowSize; 
int timeout;

// Global varaibles for B
int exp_seq_num; 
struct pkt ack;
struct msg dummy; // dummy message to send initially

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

// isA tells whether it is sender or receiver
// for A saving packets in buffer for sent packets
// for B changing the acknum in acknowledgment packet
void make_packet(struct msg message, int isA){
  if(isA == 0){
    strcpy(sentBuf[next_seq_num].payload, message.data);
    sentBuf[next_seq_num].seqnum = next_seq_num;
    sentBuf[next_seq_num].acknum = 0;
    sentBuf[next_seq_num].checksum = get_checksum(sentBuf[next_seq_num]);
  }else{
    memset(ack.payload, 0, 20);
    ack.seqnum = 0;
    ack.acknum = exp_seq_num-1;
    ack.checksum = get_checksum(ack);
  }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  printf("A_output:%s\n", message.data);
  if(next_seq_num < base+windowSize){
    make_packet(message, 0);
    tolayer3(0, sentBuf[next_seq_num]);
    if(base == next_seq_num){
      starttimer(0, timeout);
    }
    next_seq_num++;
  }else{
    msgBuf[msgBufEnd] = message;
    msgBufEnd++;
  }
  printf("A_output_end\n");
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  printf("A_input:%d\n", packet.acknum);
  int checksum = get_checksum(packet);
  if(checksum == packet.checksum && packet.acknum >= base){
    base = packet.acknum+1;
    stoptimer(0);
    if(base != next_seq_num){
      starttimer(0, timeout);
    }
    while(msgBufEnd > msgBufStart && next_seq_num < base+windowSize){
      A_output(msgBuf[msgBufStart]);
      msgBufStart++;
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("interupt\n");
  
  // increase the timeout if number of packets to be retransmitted in more
  int tempTimeOut = 2*(next_seq_num - base);
  
  // if small number of packets being set, timeout should be a fixed value
  if(tempTimeOut < timeout){
    tempTimeOut = timeout;
  }
  starttimer(0, tempTimeOut);
  // resend all messages starting from base
  int i = base;
  while(i < next_seq_num){
    tolayer3(0, sentBuf[i]);
    i++;
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  base = 1;
  next_seq_num = 1;

  msgBufStart = 0;
  msgBufEnd = 0;

  windowSize = getwinsize();
  timeout = 16;
  printf("init\n");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  printf("B_input:%d\n", packet.seqnum);
  int checksum = get_checksum(packet);
  // if correct packet received, deliver to upper layer and send acknowledgement
  if(packet.seqnum == exp_seq_num && checksum == packet.checksum){
    tolayer5(1, packet.payload);
    exp_seq_num++;
    make_packet(dummy, 1);
    tolayer3(1, ack);
  }else if(checksum == packet.checksum){
    // send acknowledgement if some correct packet is received
    make_packet(dummy, 1);
    tolayer3(1, ack);
  }
  printf("B_input_end\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  exp_seq_num = 1;
  make_packet(dummy, 1);
  printf("B_init\n");
}
