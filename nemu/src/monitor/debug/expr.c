#include "nemu.h"
#include "cpu/reg.h"
#include "memory/memory.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NUM,
  TK_HEX,

  TK_REG,    // $eax 这种寄存器
  TK_NEQ,    // !=
  TK_AND,    // &&
  TK_OR,     // ||
  TK_DEREF,  // *addr 解引用
  TK_NEG,    // -1 负号
  TK_NOT     // !expr 逻辑非

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces

  // hex 要放前面，优先级更高
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // hex numbers
  {"[0-9]+", TK_NUM},   // decimal numbers
  {"\\$[a-zA-Z]+", TK_REG},   // 寄存器

  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // logical and
  {"\\|\\|", TK_OR},    // logical or

  {"\\+", '+'},             // plus
  {"-", '-'},               // minus
  {"\\*", '*'},             // multiply
  {"/", '/'},               // divide
  {"!", TK_NOT},            // logical not
  {"\\(", '('},             // left parenthesis
  {"\\)", ')'},             // right parenthesis


};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_NUM:
          case TK_HEX:
          case TK_REG:
            assert(nr_token < 32);
            assert(substr_len < 32);
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;

          default:
            assert(nr_token < 32);
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token ++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  for (i = 0; i < nr_token; i ++) {
    int type = tokens[i].type;

    bool unary = false;

    if (i == 0) {
      unary = true;
    }
    else {
      int prev = tokens[i - 1].type;

      if (prev == '(' ||
          prev == '+' || prev == '-' ||
          prev == '*' || prev == '/' ||
          prev == TK_EQ || prev == TK_NEQ ||
          prev == TK_AND || prev == TK_OR ||
          prev == TK_DEREF || prev == TK_NEG || prev == TK_NOT) {
        unary = true;
      }
    }

    if (type == '*' && unary) {
      tokens[i].type = TK_DEREF;
    }
    else if (type == '-' && unary) {
      tokens[i].type = TK_NEG;
    }
  }

  return true;
}

// 判断tokens[p..q]是否被一对匹配括号包围
static bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  int balance = 0;
  int i;

  for (i = p; i <= q; i ++) {
    if (tokens[i].type == '(') {
      balance ++;
    }
    else if (tokens[i].type == ')') {
      balance --;
    }

    if (balance < 0) {
      return false;
    }

    // 如果在tokens[p..q]中，括号不匹配（即balance在某个位置变为0），则返回false
    if (balance == 0 && i < q) {
      return false;
    }
  }

  return balance == 0;
}

// 定义运算符优先级，数字越大优先级越高
static int precedence(int type) {
  switch (type) {
    case TK_OR:
      return 1;

    case TK_AND:
      return 2;

    case TK_EQ:
    case TK_NEQ:
      return 3;

    case '+':
    case '-':
      return 4;

    case '*':
    case '/':
      return 5;

    case TK_DEREF:
    case TK_NEG:
    case TK_NOT:
      return 6;

    default:
      return 100;
  }
}

// 区分一元二元运算符
static bool is_binary_operator(int type) {
  return type == TK_OR || type == TK_AND ||
         type == TK_EQ || type == TK_NEQ ||
         type == '+' || type == '-' ||
         type == '*' || type == '/';
}
static bool is_unary_operator(int type) {
  return type == TK_DEREF || type == TK_NEG || type == TK_NOT;
}

// 在tokens[p..q]中找到优先级最低的运算符，并返回其位置
static int find_dominant_op(int p, int q) {
  // 本质上是在最低级运算符中找到最右边的那个
  int op = -1;
  int min_prec = 100;
  int balance = 0;
  int i;

  for (i = p; i <= q; i ++) {
    int type = tokens[i].type;

    if (type == '(') {
      balance ++;
      continue;
    }

    if (type == ')') {
      balance --;
      continue;
    }

    // 跳过括号内的运算符
    if (balance != 0) {
      continue;
    }

    // 只找二元运算符参与比较
    if (!is_binary_operator(type)) {
      continue;
    }

    int prec = precedence(type);

    // <=是为了保证在同一优先级时，选择最右边的那个
    if (prec <= min_prec) {
      min_prec = prec;
      op = i;
    }
  }

  return op;
}

// 从字符串s中解析出寄存器的值
static uint32_t get_reg_val(char *s, bool *success) {
  if (strcmp(s, "$eax") == 0) return cpu.eax;
  if (strcmp(s, "$ecx") == 0) return cpu.ecx;
  if (strcmp(s, "$edx") == 0) return cpu.edx;
  if (strcmp(s, "$ebx") == 0) return cpu.ebx;
  if (strcmp(s, "$esp") == 0) return cpu.esp;
  if (strcmp(s, "$ebp") == 0) return cpu.ebp;
  if (strcmp(s, "$esi") == 0) return cpu.esi;
  if (strcmp(s, "$edi") == 0) return cpu.edi;
  if (strcmp(s, "$eip") == 0) return cpu.eip;

  printf("Unknown register: %s\n", s);
  *success = false;
  return 0;
}

// 递归计算tokens[p..q]表达式的值
static uint32_t eval(int p, int q, bool *success) {
  if (p > q) {
    *success = false;
    return 0;
  }

  else if (p == q) {
    if (tokens[p].type == TK_NUM) {
      return strtoul(tokens[p].str, NULL, 10);
    }

    if (tokens[p].type == TK_HEX) {
      return strtoul(tokens[p].str, NULL, 16);
    }

    if (tokens[p].type == TK_REG) {
      return get_reg_val(tokens[p].str, success);
    }

    *success = false;
    return 0;
  }

  else if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  else {
    int op = find_dominant_op(p, q);

    if (op != -1) {
      uint32_t val1 = eval(p, op - 1, success);
      if (!*success) return 0;

      uint32_t val2 = eval(op + 1, q, success);
      if (!*success) return 0;

      switch (tokens[op].type) {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/':
          if (val2 == 0) {
            printf("Error: division by zero\n");
            *success = false; return 0;
          } 
          return val1 / val2;
        case TK_EQ: return val1 == val2;
        case TK_NEQ: return val1 != val2;
        case TK_AND: return val1 && val2;
        case TK_OR: return val1 || val2;

        default:
          *success = false;
          return 0;
      }
    }

    // 增加对一元运算符的处理（在二元运算符之后）
    if (is_unary_operator(tokens[p].type)) {
      uint32_t val = eval(p + 1, q, success);
      if (!*success) return 0;

      switch (tokens[p].type) {
        case TK_NEG: return -val;
        case TK_NOT: return !val;
        case TK_DEREF: return vaddr_read(val, 4);

        default:
          *success = false;
          return 0;
      }
    }

    *success = false;
    return 0;
  } 
}


uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  if (nr_token == 0) {
    *success = false;
    return 0;
  }

  *success = true;
  return eval(0, nr_token - 1, success);

  return 0;
}
