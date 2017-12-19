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
// final code
// timer 
struct timeNode{
  float timeStamp; // time at which the packet was sent to layer 3
  int seqnum; // seq num of the packet
};

// Global variables for A
int Abase; // seq num of first unacknowledged packet
int next_seq_num; // next available seq num
struct msg msgBuf[1000]; // implement buffer
int msgBufStart; // start index of messages to be sent
int msgBufEnd; // next available space in message buffer
struct pkt sentBuf[1000]; // buffer to store packets for retransmissions
int windowSize;
int globalTimeOut; // default time out value
int seqArrA[1001]; // array to keep track of unacknowledged packets

// Global varaibles for B
int exp_seq_num; 
struct pkt ack; // acknowledgment packet
struct msg dummy; // dummy message for default acknowledgement
struct pkt recvBuf[1000]; // buffer out of order packets
int Bbase; // base seq num for B
int seqArrB[1001]; // array to keep track of acknowledged packets

struct timeNode timerArr[1000]; // array to store timeouts for all packets // cyclic buffer
int timerStart; // points to the current running timer
int timerEnd; // points to next available space in timerArr

// create a time node and insert into timerArr
// time node contains the time at which packet was sent and seqnum of packet
void insertTimeNode(int seqnum){
  timerArr[timerEnd].timeStamp = get_sim_time();
  timerArr[timerEnd].seqnum = seqnum;
  timerEnd++;
  // for cyclic array
  if(timerEnd == 1000){
    timerEnd = 0;
  }
}

