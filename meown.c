/*** includes ***/
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define MEOWN_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f) // change 5 and 6 bit to 0 like CTRL KEY do

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios; // struct to receive original terminal atributes
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() { 
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) // reset terminal atributes to default
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) // get terminal atributes
    die("tcgetattr");

  atexit(disableRawMode); // schedule disableRawMode function to when program exit
  struct termios raw = E.orig_termios; // create new termios struct to store raw mode config

  // config raw mode
  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON); 
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag &= ~(CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 100;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) // set atributes
    die("tcsetattr");
}

char editorReadKey() {
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1)) != 1) { // read stdin into c
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    if (y == E.screenrows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
          "Meown editor -- version %s", MEOWN_VERSION);
      if (welcomelen > E.screencols) welcomelen = E.screencols;
      abAppend(ab, welcome, welcomelen);
    } else {
      abAppend(ab, "~", 1);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows -1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** input ***/

void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'): // add ctrl_q as exit key
      editorRefreshScreen();
      exit(0);
      break;
  }
  
  // format output
  if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
}

/*** init ***/

void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screenrows) == -1) die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  };
  return 0;
}
