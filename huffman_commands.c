/*
 * Character-level Huffman encoder for COMMANDS array.
 * Uses shortened strings to fit within 32 bits (4 bytes) when encoded.
 * Each character gets a variable-length bit code; a command = concat of char
 * codes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMANDS 64
#define NUM_CHARS 256
#define MAX_CODE_LEN 64
#define MAX_HEAP (NUM_CHARS * 2)
#define TARGET_BITS 32

/* Shortened command strings (for encoding). See COMMENTS[] for full meaning. */
static const char *COMMANDS[] = {
    "Pltog",  "Plstat", "Plmode",  "Pltarg", "Plkwlm", "Plinit", "Pltqcm",
    "Plclmp", "LcKp",   "lcKi",    "lcKd",   "lcpid",  "lcSRT",  "lcLcTog",
    "lcCSR",  "lcCVD",  "lcTVD",   "lcLTq",  "lcITq",  "lck",    "lcMTq",
    "lcPTq",  "lcUF",   "lcmode",  "lcSt",   "lcPh",   "efTog",  "efEBk",
    "efLpCt", "efCOk",  "efTS_s",  "efTC_s", "efESk",  "efESs",  "efLEk",
    "efTLk",  "efFLp",  "rgRgTog", "rgMd",   "rgApTq", "rgBTN",  "rgRTq",
    "rgTLD",  "rgTZPD", "rgPBM",   "rgPAC",  "rgPdMu", "rgTk"};

/* Comment for each command: full name or description (same index as
 * COMMANDS[]). */
static const char *COMMENTS[] = {
    "Power limit toggle",
    "Power limit status",
    "Power limit mode",
    "Power limit target",
    "Power limit kW limit",
    "Power limit init",
    "Power limit torque command",
    "Power limit clamp",
    "Launch control Kp",
    "Launch control Ki",
    "Launch control Kd",
    "Launch control PID",
    "Launch control slip ratio target",
    "Launch control LC toggle",
    "Launch control current slip ratio",
    "Launch control current velocity difference",
    "Launch control target velocity difference",
    "Launch control LC torque command",
    "Launch control initial torque",
    "Launch control k",
    "Launch control max torque",
    "Launch control previous torque",
    "Launch control use filter",
    "Launch control mode",
    "Launch control state",
    "Launch control phase",
    "Efficiency efficiency toggle",
    "Efficiency energy budget kWh (efEBk)",
    "Efficiency lap counter",
    "Efficiency carry over energy kWh (efCOk)",
    "Efficiency time eff in straights (s)",
    "Efficiency time eff in corners (s)",
    "Efficiency energy spent in corners kWh (efESk)",
    "Efficiency energy spent in straights kWh (efESs)",
    "Efficiency lap energy spent kWh (efLEk)",
    "Efficiency total lap distance km (efTLk)",
    "Efficiency finished lap",
    "Regen regen toggle",
    "Regen mode",
    "Regen APPS torque",
    "Regen BPS torque Nm",
    "Regen regen torque command",
    "Regen torque limit D Nm",
    "Regen torque at zero pedal D Nm",
    "Regen percent BPS for max regen",
    "Regen percent APPS for coasting",
    "Regen pad mu",
    "Regen tick"};

#define NUM_COMMANDS ((int)(sizeof(COMMANDS) / sizeof(COMMANDS[0])))

typedef struct Node {
  int symbol; /* character (0..255), or -1 for internal */
  unsigned long freq;
  struct Node *left, *right;
} Node;

typedef struct {
  unsigned long code; /* up to MAX_CODE_LEN bits */
  int len;
} CodeEntry;

/* One code per character */
static CodeEntry char_codes[NUM_CHARS];

static Node *heap[MAX_HEAP];
static int heap_size;

static void heap_swap(int i, int j) {
  Node *t = heap[i];
  heap[i] = heap[j];
  heap[j] = t;
}

static void heap_up(int i) {
  while (i > 0) {
    int p = (i - 1) / 2;
    if (heap[p]->freq <= heap[i]->freq)
      break;
    heap_swap(p, i);
    i = p;
  }
}

static void heap_down(int i) {
  for (;;) {
    int l = 2 * i + 1, r = 2 * i + 2, smallest = i;
    if (l < heap_size && heap[l]->freq < heap[smallest]->freq)
      smallest = l;
    if (r < heap_size && heap[r]->freq < heap[smallest]->freq)
      smallest = r;
    if (smallest == i)
      break;
    heap_swap(i, smallest);
    i = smallest;
  }
}

static void heap_push(Node *n) {
  heap[heap_size++] = n;
  heap_up(heap_size - 1);
}

static Node *heap_pop(void) {
  Node *top = heap[0];
  heap[0] = heap[--heap_size];
  if (heap_size > 0)
    heap_down(0);
  return top;
}

/* Count character frequencies from all command strings */
static void count_char_freq(unsigned long *freq) {
  int c, i;
  const char *p;
  for (c = 0; c < NUM_CHARS; c++)
    freq[c] = 0;
  for (i = 0; i < NUM_COMMANDS; i++) {
    for (p = COMMANDS[i]; *p; p++)
      freq[(unsigned char)*p]++;
  }
}