// adaptive timeout scheme
// isTimeOut - implement adaptive timeout only for acknowledgments
int getTimeOut(int isTimeOut){
  int timeOut;
  int seqnum = timerArr[timerStart].seqnum;
  float curSimTime = get_sim_time();

  int sampleRTT;
  int estTimeOut;

  if(isTimeOut == 0){
    // timerStart points to the packet just received
    sampleRTT = (int)(curSimTime - timerArr[timerStart].timeStamp);
    estTimeOut = (int)((0.2 * (float)globalTimeOut) + (0.8 * (float)sampleRTT));
    // add some margin value to estimated timeout
    globalTimeOut = estTimeOut + 5;
  }

  // find the timeout value for next unacknowledged packet
  while(timerStart != timerEnd && seqArrA[seqnum] == 1){
    timerStart++;
    if(timerStart == 1000){
      timerStart = 0;
    }
    seqnum = timerArr[timerStart].seqnum;
  }

  printf("globalTimeOut:%d\n", globalTimeOut);
  // timerStart points to unacknowledged packet
  if(timerStart == timerEnd){ // if no unacknowledged packet
    return 0;
  }else{
    printf("sim time:%f\n", curSimTime);
    printf("time stamp:%f\n", timerArr[timerStart].timeStamp);
    // get the new timeout value using the current simulation time and timestamp of the unacknowledged node
    timeOut = globalTimeOut - (int)(curSimTime - timerArr[timerStart].timeStamp);
    
    printf("timeout:%d for %d\n", timeOut, seqnum);
    
    if(timeOut <= 0){
      /* this is the case when because of our adaptive timeout calculation, 
      the current packet is already timedout, we give a buffer amount to it and add deviation amount to
      the globalTimeOut
      */
      globalTimeOut = globalTimeOut + 1;
      timeOut = 1;
    }
    return timeOut;
  }
  
}

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
    ack.acknum = exp_seq_num;
    ack.checksum = get_checksum(ack);
  }
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  printf("A_output:%d\n", next_seq_num);
  if(next_seq_num < Abase+windowSize){
    make_packet(message, 0);
    tolayer3(0, sentBuf[next_seq_num]);

    // start timer for each node
    // either start or insert into timerArr
    insertTimeNode(next_seq_num);

    if((timerStart+1 == 1000 && timerEnd == 0 ) || timerStart + 1 == timerEnd){ // start timer if the new node is the only node
    //if(base == next_seq_num){
      printf("starting timer for:%d\n", next_seq_num);
      starttimer(0, globalTimeOut); // check
    }
    next_seq_num++;
  }else{
    if(msgBufEnd < 1000){
      msgBuf[msgBufEnd] = message;
      msgBufEnd++;
    }
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  printf("A_input:%d\n", packet.acknum);
  int timeOut;

  int checksum = get_checksum(packet);

  // check within window
  if(checksum == packet.checksum && seqArrA[packet.acknum] != 1){
    // if packet is correct, stop current timer
    seqArrA[packet.acknum] = 1; // acknum is the seq num of packet

    if(packet.acknum == Abase){ // current timer
      stoptimer(0);
      // start timer for next 
      // check if there is any other packet
      if(timerStart != timerEnd){
        timeOut = getTimeOut(0);
        printf("timeout:%d\n", timeOut);
        if(timeOut != 0){
          starttimer(0, timeOut);
        }
      }
    }

    // move the base to increase window size
    if(Abase = packet.acknum){
      while(seqArrA[Abase] == 1){
        Abase++;
      }
    }

    // check untransmitted messages
    while(msgBufEnd > msgBufStart && next_seq_num < Abase+windowSize){
      A_output(msgBuf[msgBufStart]);
      msgBufStart++;
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  // reaches here when node at timerStart timed out
  // resend packet and insert into the timer arr
  int timeOut;

  int curr = timerArr[timerStart].seqnum;
  tolayer3(0, sentBuf[curr]); // send pkt again 

  // reset timer for current node
  timerArr[timerStart].timeStamp = get_sim_time();
  // add this to the end of the list, since the timerArr should be sorted wrt timeout
  timerArr[timerEnd] = timerArr[timerStart];
  timerStart++;
  timerEnd++;
  // cyclic buffer
  if(timerStart == 1000){
    timerStart = 0;
  }
  if(timerEnd == 1000){
    timerEnd = 0;
  }

  // start timer for the smallest unack node
  timeOut = getTimeOut(1);
  if(timeOut != 0){ 
    starttimer(0, timeOut);
  }
  printf("inter\n");
    
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  //printf("A_init\n");
  Abase = 1;
  next_seq_num = 1;

  msgBufStart = 0;
  msgBufEnd = 0;

  windowSize = getwinsize();
  globalTimeOut = 20;

  timerStart = 0;
  timerEnd = 0;
  //printf("A_init_end\n");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  // valid packets
  printf("B_input:%d\n", packet.seqnum);
  int checksum = get_checksum(packet);

  if(packet.seqnum <= 1000 && (packet.seqnum < Bbase) && checksum == packet.checksum){ // acknowledge already received data
    exp_seq_num = packet.seqnum;
    make_packet(dummy, 1);
    tolayer3(1, ack);
  }else if(packet.seqnum <= 1000 && packet.seqnum >= Bbase && packet.seqnum < Bbase+windowSize && checksum == packet.checksum){
      // prepare acknowledgment
    exp_seq_num = packet.seqnum; // variable used in make_packet
    make_packet(dummy, 1);
    tolayer3(1, ack);
    if(seqArrB[packet.seqnum] != 1){
      seqArrB[packet.seqnum] = 1; // it has been received correctly
      recvBuf[packet.seqnum] = packet; // buffer it

      // check if data received completes some gap
      if(packet.seqnum == Bbase){ // gap is completed when base seq num 
        printf("delivering to layer 5");
        while(seqArrB[Bbase] != 0){
          tolayer5(1, recvBuf[Bbase].payload);
          Bbase++;
        }
      }
    }
    
  }
  //printf("B_input_end\n");
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  Bbase = 1;
  exp_seq_num = 1;
  make_packet(dummy, 1);
  windowSize = getwinsize();
  //printf("B_init\n");
}
