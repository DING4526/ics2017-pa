#include "nemu.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"

/* 额外增加一段死循环检测工具 */

#ifdef DEAD_LOOP_CHECK
  #define EIP_HISTORY_SIZE 1024
  #define EIP_REPEAT_THRESHOLD 512

  static vaddr_t eip_history[EIP_HISTORY_SIZE];
  static int eip_history_idx = 0;
  static int eip_history_cnt = 0;

  static bool detect_dead_loop(vaddr_t eip) {
    int cnt = 0;

    for (int i = 0; i < eip_history_cnt; i ++) {
      if (eip_history[i] == eip) {
        cnt ++;
      }
    }

    eip_history[eip_history_idx] = eip;
    eip_history_idx = (eip_history_idx + 1) % EIP_HISTORY_SIZE;

    if (eip_history_cnt < EIP_HISTORY_SIZE) {
      eip_history_cnt ++;
    }

    return cnt > EIP_REPEAT_THRESHOLD;
  }
#endif


/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

int nemu_state = NEMU_STOP;

void exec_wrapper(bool);
void query_intr(void);

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  if (nemu_state == NEMU_END) {
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  }
  nemu_state = NEMU_RUNNING;

  bool print_flag = n < MAX_INSTR_TO_PRINT;

  for (; n > 0; n --) {
    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */

    #ifdef DEAD_LOOP_CHECK
      vaddr_t old_eip = cpu.eip;
    #endif

    exec_wrapper(print_flag);

    #ifdef DEAD_LOOP_CHECK
      if (detect_dead_loop(old_eip)) {
        printf("Possible dead loop detected near eip = 0x%08x\n", old_eip);
        nemu_state = NEMU_STOP;
        return;
      }
    #endif

#ifdef DEBUG
    /* TODO: check watchpoints here. */
    if (check_watchpoints()) {
      nemu_state = NEMU_STOP;
    }

#endif

#ifdef HAS_IOE
    extern void device_update();
    extern bool device_update_pending();

    // if (device_update_pending()) {
    //   device_update();
    // }
    device_update();

    query_intr();
#endif

    if (nemu_state != NEMU_RUNNING) { return; }
  }

  if (nemu_state == NEMU_RUNNING) { nemu_state = NEMU_STOP; }
}
