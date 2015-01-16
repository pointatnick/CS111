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

// Global variables
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
void insert_cmd(char *word, command_type cmd_type, command_stream_t cmd_stream)
{
  node_t new_node = (node_t) checked_malloc(sizeof(struct node));
  command_t new_cmd = make_cmd(word, cmd_type);
  new_node->m_command = new_cmd; 
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

command_t make_cmd(char *word, command_type cmd_type)
{
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = cmd_type;
  switch(cmd_type)
  {
    case IF_COMMAND:
    case UNTIL_COMMAND:
    case WHILE_COMMAND:
    case PIPE_COMMAND:
      // Inside a pipe command can be subshell, pipe, sequence, simple
    case SEQUENCE_COMMAND:
      // Inside a sequence command can be subshell, pipe, seqeuence, simple 
    case SUBSHELL_COMMAND:
      // Inside a subshell command can be subshell, pipe, sequence, simple
    default:
      new_cmd = make_simple_cmd(word)
  }
  return new_cmd;
}

command_t make_simple_cmd(char *word)
{
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = SIMPLE_COMMAND;
  new_cmd->status = -1;
  new_cmd->input = NULL;
  new_cmd->output = NULL;
  new_cmd->u.word = word;
  return new_cmd;
}

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

// Needs updating
bool fi_check(const char *word, int iter)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[iter + i] == fi_cmd[i] || (i == 2 && (word[i] == '\n' || word[i] == ';' || word[i] == '\0'))
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
    if (word[i] == done_cmd[i] || (i == 2 && (word[i] == '\n' || word[i] == ';' || word[i] == '\0'))
      continue;
    else return false;
  }
  return true;
}

