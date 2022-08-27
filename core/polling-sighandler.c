#include<signal.h>
#include<stdint.h>

int SIGINT_FLAG = 0;

void sigint_flag_handler (int signal){
  SIGINT_FLAG = 1;
}

uint64_t install_polling_sigint_handler () {
  SIGINT_FLAG = 0;
  void (*prev_handler)(int) = signal(SIGINT, sigint_flag_handler);
  return (uint64_t)prev_handler;
}

void restore_previous_sigint_handler (uint64_t prev_handler_int) {
  SIGINT_FLAG = 0;
  void (*prev_handler)(int) = (void*)prev_handler_int;
  signal(SIGINT, prev_handler);
}

int poll_sigint_flag () {
  int flag = SIGINT_FLAG;
  SIGINT_FLAG = 0;
  return flag;
}
