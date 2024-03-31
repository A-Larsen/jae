#include "term.h"

Termios orig_termios;

void term_addLine(Term *term)
{
    if (term->pos.col == term->lines_count) {
        int pos = term->lines_count++;
        term->lines = (Lines *)realloc(term->lines, sizeof(*term->lines) *
                              term->lines_count);
        if (term->pos.col + 1 < term->ws.cols) {
            term->pos.col++;
        }
        memset(term->lines[pos].str, 0, 85);
        term->lines[pos].strlen = 0;
        term->pos.row = 1;
        /* WRITE("\r\n", 2); */
        term_setPos(term, term->pos.row, term->pos.col);
    }
}

void term_clearScreen()
{
	WRITE(ESCSEQ "2J", 4);
    WRITE(ESCSEQ "H", 3);
}

void term_close(Term *term)
{

    term_clearScreen();
    term_disableRawMode();
    printf("lines:\n");
    for (int i = 0; i < term->lines_count; ++i) {
        printf("%s\n", term->lines[i].str);
    }
    free(term->lines);
    exit(0);
}

void term_die(const char *s)
{
    term_clearScreen();

    if (s) {
        perror(s);
    }

    exit(1);
}

void term_disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        term_die("tcsetattr failed");
    }
}

void term_enableRawMode(Term *term)
{
    if (tcgetattr(STDIN_FILENO, &orig_termios)) {
        term_die("tcgetattr failed");
    }

    atexit(term_disableRawMode);

    term->io = orig_termios;
    term->io.c_iflag &= !(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term->io.c_oflag &= ~(OPOST);
    term->io.c_cflag |= (CS8);
    term->io.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    term->io.c_cc[VMIN] = 0;
    term->io.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term->io)) {
        term_die("tcsetattr failed");
    }
}

void term_exitCommandMode(Term *term)
{
    WRITE(ESCSEQ "2K", sizeof(ESCSEQ) + 2);
    term_setPos(term, term->pos.row, term->pos.col);
    term->global_mode = MODE_G_NORMAL; 
}

int term_getCursorPosition( Term *term)
{
    char buf[32];
    unsigned int i = 0;

    if (WRITE(ESCSEQ "6n", 4) != 4)
        return -1;

    while(i < sizeof(buf) - 1){
        if(read(STDIN_FILENO, &buf[i], 1) != 1) 
            break;

        if(buf[i] == 'R') break;
        i++;
    }

    buf[i] = '\0';

    if(buf[0] != ESC || buf[1] != '[') return -1;

    if(sscanf(&buf[2], PRIu16 ";" PRIu16, &term->ws.rows, &term->ws.cols) != 2) 
        return -1;

    return 0;
}

uint32_t term_getI(Term *term) {
    return term->pos.col - 1;
}

int term_getWindowSize(Term *term)
{

    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if (WRITE(ESCSEQ "999C" ESCSEQ "999B", 12) != 12) 
            return -1;
        return term_getCursorPosition(term);
    }else{
        // not sure why, but is seems like row and colomn need to swaped
        term->ws.rows = ws.ws_col;
        term->ws.cols = ws.ws_row;
        return 0;
    }
}

int term_init( Term *term, char *file_path)
{
    term_enableRawMode(term);
    memset(term, 0, sizeof(Term));
    memset(term->file_path, 0, sizeof(term->file_path));
    term->lines_count = 1;
    if (file_path){
        int file_path_len = strlen(file_path);
        memcpy(term->file_path, file_path, file_path_len);

    }
    term->pos.col = 1;
    term->pos.row = 1;
    term->lines = (Lines *)malloc(sizeof(*term->lines));
    memset(term->lines[0].str, 0, 85);
    term_getWindowSize(term);
    term_clearScreen();
    term_setCursorBlock();
    term_setPos(term, term->pos.row, term->pos.col);
    if (file_path){
        term_loadBufferFile(term, file_path);
    }
    return 0;
}

void term_loadBufferFile( Term *term, char *path)
{
    FILE *file = fopen(path, "r");
    int i = 0;
    char ch;

    while((ch = fgetc(file)) != EOF) {
        int pos = term->lines_count - 1;
        if (ch == '\n') {
            term->lines[pos].strlen = strlen(term->lines[pos].str);
            term_addLine(term);
            i = 0;
            continue;
        }

        char str[1] = {ch};
        term->lines[pos].str[i] = ch;
        WRITE(str, 1);
        i++;
    }
    if (term->lines_count > 2) {
        int pos = term->lines_count - 1;
        term->lines[pos].strlen = strlen(term->lines[pos].str);
        term_addLine(term);
    }
    term->pos.col = 1;
    term->pos.row = 1;
    term_setPos(term, 1, 1);
    fclose(file);
}

