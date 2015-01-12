#include "command.h"
#include "command-internals.h"

#include <error.h>

bool end = false;

typedef struct node node_t;

struct node {
  command_t m_command;
  node_t* m_next;
};

struct command_stream {
  node_t* m_head;
  node_t* m_curr;
  int stream_size;
};

/* Read a command, add it to the command_stream. Check that it is a valid
   command first before adding the command in. */

// Initialization
command_stream_t
init_command_stream()
{
  command_stream_t cmd_stream = (command_stream_t) checked_malloc(sizeof (struct command_stream));
  m_head = NULL;
  return cmd_stream;
}

// Syntax checks
bool is_word(char c)
{
  return isalpha(c) || isdigit(c) || (c == '!') || (c == '%')
                    || (c == '+') || (c == ',') || (c == '-')
                    || (c == '.') || (c == '/') || (c == ':')
                    || (c == '@') || (c == '^') || (c == '_');
}

bool is_token(char c)
{
  return (c == ';') || (c == '|') || (c == '(')
      || (c == ')') || (c == '<') || (c == '>');
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  int c = 0;
  int line = 1;
  int cmd_chars = 0;
  // initialize command stream
  command_stream_t cmd_stream = init_command_stream();
  // read in commmand
  while ((c = get_next_byte(get_next_byte_argument)) != EOF)
  {
    // check if special character (not part of the command)
    // tokens: ; | ( ) < >
    // comments: # not preceded by token, followed by chars up to (not including) the newline
    // whitespace: space, tab, newline
      // newline is special, able to substitute for semicolon
      // newline precedes ( ) if then else fi while do done until and first word of simple command
      // newline can follow any special token other than < >
    // check if valid character
      // add to current command node
      // if token or compound word, create new node
  }
  // verify that it's a valid command
  // add command to command stream
  return cmd_stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if (s->m_curr)
  {
    node_t cmd_node = s->m_curr;
    command_t cmd = s->m_curr->m_command;
    s->m_curr = s->m_curr->m_next;
    free(cmd_node);
    return cmd;
  }
  return NULL;
}