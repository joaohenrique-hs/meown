/*** includes ***/
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f) // change 5 and 6 bit to 0 like CTRL KEY do

/*** data ***/

struct termios orig_termios; // struct to receive original terminal atributes

/*** terminal ***/

void die(const char *s) {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() { 
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) // reset terminal atributes to default
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) // get terminal atributes
    die("tcgetattr");

  atexit(disableRawMode); // schedule disableRawMode function to when program exit
  struct termios raw = orig_termios; // create new termios struct to store raw mode config

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

/*** output ***/

void editorDrawRows() {
  int y;
  for (y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen() {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[H", 3);
}

/*** input ***/

void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'): // add ctrl_q as exit key
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
int main() {
  editorRefreshScreen();
  enableRawMode();

  while (1) {
    editorProcessKeypress();
  };
  return 0;
}
