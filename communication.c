#include "umps/libumps.h"
#include "umps/arch.h"
#include "umps/types.h"
#include <fcntl.h>

#define ST_READY           1                    /*Define dei registri per l'uso del terminale*/
#define ST_BUSY            3
#define ST_TRANSMITTED     5
#define ST_RECEIVED        5

#define CMD_ACK            1
#define CMD_TRANSMIT       2
#define CMD_RECV           2

#define CHAR_OFFSET        8
#define TERM_STATUS_MASK   0xFF

#define NULL ( (void *) 0 )

#define IS_DIGIT(x) ('0' <= x && x <= '9')

#define LINE_BUF_SIZE 64

static char buf[LINE_BUF_SIZE];
static char buf_2[LINE_BUF_SIZE];


static int term_putchar(char c,volatile termreg_t *rg);

static int term_puts(char *str,volatile termreg_t *rg);

static int term_getchar(volatile termreg_t *rg);

static void readline(char *b, unsigned int count,volatile termreg_t *rg);

volatile termreg_t *terminal = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL, 0);

volatile termreg_t *terminal_2 = (termreg_t*) DEV_REG_ADDR(IL_TERMINAL, 1);

volatile dtpreg_t *disk = (dtpreg_t*) DEV_REG_ADDR(IL_DISK, 0);

static unsigned int tx_status(volatile termreg_t *tp);

static unsigned int rx_status(volatile termreg_t *tp);

void main(void)
{
  while(1){

    readline(buf_2, LINE_BUF_SIZE,terminal);

    term_puts(buf_2,terminal_2);
                                                        /*Lettura messaggio dal terminale 0 e scritto nel terminale 1*/
    term_puts("\n",terminal_2);


    readline(buf, LINE_BUF_SIZE,terminal_2);

    term_puts(buf,terminal);
                                                        /*Lettura messaggio dal terminale 1 e scritto nel terminale 0*/
    term_puts("\n",terminal);

  }

}




static void readline(char *b, unsigned int count, volatile termreg_t *rg)
{
  int c;

  while (--count && (c = term_getchar(rg)) != '\n')                               /*Funzione ausiliara alla term_getchar che serve per limitare
                                                                                    e controllare la lunghezza della stringa digitata*/
  *b++ = c;

  *b = '\0';
}


int term_getchar(volatile termreg_t *rg)
{
  unsigned int stat;

  stat = rx_status(rg);

  if (stat != ST_READY && stat != ST_RECEIVED)
  return -1;                                                       /*Controllo sullo stato del terminale */

  rg->recv_command = CMD_RECV;

  while ((stat = rx_status(rg)) == ST_BUSY)
  ;

  if (stat != ST_RECEIVED)
  return -1;

  stat = rg->recv_status;

  rg->recv_command = CMD_ACK;                                       /*Risposta all'interrupt del terminale*/


  return stat >> CHAR_OFFSET;                                       /*Ritorno del carattere della stringa, viene preso uno a uno
                                                                     in ordine*/
}



int term_puts(char *str,volatile termreg_t *rg)
{
  for (; *str; ++str)
  if (term_putchar(*str,rg))                                      /*Funzione ausiliara che trasmette carattere per carattere*/
  return -1;                                                      /*alla funzione putchar per scrivere sul terminale*/
  return 0;
}


int term_putchar(char c,volatile termreg_t *rg)
{
  unsigned int stat;
  stat = tx_status(rg);
  if (stat != ST_READY && stat != ST_TRANSMITTED)
  return -1;                                                       /*Controllo sullo stato del terminale */

  rg->transm_command = ((c << CHAR_OFFSET) | CMD_TRANSMIT);         /*Trasmissione del carattere sul terminale in questione passato come parametro*/


  while ((stat = tx_status(rg)) == ST_BUSY)
  ;


  if (stat != ST_TRANSMITTED)
  return -1;

  rg->transm_command = CMD_ACK;                                     /*Risposta all'interrupt del terminale*/
  return 0;
}


static unsigned int tx_status(volatile termreg_t *tp)
{                                                           /*Ricevere lo stato di trasmissione del terminale */
  return ((tp->transm_status) & TERM_STATUS_MASK);
}

static unsigned int rx_status(volatile termreg_t *tp)
{                                                           /*Ricevere lo stato di ricezione del terminale */
  return ((tp->recv_status) & TERM_STATUS_MASK);
}