// Print error message
void print_err(int line)
{
  fprintf(stderr, "%d: Incorrect syntax", line);
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  char c;
  char* buffer;
  char* char_stream;
  char* check_stream[6];
  int buffer_size = 1024;
  int line = 1;
  int it = 0;
  int counter = 0;
  int check;
  int sc_marker = 0;
  int paren_count = 1;
  bool found_right_paren = false;
  bool newline_enc = false;
  enum command_type cmd_type;

  // Initialize
  command_stream_t cmd_stream = init_command_stream();
  char_stream = (char*) checked_malloc(buffer_size);
  buffer = (char*) checked_malloc(buffer_size);
  // Copy file to buffer
  for(int i = 0; (c = get_next_byte(get_next_byte_argument)) != EOF; i++)
  {
    if (i > buffer_size)
    {
      buffer_size += 1024;
      buffer = (char*) checked_realloc(buffer, buffer_size);
    }
    buffer[i] = c;
  }

    // GENERAL OUTLINE:
    // Iterate over stream and process each line as an expression
    // Check the whole command stream if each expression taken in is valid

    // check all commands for syntax
      // if invalid syntax, report error and exit
    // do a second pass and handle redirects
    // note: fix allocation for char_stream

  for(int i = 0; i < strlen(buffer); i++)
  {
    char_stream[it] = buffer[i];
    /* When we hit a newline, check for if, while, until keywords.
       If we encounter a proper if, while, until, continue
       storing bytes into char_stream until you hit corresponding
       fi or done. Then, insert char_stream into a command node
       and reset. */
    // character check
    if (!is_word(char_stream[it] && !is_token(char_stream[it]) &&
        char_stream[it] != ' ' && char_stream[it] != '\n' &&
        char_stream[it] != '\t' && char_stream[it] != '\0')
    {
      print_err(line);
      // exit
    }
    // if-while-until check
    if (newline_enc || i == 0)
    {
      check = i;
      switch(char_stream[it])
      {
        case 'i':
          for (int j = 0; j < 3; j++)
          {
            check_stream[j] = buffer[check];
            check++;
          }
          if (if_check(check_stream))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 3; j++)
              {
                check_stream[j] = buffer[check];
                check++;
              }
              if (if_check(check_stream))
                counter++;
              if (char_stream[it] == '\n')
              {
                line++;
                if (fi_check(buffer, i + 1))
                  counter--;
              }
              if (counter == 0)
              {
                for (int k = 1; k < 4; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 3;
                i += 3;
                cmd_type = IF_COMMAND;
                insert_cmd(char_stream, cmd_type, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0' strlen(check_stream));
                break;
              }
              it++;
            }
            if (char_stream[it] == '\0' && counter != 0)
            {
              print_err(line);
              // exit
            }
          }
        case 'w':
          for (int j = 0; j < 6; j++)
          {
            check_stream[j] = buffer[check];
            check++;
          }
          if (while_check(check_stream))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 6; j++)
              {
                check_stream[j] = buffer[check];
                check++;
              }
              if (while_check(check_stream))
                counter++;
              if (char_stream[it] == '\n')
              {
                line++;
                if (done_check(buffer, i + 1))
                  counter--;
              }
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 6;
                i += 6;
                cmd_type = WHILE_COMMAND;
                insert_cmd(char_stream, cmd_type, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0' strlen(check_stream));
                break;
              }
              it++;
            }
            if (char_stream[it] == '\0' && counter != 0)
            {
              print_err(line);
              // exit
            }
          }
        case 'u':
          for (int j = 0; j < 6; j++)
          {
            check_stream[j] = buffer[check];
            check++;
          }
          if (until_check(check_stream))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 6; j++)
              {
                check_stream[j] = buffer[check];
                check++;
              }
              if (until_check(check_stream))
                counter++;
              if (char_stream[it] == '\n')
              {
                line++;
                if (done_check(buffer, i + 1))
                  counter--;
              }
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 6;
                i += 6;
                cmd_type = WHILE_COMMAND;
                insert_cmd(char_stream, cmd_type, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0' strlen(check_stream));
                break;
              }
              it++;
            }
            if (char_stream[it] == '\0' && counter != 0)
            {
              print_err(line);
              // exit
            }
          }
        default:
          break;
      }
      newline_enc = false;
    }
    // comment behavior
    if (char_stream[it] == '#')
      if (it == 0 || !is_word(char_stream[it - 1]))
        for(; char_stream[it] != '\n'; it++)
        {
          char_stream[it] == buffer[i];
          i++;
        }
    // newline behavior
    if (char_stream[it] == '\n')
    {
      line++;
      newline_enc = true;
      continue;
    }
    // semi-colon behavior
    if (char_stream[it] == ';')
    {
      if (it == 0)
      {
        print_err(line);
        // exit
      }
      for(; char_stream[it] != '\n' && char_stream[it] != ' ' && char_stream[it] != '\0'; it++)
      {
        if (char_stream[it - 1] == ';' || char_stream[it - 1] == '\n' ||
            char_stream[it - 1] == '|' || char_stream[it - 1] == '<' ||
            char_stream[it - 1] == '>' || char_stream[it - 1] == '(' ||
            char_stream[it - 1] == ' ' || char_stream[it - 1] == '\t')
        {
          print_err(line);
          // exit
        }
        char_stream[it] = buffer[i];
        i++;
        sc_marker++;
      }
      if (sc_marker > 1)
      {
        cmd_type = SEQUENCE_COMMAND;
        insert_cmd(char_stream, cmd_type, cmd_stream);
        memset(char_stream, '\0', strlen(char_stream));
        break;
      }
      else
        make_simple_cmd(char_stream);
    }
    // pipe behavior
    if (char_stream[it] == '|')
    {
      if (it == 0)
      {
        print_err(line);
        // exit
      }
      for(; char_stream[it] != '\n' && char_stream[it] != ';' && char_stream[it] != '\0'; it++)
      {
        char_stream[it] = buffer[i];
        i++;
      }
      cmd_type = PIPE_COMMAND;
      insert_cmd(char_stream, cmd_type, cmd_stream);
      memset(char_stream, '\0', strlen(char_stream));
      continue;
    }
    // subshell behavior
    if (char_stream[it] == '(')
    {
      paren_count++;
      while(!found_right_paren)
      {
        it++;
        i++;
        char_stream[it] = buffer[i];
        if(char_stream[it] == '(')
          paren_count++;
        if(char_stream[it] == ')')
          paren_count--;
        if(paren_count == 0)
          found_right_paren = true;
        if(char_stream[it] == '\n' || char_stream[it] == '\0')
        {
          print_err(line);
          // exit
        }
      }
      cmd_type = SUBSHELL_COMMAND;
      insert_cmd(char_stream, cmd_type, cmd_stream);
      memset(char_stream, '\0', strlen(char_stream));
    }
    it++;
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