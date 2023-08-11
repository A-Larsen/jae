#ifndef _TERM_H_
#define _TERM_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "perror.h"

#define CTRL_KEY(k) ((k) & 0x1f)
#define READ(c, n)  read(STDIN_FILENO, c, n)
#define WRITE(c, n) write(STDIN_FILENO, c, n)
#define ESCSEQ "\x1b["
#define ESC '\x1b'
#define PADDINGLEFT 4
#define PADDINGBOTTOM 2
#define PADDINGTOP 1U
#define TABWIDTH 4

typedef struct termios Termios;

typedef enum 
{
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
}
EditorKey;

typedef struct _Pos 
{
    uint16_t col; 
    uint8_t row;
} 
Pos;

typedef enum _GlobalMode {
    MODE_G_NORMAL, MODE_G_INSERT, MODE_G_COMMAND
} GlobalMode;

typedef enum _ModeNormal {
    MODE_N_VISUAL, MODE_N_VBLOCK, 
    MODE_N_VLINE
} ModeNormal;

typedef enum _ModeInsert {
    MODE_I_DEFAULT, MODE_I_REPLACE
} ModeInsert;

typedef struct _Lines{
    // I'm assumning that you're lines are not over 80 characters
    char str[85]; 
    uint8_t strlen;
    uint8_t *wordspos; 
    uint8_t start; 
    uint8_t end;
} Lines;

typedef struct _Term 
{
    GlobalMode global_mode;
    ModeInsert insert_mode;
    Pos pos;
	struct {uint16_t cols; uint16_t rows;} ws;
    Termios io;
    ModeNormal normal_mode;
    Lines *lines;
    uint16_t lines_count;
    struct {uint16_t x; uint16_t y;} cursor;
    char input[1];
    char file_path[50];
    struct {
        char buffer[512];
        int i;
        void *data;
        void (*callee) (struct _Term *term);
        bool called;
    } command;
} 
Term;

void term_addLine(Term *term);
void term_clearScreen();
void term_close(Term *term);
void term_die(const char *s);
void term_disableRawMode();
void term_enableRawMode(Term *term);
void term_exitCommandMode(Term *term);
int term_getCursorPosition(Term *term);
uint32_t term_getI(Term *term);
int term_getWindowSize(Term *term);
int term_init(Term *term, char *file_path);
void term_loadBufferFile(Term *term, char *path);
void term_moveCursor(Term *term, int key);
void term_parseCommand(Term *term);
int term_processKeypress(Term *term);
int term_readKey();
void term_removeCharBack( Term *term);
void term_render(Term *term);
void term_run(Term *term);
void term_save(Term *term);
void term_setCursorBlock();
void term_setCursorLine();
void term_setPos(Term *term, uint16_t row, uint16_t col);

#endif // _TERM_H_
