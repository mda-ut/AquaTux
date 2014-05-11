#include "Operation.h"

void OperationNull::work()
{
  // ncurses stuff
  initscr();
  clear();
  cbreak();
  printw("Type q to quit\n");
  refresh();
  while (CharacterStreamSingleton::get_instance().wait_key(1000) != 'q') {}

  // close ncurses
  endwin();
}
