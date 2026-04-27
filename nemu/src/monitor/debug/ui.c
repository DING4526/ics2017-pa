#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include "cpu/reg.h"
#include "memory/memory.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Step instruction N times; The default N is 1", cmd_si },
  { "info", "Print the program state", cmd_info },
  { "x", "Examine memory", cmd_x },
  { "p", "Print the value of an expression", cmd_p },

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  uint64_t n = 1;

  if (args != NULL) {
    while (*args == ' ') { args ++; }

    // 检查args是不是全为数字
    if (*args == '\0' || *args == '-') {
      printf("Usage: si [N]\n");
      return 0;
    }
    for (char *p = args; *p != '\0'; p ++) {
      if (*p < '0' || *p > '9') {
        printf("Usage: si [N]\n");
        return 0;
      }
    }

    // 将args字符串转成10进制数n
    n = strtoull(args, NULL, 10);

    if (n == 0) {
      printf("Usage: si [N]\n");
      return 0;
    }
  }

  // 执行n条指令
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Usage: info [<SUBCMD>]\n");
    printf("SUBCMD list:\n");
    printf("  r - show registers\n");
    return 0;
  }

  char *subcmd = strtok(args, " ");

  if (subcmd == NULL) {
    printf("Usage: info [<SUBCMD>]\n");
    printf("SUBCMD list:\n");
    printf("  r - show registers\n");
    return 0;
  }

  // r - 打印寄存器状态
  if (strcmp(subcmd, "r") == 0) {
    int i;
    for (i = 0; i < 8; i ++) {
      printf("%s\t0x%08x\t%u\n", regsl[i], reg_l(i), reg_l(i));
    }

    printf("eip\t0x%08x\t%u\n", cpu.eip, cpu.eip);
  }
  else {
    printf("Unknown subcommand 'info %s'\n", subcmd);
  }

  return 0;
}

static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  char *n_str = strtok(args, " ");
  char *expr_str = strtok(NULL, "");

  if (n_str == NULL || expr_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  char *endptr = NULL;
  int n = strtol(n_str, &endptr, 10);

  if (endptr == n_str || n <= 0) {
    printf("Invalid N: %s\n", n_str);
    return 0;
  }

  endptr = NULL;
  // 目前仅支持纯数字地址
  vaddr_t addr = strtoul(expr_str, &endptr, 0);

  if (endptr == expr_str) {
    printf("Invalid address expression: %s\n", expr_str);
    return 0;
  }

  int i;
  for (i = 0; i < n; i ++) {
    uint32_t data = vaddr_read(addr + i * 4, 4);
    printf("0x%08x:\t0x%08x\n", addr + i * 4, data);
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success = true;
  uint32_t result = expr(args, &success);

  if (success) {
    printf("0x%x (%u)\n", result, result);
  }
  else {
    printf("Bad expression\n");
  }

  return 0;
}


void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