void term_moveCursor(Term *term, int key)
{
    uint32_t i = term_getI(term);

    switch (key) {
    case 'j': 
        {
            if (term->pos.col < term->lines_count) {
              term->pos.col++; 
              i = term_getI(term);
              uint8_t newlen = term->lines[i].strlen;

              if (term->pos.row > newlen)
                  term->pos.row = newlen;

              if(newlen == 0) term->pos.row = 1;
            }
            break;
          }
    case 'k': 
        {
            if (term->pos.col - 1 > 0) {
              term->pos.col--; 
              i = term_getI(term);
              uint8_t newlen = term->lines[i].strlen;
              if (term->pos.row > newlen)
                  term->pos.row = newlen;

              if(newlen == 0) term->pos.row = 1;
            }
            break; 
        }
    case 'h': 
        {
            if (term->pos.row - 1 > 0) 
                term->pos.row--; 

            break; 
        }
    case 'l': 
        {
            if (term->lines_count > 0) {
              if (term->pos.row <
                  term->lines[i].strlen + 1)
                  term->pos.row++; 
            }

            break;
        }
    } // switch
}

void term_parseCommand(Term *term)
{
    char *p = term->command.buffer;
    switch(*p) {
        case 'w': {
            p++;
            char *d = term->file_path;
            while(*(p++)) *(d++) = *p;
            int len = p - term->command.buffer;
            term->file_path[len] = '\0';
            term->command.callee = term_save;
            term->command.data = (void *)term->file_path;
            break;
        }
        case 'q': {
            term->command.callee = term_close;
            break;
        }
    }
}

int term_processKeypress(Term *term)
{
    int c = term_readKey();
    uint32_t i = term_getI(term);

    term->input[0] = '\0';
    if (term->global_mode == MODE_G_NORMAL) {
        switch (c) {
        case 'I': {
            term->pos.row = 1;
            term->global_mode = MODE_G_INSERT;
            term->insert_mode = MODE_I_DEFAULT;
            term_setPos(term, term->pos.row, term->pos.col);
            term_setCursorLine(); 
            break;

        }
        case 'A': {
            term->pos.row = term->lines[i].strlen + 1;
            term->global_mode = MODE_G_INSERT;
            term->insert_mode = MODE_I_DEFAULT;
            term_setPos(term, term->pos.row, term->pos.col);
            term_setCursorLine(); 
            break;

        }
        case 'o':  {
            term_addLine(term); 
            break;
        }
        case ':':  {
            char format[255];
            sprintf(format, ESCSEQ "%hu;%huH", term->ws.cols, 1U);
            int len = strlen(format);
            WRITE(format, len);
            WRITE(":", 1);
            memset(term->command.buffer, 0, sizeof(term->command.buffer));
            // -1 because of ':' symbol
            term->command.i = -1;
            /* term->command.i = 0; */
            term->command.called = false;
            term->command.data = NULL;
            term->global_mode = MODE_G_COMMAND;
            break;

        }
        case 'r': {
            term->global_mode = MODE_G_INSERT;
            term->insert_mode = MODE_I_REPLACE;
            term_setCursorLine(); 
            break;
        }
        case 'j': case 'k': case 'l': case 'h': {
            term_moveCursor(term, c); 
            break;
        }
        case 'q' : {
            term_close(term);
            break;
        }
        case 'i':  {
            term->global_mode = MODE_G_INSERT; 
            term->insert_mode = MODE_I_DEFAULT;
            term_setCursorLine(); 
            break; 
        }
        } // switch
    } else if (term->global_mode == MODE_G_INSERT) {
        switch (c) {
        case '\r':term_addLine(term); break;
        case BACKSPACE: term_removeCharBack(term); break;
        case ESC: term_setCursorBlock(); term->global_mode = MODE_G_NORMAL; break;
        default: term->input[0] = c; break;
        } // switch
    } else if (term->global_mode == MODE_G_COMMAND) {
        switch (c) {
        default: term->input[0] = c; break;
        case '\r':
                term_parseCommand(term);
                term->command.called = true;
                break;
        case ESC: term_setCursorBlock(); 
            term_exitCommandMode(term);
            break;
        } // switch

    }

    return 0;
}

