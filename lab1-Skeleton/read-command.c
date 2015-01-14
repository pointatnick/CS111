#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

typedef struct node* node_t;

// global variables
int cmd_read = 0;
int cmd_total = 0;
const char* if_cmd = "if ";
const char* fi_cmd = "fi "
const char* while_cmd = "while ";
const char* until_cmd = "until ";
const char* done_cmd = "done ";

struct node {
  command_t m_command;
  node_t m_next;
};

struct command_stream {
  node_t m_head;
  node_t m_curr;
  int stream_size;
};

// May need to be updated later
void insert_cmd(command_t cmd, command_stream_t cmd_stream)
{
  node_t new_node = (node_t) checked_malloc(sizeof(struct node));
  // make command
  if (cmd_stream->m_head == NULL)
  {
    cmd_stream->m_head = new_node;
    cmd_stream->m_curr = new_node;
  }
  else
  {
    cmd_stream->m_curr->m_next = new_node;
    cmd_stream->m_curr = new_node;
  }
  cmd_total++;
}

enum token_type
{
  IF_TOKEN,
  PIPE_TOKEN,
  SEQUENCE_TOKEN,
  SUBSHELL_TOKEN,
  UNTIL_TOKEN,
  WHILE_TOKEN,
};

/* command_t make_cmd(char *word, token_type cmd_type)
{

}

command_t simple_cmd(char **param)
{
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  // set simple command; what parameters to use?
  new_cmd->type = SIMPLE_COMMAND;
  new_cmd->status = -1;
  new_cmd->input = NULL;
  new_cmd->output = NULL;
  // set union stuff
  // new_cmd->union word = param
  return new_cmd;
}

command_t compound_cmd(command_t param)
{
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  // depending on if, until, while, sequence, pipe and set type appropriately
  new_cmd->status = -1;
  // set union stuff
  return new_cmd;
}

command_t subshell_cmd();
{
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = SUBSHELL_COMMAND;
  new_cmd->status = -1;
  // set union stuff
  return new_cmd;
} */

/* Read a command, add it to the command_stream. Check that it is a valid
   command first before adding the command in. */

// Initialization
command_stream_t
init_command_stream()
{
  command_stream_t cmd_stream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  cmd_stream->m_head = NULL;
  cmd_stream->m_curr = NULL;
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

// compound command keyword checks
bool if_check(const char *word)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[i] == if_cmd[i] || (i == 2 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool fi_check(const char *word, int iter)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[iter + i] == fi_cmd[i] || (i == 2 && (word[i] == '\n' || word[i] == EOF))
      continue;
    else return false;
  }
  return true;
}

bool while_check(const char *word)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[i] == if_cmd[i] || (i == 5 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool until_check(const char *word)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[i] == if_cmd[i] || (i == 5 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool done_check(const char *word)
{
  for(int i = 0; i < 5; i++)
  {
    if (word[i] == done_cmd[i] || (i == 2 && (word[i] == '\n' || word[i] == EOF))
      continue;
    else return false;
  }
  return true;
}

// print error message
void print_err(int line)
{
  fprintf(stderr, "%d: Incorrect syntax", line);
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  char c;
  char check_c;
  char* buffer;
  char* char_stream;
  char* check_stream[5];
  int buffer_size = 1024;
  int line = 1;
  int char_stream_iter = 0;
  bool newline_enc = false;
  enum token_type cmd_token;

  // initialize
  command_stream_t cmd_stream = init_command_stream();
  char_stream = (char*) checked_malloc(buffer_size);
  buffer = (char*) checked_malloc(buffer_size);
  // read in commmand
  for(int i = 0; (c = get_next_byte(get_next_byte_argument)) != EOF; i++)
  {
    if (i > buffer_size)
    {
      buffer_size += 1024;
      buffer = (char*) realloc(buffer, buffer_size);
    }
    buffer[i] = c;
  }

    // GENERAL OUTLINE:
    // Iterate over stream and process each line as an expression
    // Check the whole command stream if each expression taken in is valid

    // how do we take in lines as expressions?
    // make a character array that takes in the whole line until newline/EOF

    // tokens: ; | ( ) < >
    // comments: # not preceded by token, followed by chars up to (not including) the newline
    // whitespace: space, tab, newline
      // newline is special, able to substitute for semicolon
      // newline can only precede ( ) if then else fi while do done until and first word of simple command
      // newline can follow any special token other than < >
    // take a command
    // check syntax for command
      // if invalid syntax, report error and exit
    // add command to node
  for(int i = 0; i < strlen(buffer); i++)
  {
    char_stream[char_stream_iter] = buffer[i];
    // everytime we insert, reset after
    if (newline_enc)
    {
      switch(c)
      {
        case 'i':
          for (; char_stream_iter < 3; char_stream_iter++)
            char_stream[char_stream_iter] = buffer[i];
          if (if_check(char_stream))
            for(; i < strlen(buffer) && buffer[i] != EOF; i++)
            {
              char_stream[char_stream_iter] = buffer[i];
              if (buffer[i] == '\n')
              {
                if (fi_check(buffer, i + 1))
                {
                  char_stream[char_stream_iter + 1] = 'f';
                  char_stream[char_stream_iter + 2] = 'i';
                  //insert cmd into cmdstream and break
                }
                print_err(line);
              }
            }
        case 'w':
          for (; char_stream_iter < 6; char_stream_iter++)
            char_stream[char_stream_iter] = buffer[i];
          if (while_check(char_stream))
            for(; i < strlen(buffer) && buffer[i] != EOF; i++)
            {
              char_stream[char_stream_iter] = buffer[i];
              if (buffer[i] == '\n')
              {
                if (done_check(buffer, i + 1))
                {
                  char_stream[char_stream_iter + 1] = 'd';
                  char_stream[char_stream_iter + 2] = 'o';
                  char_stream[char_stream_iter + 3] = 'n';
                  char_stream[char_stream_iter + 4] = 'e';
                  //insert and reset and break
                }
                print_err(line);
              }
            }
        case 'u':
          for (; char_stream_iter < 6; char_stream_iter++)
            char_stream[char_stream_iter] = buffer[i];
          if (while_check(char_stream))
            for(; i < strlen(buffer) && buffer[i] != EOF; i++)
            {
              char_stream[char_stream_iter] = buffer[i];
              if (buffer[i] == '\n')
              {
                if (done_check(buffer, i + 1))
                {
                  char_stream[char_stream_iter + 1] = 'd';
                  char_stream[char_stream_iter + 2] = 'o';
                  char_stream[char_stream_iter + 3] = 'n';
                  char_stream[char_stream_iter + 4] = 'e';
                  //insert and reset and break
                }
                print_err(line);
              }
            }
        default:
          break;
      }
      newline_enc = false;
    }
    if (c == '\n')
    {
      line++;
      newline_enc = true;
      continue;
    }
    if (c == '#' && !last_char_token)
    {
      do
      {
        c = get_next_byte(get_next_byte_argument);
      } while(c != '\n' && c != EOF);
      if (c == '\n')
        line++;
    }
    if (is_token(c))
    {
      last_char_token = true;
      if (c == '(')
        left_parens++;
      if (c == ')')
        right_parens++;
    }
  }
  return cmd_stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if (cmd_read == cmd_total)
    return NULL;
  if (s->m_curr)
  {
    node_t cmd_node = s->m_curr;
    command_t cmd = s->m_curr->m_command;
    s->m_curr = s->m_curr->m_next;
    free(cmd_node);
    cmd_read++;
    return cmd;
  }
  fprintf(stderr, "Could not read command");
  exit(0);
}