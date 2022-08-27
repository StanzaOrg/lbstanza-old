#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include "polling-sighandler.c"

int sleep_ms (uint64_t ms){
  struct timespec t1, t2;
  t1.tv_sec = ms / 1000L;
  t1.tv_nsec = (ms % 1000L) * 1000000L;
  return (int)nanosleep(&t1, &t2);
}

int main (int argc, char** argvs){
  uint64_t prev_handler = install_polling_sigint_handler();
  printf("Start Program\n");
  while(1){
    int flag = poll_sigint_flag();
    printf("GLOBAL FLAG = %d\n", flag);
    sleep_ms(1000);
    if(flag)
      restore_previous_sigint_handler(prev_handler);
  }
  return 0;  
}