int term_readKey()
{
   int nread;
   char c;

    while((nread = READ(&c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) 
            term_die("couldnt read key");
    }

    if (c == ESC ) {
        char seq[3];

        if(READ(&seq[0], 1) != 1) return ESC;

        if(READ(&seq[1], 1) != 1) return ESC;

        if (seq[0] == '[') {
            if (seq[1] >= 0 && seq[1] <= '9') {
                if(READ(&seq[2], 1) != 1) return ESC;

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == '0') {
            switch(seq[1]){
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return ESC;
    }

    return c;
}

void term_removeCharBack( Term *term)
{
    uint16_t i = term_getI(term);

    if (term->pos.row == 1) return;
    term_setPos(term, 1, term->pos.col);
    WRITE(ESCSEQ "2K", sizeof(ESCSEQ) + 2);
    term->lines[i].strlen--;

    if (term->lines[i].strlen > 0) {
        for (int j = term->pos.row - 2; j < term->lines[i].strlen; ++j) {
            term->lines[i].str[j] = term->lines[i].str[j + 1];
        }
        term->lines[i].str[term->lines[i].strlen] = '\0';
        WRITE(term->lines[i].str, term->lines[i].strlen);
        term->pos.row--;

    } else if (term->lines[i].strlen ==  0) {
        term->lines[i].str[0] = '\0';
        term->pos.row = 1;
    }

    term_setPos(term, term->pos.row, term->pos.col);
}

void term_render(Term *term)
{
    if (term->global_mode == MODE_G_INSERT) {
        uint32_t offset = (term->pos.col < term->ws.cols) \
                          ? 0 : term->pos.col - term->ws.cols;
        uint32_t i = term_getI(term) - offset;
        /* uint32_t o = term_getI(term) + offset; */
        if (term->input[0] != '\0') {
            if (term->insert_mode == MODE_I_REPLACE) {
                term->lines[i].str[term->pos.row - 1] = term->input[0];
                WRITE(term->input, 1);
                term->pos.row++;
                if (term->pos.row > term->lines[i].strlen)
                    term->lines[i].strlen = term->pos.row;

            } else if (term->insert_mode == MODE_I_DEFAULT) {

                term->lines[i].strlen++;
                if (term->lines[i].strlen > 1) {
                    for(int j = term->lines[i].strlen; j > term->pos.row - 1; --j) {
                        term->lines[i].str[j] = term->lines[i].str[j - 1];
                    }
                } 

                term->lines[i].str[term->pos.row - 1] = term->input[0];
                term_setPos(term, 1, term->pos.col);
                WRITE(term->lines[i].str, term->lines[i].strlen);
                term->pos.row++;
                term_setPos(term, term->pos.row, term->pos.col);
            }
        }
    }else  if (term->global_mode == MODE_G_NORMAL) {
        term_setPos(term, term->pos.row, term->pos.col);
        } else if (term->global_mode == MODE_G_COMMAND) {
        if (term->command.called) {
            if (term->command.callee) term->command.callee(term);
            term_exitCommandMode(term);
        } else  {
            term->command.buffer[term->command.i] = *term->input;
            term->command.buffer[term->command.i + 1] = '\0';
            term->command.i++;
            WRITE(term->input, 1);
        }
    } 
}

void term_run(Term *term)
{
    while(1) {
        if(term_processKeypress(term) == 1) break;
        term_render(term);
    }
    term_close(term);
}

void term_save(Term *term)
{

    /* printf("%d\n", term->lines_count); */
    char *file_path = (char *)term->command.data;
    FILE *file = fopen(file_path, "w+");
    for (uint16_t i = 0; i < term->lines_count; ++i) {
        fwrite(term->lines[i].str, 1, term->lines[i].strlen, file);
        if (i + 1 != term->lines_count) {
            fwrite("\n", 1, 1, file);
        }
    }
    fclose(file);
}

void term_setCursorBlock()
{
    WRITE(ESCSEQ "2 q", sizeof(ESCSEQ) + 3);
}

void term_setCursorLine()
{
    WRITE(ESCSEQ "6 q", sizeof(ESCSEQ) + 3);
}

void term_setPos(Term *term, uint16_t row, uint16_t col) {
        /* if (col + PADDINGTOP > term->ws.cols - PADDINGBOTTOM) return; */
        uint16_t leftPadding = row > 0 ? row + PADDINGLEFT : PADDINGLEFT + 1;
        uint16_t topPadding = col > 0 ? col + PADDINGTOP : PADDINGTOP + 1;
        char format[255];
        sprintf(format, ESCSEQ "%hu;%huH", topPadding, leftPadding);
        int len = strlen(format);
        WRITE(format, len);
}