/* Build Huffman tree from character frequencies; assign code per character */
static void build_char_codes(const unsigned long *freq) {
  int c, n_used = 0;
  Node *root = NULL;
  heap_size = 0;

  for (c = 0; c < NUM_CHARS; c++) {
    char_codes[c].code = 0;
    char_codes[c].len = 0;
    if (freq[c] == 0)
      continue;
    Node *n = (Node *)malloc(sizeof(Node));
    n->symbol = c;
    n->freq = freq[c];
    n->left = n->right = NULL;
    heap_push(n);
    n_used++;
  }

  if (n_used == 0)
    return;

  while (heap_size > 1) {
    Node *a = heap_pop();
    Node *b = heap_pop();
    Node *p = (Node *)malloc(sizeof(Node));
    p->symbol = -1;
    p->freq = a->freq + b->freq;
    p->left = a;
    p->right = b;
    heap_push(p);
  }
  root = heap_pop();

  /* DFS: left = 0, right = 1 */
  typedef struct {
    Node *n;
    unsigned long code;
    int len;
  } StackFrame;
  StackFrame stack[MAX_CODE_LEN * 2];
  int sp = 0;
  stack[sp].n = root;
  stack[sp].code = 0;
  stack[sp].len = 0;
  sp++;

  while (sp > 0) {
    StackFrame f = stack[--sp];
    if (f.n->symbol >= 0) {
      char_codes[f.n->symbol].code = f.code;
      char_codes[f.n->symbol].len = f.len;
      continue;
    }
    if (f.n->right) {
      stack[sp].n = f.n->right;
      stack[sp].code = (f.code << 1) | 1;
      stack[sp].len = f.len + 1;
      sp++;
    }
    if (f.n->left) {
      stack[sp].n = f.n->left;
      stack[sp].code = f.code << 1;
      stack[sp].len = f.len + 1;
      sp++;
    }
  }

  /* Free tree */
  {
    Node *stk[MAX_HEAP];
    int stksp = 0;
    stk[stksp++] = root;
    while (stksp > 0) {
      Node *t = stk[--stksp];
      if (t->left)
        stk[stksp++] = t->left;
      if (t->right)
        stk[stksp++] = t->right;
      free(t);
    }
  }
}

/* Encode command string: concatenate each character's Huffman code. Return
 * total bits. */
static int encode_command(const char *cmd, int *out_bits, int *out_bytes) {
  int bits = 0;
  const char *p;
  for (p = cmd; *p; p++) {
    int c = (unsigned char)*p;
    bits += char_codes[c].len;
  }
  *out_bits = bits;
  *out_bytes = (bits + 7) / 8;
  return bits;
}

/* Print the full bit string for a command (each char's code concatenated) */
static void print_command_bits(const char *cmd) {
  const char *p;
  for (p = cmd; *p; p++) {
    int c = (unsigned char)*p;
    int len = char_codes[c].len;
    unsigned long code = char_codes[c].code;
    int i;
    for (i = len - 1; i >= 0; i--)
      putchar(((code >> i) & 1) ? '1' : '0');
  }
}

static void print_char_code(int c) {
  int len = char_codes[c].len;
  unsigned long code = char_codes[c].code;
  int i;
  for (i = len - 1; i >= 0; i--)
    putchar(((code >> i) & 1) ? '1' : '0');
}

int main(void) {
  unsigned long char_freq[NUM_CHARS];
  int i, total_bits = 0, max_bits = 0, min_bits = 999999;

  count_char_freq(char_freq);
  build_char_codes(char_freq);

  /* Character code table (only chars that appear) */
  printf("Huffman codes per character (used in commands):\n");
  printf("%-6s %-8s %s\n", "Char", "Code", "Len");
  printf("----------------------------------------\n");
  for (i = 0; i < NUM_CHARS; i++) {
    if (char_codes[i].len == 0)
      continue;
    if (i >= 32 && i < 127)
      printf("'%c'     ", i);
    else
      printf("0x%02X   ", i);
    print_char_code(i);
    printf(" %d\n", char_codes[i].len);
  }

  /* Short form -> comment (for reference) */
  printf("\nShort form -> comment (full meaning):\n");
  printf("%-14s %s\n", "Short", "Comment");
  printf("------------------------------------------------------------\n");
  for (i = 0; i < NUM_COMMANDS; i++)
    printf("%-14s %s\n", COMMANDS[i], COMMENTS[i]);

  printf("\nEncoded commands (each character -> its bits, concatenated):\n");
  printf("%-4s %-14s %-44s %6s %6s  %s\n", "Idx", "Command", "Bit string",
         "Bits", "Bytes", "OK/OVER");
  printf("%s\n", "-------------------------------------------------------------"
                 "-------------------");

  for (i = 0; i < NUM_COMMANDS; i++) {
    int bits, bytes;
    const char *cmd = COMMANDS[i];
    encode_command(cmd, &bits, &bytes);
    total_bits += bits;
    if (bits > max_bits)
      max_bits = bits;
    if (bits < min_bits)
      min_bits = bits;
    printf("%-4d %-14s ", i, cmd);
    print_command_bits(cmd);
    printf(" %6d %6d  %s\n", bits, bytes, bits <= TARGET_BITS ? "OK" : "OVER");
  }

  printf("\n--- When sending (target %d bits / %d bytes max) ---\n",
         TARGET_BITS, (TARGET_BITS + 7) / 8);
  printf("Per command:  min %d bits (%d byte(s)), max %d bits (%d byte(s))\n",
         min_bits, (min_bits + 7) / 8, max_bits, (max_bits + 7) / 8);
  printf("Average:      %.2f bits, %.2f bytes (per command)\n",
         (double)total_bits / NUM_COMMANDS,
         (double)total_bits / (8.0 * NUM_COMMANDS));
  printf("Each character has its own variable-length code; command = concat of "
         "char codes.\n");

  return 0;
}
