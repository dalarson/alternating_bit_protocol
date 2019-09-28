#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include "project2.h"
 
/* ***************************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for unidirectional or bidirectional
   data transfer protocols from A to B and B to A.
   Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets may be delivered out of order.

   Compile as gcc -g project2.c student2.c -o p2
**********************************************************************/

/******************
 * STATE VARIABLES
 *****************/
enum state {
  AWAIT_SEND, AWAIT_ACK
} A_STATE;

int A_SEQ;
int B_SEQ;

struct pkt LAST_PACKET;
struct pkt PACKET_IN_QUEUE;


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 
 * The routines you will write are detailed below. As noted above, 
 * such procedures in real-life would be part of the operating system, 
 * and would be called by other procedures in the operating system.  
 * All these routines are in layer 4.
 */

/* 
 * A_output(message), where message is a structure of type msg, containing 
 * data to be sent to the B-side. This routine will be called whenever the 
 * upper layer at the sending side (A) has a message to send. It is the job 
 * of your protocol to insure that the data in such a message is delivered 
 * in-order, and correctly, to the receiving side upper layer.
 */
void A_output(struct msg message) {
  // check if we're ready to send
  if (A_STATE != AWAIT_SEND){
    // printf("Previous packet not ack'ed. Dropping.\n");
    return;
  }
  // printf("A seq: %d\tB seq: %d\n", A_SEQ, B_SEQ);
  // construct packet to be sent
  struct pkt packet;
  packet.seqnum = A_SEQ;
  memmove(packet.payload, message.data, MESSAGE_LENGTH);
  packet.checksum = get_checksum(&packet);

  // printf("Sending packet w seqnum: %d, payload: %s, checksum: %d, calculated checksum: %d\n", packet.seqnum, packet.payload, packet.checksum, get_checksum(&packet));

  // send packet
  tolayer3(AEntity, packet); 

  // store in case we need to retransmit
  LAST_PACKET = packet;

  // adjust state
  A_STATE = AWAIT_ACK;

  // begin timer
  startTimer(AEntity, 15);

  return;
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message)  {
}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) { // received an ack
  // verify checksum
  // printf("Ack received: Acknum: %d, checksum: %d, calculated checksum: %d\n", packet.acknum, packet.checksum, get_checksum(&packet));
  if (!validate_checksum(&packet)){
    // printf("ACK packet corrupted. Dropping.\n");
    return;
  }
  if (packet.acknum != A_SEQ){
    // printf("Received NAK. Dropping.\n");
    return;
  }

  // received valid ack

  // stop timer
  stopTimer(AEntity);

  // change seq num and state
  A_SEQ = flip_number(A_SEQ);
  A_STATE = AWAIT_SEND;

  
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt() {
  // printf("Timer interrupt at time %f, retransmitting.\n", getClockTime());
  // need to retransmit packet
  tolayer3(AEntity, LAST_PACKET);

  startTimer(AEntity, 15);
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  A_STATE = AWAIT_SEND;
  A_SEQ = 0;
}


/* 
 * Note that with simplex transfer from A-to-B, there is no routine  B_output() 
 */

/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt packet) {
  // printf("Receiving packet w seqnum: %d, payload: %s, checksum: %d, calculated checksum: %d\n", packet.seqnum, packet.payload, packet.checksum, get_checksum(&packet));


  // verify checksum
  if (!validate_checksum(&packet)){
    // printf("Packet corrupted. Dropping. %f\n", getClockTime());
    return;
  }

  // verify seq num
  if (packet.seqnum != B_SEQ){
    // printf("Received unexpected seqnum %d, expecting %d. Dropping.\n", packet.seqnum, B_SEQ);
    // send nak
    ack(BEntity, flip_number(B_SEQ));
    return;
  }

  // received valid packet
  // construct message to send up to layer 5
  struct msg message;
  memcpy(message.data, packet.payload, MESSAGE_LENGTH);

  // send to layer 5
  tolayer5(BEntity, message);

  // send ack
  ack(BEntity, B_SEQ); // TODO: update ack
  // flip the seq num
  B_SEQ = flip_number(B_SEQ);
  

}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void  B_timerinterrupt() {
}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init() {
  B_SEQ = 0;
}

/*****************************************
 HELPER FUNCTIONS
 ****************************************/


/*
 * Implementation of fletcher-16 checksum
 */
int get_checksum(struct pkt *packet){
  int c1 = 0, c2 = 0;

  // printf("BEFORE checksum: seq = %d, ack = %d\n", packet->seqnum, packet->acknum);

  c1 += packet->seqnum;
  c2 += c1;

  c1 += packet->acknum;
  c2 += c1;

  int i;
  for (i = 0; i < MESSAGE_LENGTH; i++){
    c1 += (int) packet->payload[i];
    c2 += c1;
  }

  int checksum = (c1 << 8) | c2;

  // printf("%d\n", checksum); // for testing
  // print_bin(checksum); // for testing

  // printf("After calculation: seq = %d, ack = %d, checksum = %d\n", packet->seqnum, packet->acknum, checksum);

  return checksum;

}

int validate_checksum(struct pkt *packet){
  return (packet->checksum == get_checksum(packet));
}

void ack(int AorB, int ack){
  struct pkt packet;
  packet.acknum = ack;
  packet.seqnum = 0;
  memcpy(packet.payload, "This pkt is an ack!!", MESSAGE_LENGTH);
  packet.checksum = get_checksum(&packet);
  tolayer3(AorB, packet);
}

int flip_number(int x){
  if (x == 1) return 0;
  else return 1;
}

void print_bin(int x){
  int i; 
  printf("\n");
  for (i = 1 << 15; i > 0; i = i / 2) 
    (x & i)? printf("1"): printf("0"); 
  printf("\n");
}

