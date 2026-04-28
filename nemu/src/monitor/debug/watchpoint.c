#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].old_value = 0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(char *expr_str) {
  if (free_ == NULL) {
    printf("No free watchpoint available.\n");
    return NULL;
  }

  bool success = true;
  uint32_t value = expr(expr_str, &success);

  if (!success) {
    printf("Bad expression: %s\n", expr_str);
    return NULL;
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  strncpy(wp->expr, expr_str, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';
  wp->old_value = value;

  printf("Watchpoint %d: %s = 0x%x (%u)\n",
         wp->NO, wp->expr, wp->old_value, wp->old_value);

  return wp;
}

void free_wp(int no) {
  WP *prev = NULL;
  WP *cur = head;

  while (cur != NULL) {
    if (cur->NO == no) {
      if (prev == NULL) {
        head = cur->next;
      }
      else {
        prev->next = cur->next;
      }

      cur->next = free_;
      free_ = cur;

      cur->expr[0] = '\0';
      cur->old_value = 0;

      printf("Delete watchpoint %d\n", no);
      return;
    }

    prev = cur;
    cur = cur->next;
  }

  printf("No watchpoint number %d\n", no);
}

void display_watchpoints() {
  WP *cur = head;

  if (cur == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tValue\t\tExpression\n");

  while (cur != NULL) {
    printf("%d\t0x%08x\t%s\n", cur->NO, cur->old_value, cur->expr);
    cur = cur->next;
  }
}

bool check_watchpoints() {
  WP *cur = head;
  bool stop = false;

  while (cur != NULL) {
    bool success = true;
    uint32_t new_value = expr(cur->expr, &success);

    if (!success) {
      printf("Failed to evaluate watchpoint %d: %s\n", cur->NO, cur->expr);
      cur = cur->next;
      continue;
    }

    if (new_value != cur->old_value) {
      printf("Watchpoint %d triggered: %s\n", cur->NO, cur->expr);
      printf("Old value = 0x%08x (%u)\n", cur->old_value, cur->old_value);
      printf("New value = 0x%08x (%u)\n", new_value, new_value);

      cur->old_value = new_value;
      stop = true;
    }

    cur = cur->next;
  }

  return stop;
}
