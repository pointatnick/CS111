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

bool if_check(const char *word, int iter)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[iter + i] == if_cmd[i] || (i == 2 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

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

bool while_check(const char *word, int iter)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[iter + i] == if_cmd[i] || (i == 5 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool until_check(const char *word, int iter)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[iter + i] == if_cmd[i] || (i == 5 && word[i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool done_check(const char *word, int iter)
{
  for(int i = 0; i < 5; i++)
  {
    if (word[iter + i] == done_cmd[i] || (i == 2 && (word[i] == '\n' || word[i] == ';' || word[i] == '\0'))
      continue;
    else return false;
  }
  return true;
}

bool valid_char(char c)
{
  return (is_word(c)) || (is_token(c)) || (c == ' ') ||
         (c == '\n') || (c == '\t') || (c == '\0')
}

// Print error message
void print_err(int line)
{
  fprintf(stderr, "%d: Incorrect syntax", line);
}

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
  char* mini_buffer = (char*) checked_malloc(strlen(word));
  char* mini_char = (char*) checked_malloc(strlen(word));
  char* mini_check[6];
  int check;
  int it = 0;
  int counter = 0;
  mini_buffer = word;
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = cmd_type;
  new_cmd->status = -1;
  new_cmd->input = NULL;
  new_cmd->output = NULL;
  new_cmd->u.word = NULL;
  for(int i = 0; i < 3; i++)
    new_cmd->u.command[i] = NULL;
  switch(cmd_type)
  {
    case IF_COMMAND:
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 3; i < strlen(mini_buffer); i++)
      {
        // skip then/else
        if (mini_buffer[i] == 't')
          if (mini_buffer[i + 1] == 'h')
            if (mini_buffer[i + 2] == 'e')
              if (mini_buffer[i + 3] == 'n')
                if (mini_buffer[i + 4] == ' ' || mini_buffer[i + 4] == '\n')
                {
                  i += 5;
                  continue;
                }
        if (mini_buffer[i] == 'e')
          if (mini_buffer[i + 1] == 'l')
            if (mini_buffer[i + 2] == 's')
              if (mini_buffer[i + 3] == 'e')
                if (mini_buffer[i + 4] == ' ' || mini_buffer[i + 4] == '\n')
                {
                  i += 5;
                  continue;
                }
        mini_char[it] = mini_buffer[i];
        // if-while-until check
        check = i;
        switch(mini_buffer[i])
        {
          case 'i':
            for (int j = 0; j < 3; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (if_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 3; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (if_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (fi_check(mini_buffer, i))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 4; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 3;
                  i += 3;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, IF_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
          case 'w':
            for (int j = 0; j < 6; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (while_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 6; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (while_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (done_check(mini_buffer, i))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 6; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 6;
                  i += 6;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, WHILE_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
            case 'u':
              for (int j = 0; j < 6; j++)
              {
                mini_check[j] = mini_buffer[check];
                check++;
              }
              if (until_check(mini_check))
              {
                for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
                {
                  mini_char[it] = mini_buffer[i];
                  check = i;
                  for (int j = 0; j < 6; j++)
                  {
                    mini_check[j] = mini_buffer[check];
                    check++;
                  }
                  if (until_check(mini_check))
                    counter++;
                  if (mini_char[it] == '\n')
                    if (done_check(mini_buffer, i))
                      counter--;
                  if (counter == 0)
                  {
                    for (int k = 1; k < 6; k++)
                      mini_char[it + k] = mini_buffer[i + k];
                    it += 6;
                    i += 6;
                    for (int i = 0; i < 3; i++)
                      if (new_cmd->u.command[i] == NULL)
                      {
                        new_cmd->u.command[i] = make_cmd(mini_char, UNTIL_COMMAND);
                        break;
                      }
                    memset(mini_char, '\0', strlen(mini_char));
                    memset(mini_check, '\0' strlen(mini_check));
                    it = 0;
                    break;
                  }
                  it++;
                }
              }
            default:
              break;
          }
        }
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          if (it > 0)
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (mini_char[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
    case UNTIL_COMMAND:
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 3; i < strlen(mini_buffer); i++)
      {
        // skip do
        if (mini_buffer[i] == 'd')
          if (mini_buffer[i + 1] == 'o')
            if (mini_buffer[i + 2] == ' ' || mini_buffer[i + 2] == '\n')
            {
              i += 3;
              continue;
            }
        mini_char[it] = mini_buffer[i];
        // if-while-until check
        check = i;
        switch(buffer[i])
        {
          case 'i':
            for (int j = 0; j < 3; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (if_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 3; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (if_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (fi_check(mini_buffer, i + 1))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 4; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 3;
                  i += 3;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, IF_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
          case 'w':
            for (int j = 0; j < 6; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (while_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 6; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (while_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (done_check(mini_buffer, i + 1))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 6; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 6;
                  i += 6;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, WHILE_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
            case 'u':
              for (int j = 0; j < 6; j++)
              {
                mini_check[j] = mini_buffer[check];
                check++;
              }
              if (until_check(mini_check))
              {
                for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
                {
                  mini_char[it] = mini_buffer[i];
                  check = i;
                  for (int j = 0; j < 6; j++)
                  {
                    mini_check[j] = mini_buffer[check];
                    check++;
                  }
                  if (until_check(mini_check))
                    counter++;
                  if (mini_char[it] == '\n')
                    if (done_check(mini_buffer, i + 1))
                      counter--;
                  if (counter == 0)
                  {
                    for (int k = 1; k < 6; k++)
                      mini_char[it + k] = mini_buffer[i + k];
                    it += 6;
                    i += 6;
                    for (int i = 0; i < 3; i++)
                      if (new_cmd->u.command[i] == NULL)
                      {
                        new_cmd->u.command[i] = make_cmd(mini_char, UNTIL_COMMAND);
                        break;
                      }
                    memset(mini_char, '\0', strlen(mini_char));
                    memset(mini_check, '\0' strlen(mini_check));
                    it = 0;
                    break;
                  }
                  it++;
                }
              }
            default:
              break;
          }
        }
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          if (it > 0)
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (char_stream[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
    case WHILE_COMMAND:
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 3; i < strlen(mini_buffer); i++)
      {
        // skip do
        if (mini_buffer[i] == 'd')
          if (mini_buffer[i + 1] == 'o')
            if (mini_buffer[i + 2] == ' ' || mini_buffer[i + 2] == '\n')
            {
              i += 2;
              continue;
            }
        mini_char[it] = mini_buffer[i];
        // if-while-until check
        check = i;
        switch(buffer[i])
        {
          case 'i':
            for (int j = 0; j < 3; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (if_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 3; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (if_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (fi_check(mini_buffer, i + 1))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 4; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 3;
                  i += 3;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, IF_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
          case 'w':
            for (int j = 0; j < 6; j++)
            {
              mini_check[j] = mini_buffer[check];
              check++;
            }
            if (while_check(mini_check))
            {
              for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
              {
                mini_char[it] = mini_buffer[i];
                check = i;
                for (int j = 0; j < 6; j++)
                {
                  mini_check[j] = mini_buffer[check];
                  check++;
                }
                if (while_check(mini_check))
                  counter++;
                if (mini_char[it] == '\n')
                  if (done_check(mini_buffer, i + 1))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 1; k < 6; k++)
                    mini_char[it + k] = mini_buffer[i + k];
                  it += 6;
                  i += 6;
                  for (int i = 0; i < 3; i++)
                    if (new_cmd->u.command[i] == NULL)
                    {
                      new_cmd->u.command[i] = make_cmd(mini_char, WHILE_COMMAND);
                      break;
                    }
                  memset(mini_char, '\0', strlen(mini_char));
                  memset(mini_check, '\0' strlen(mini_check));
                  it = 0;
                  break;
                }
                it++;
              }
            }
            case 'u':
              for (int j = 0; j < 6; j++)
              {
                mini_check[j] = mini_buffer[check];
                check++;
              }
              if (until_check(mini_check))
              {
                for(; i < strlen(mini_buffer) && mini_buffer[i] != '\0'; i++)
                {
                  mini_char[it] = mini_buffer[i];
                  check = i;
                  for (int j = 0; j < 6; j++)
                  {
                    mini_check[j] = mini_buffer[check];
                    check++;
                  }
                  if (until_check(mini_check))
                    counter++;
                  if (mini_char[it] == '\n')
                    if (done_check(mini_buffer, i + 1))
                      counter--;
                  if (counter == 0)
                  {
                    for (int k = 1; k < 6; k++)
                      mini_char[it + k] = mini_buffer[i + k];
                    it += 6;
                    i += 6;
                    for (int i = 0; i < 3; i++)
                      if (new_cmd->u.command[i] == NULL)
                      {
                        new_cmd->u.command[i] = make_cmd(mini_char, UNTIL_COMMAND);
                        break;
                      }
                    memset(mini_char, '\0', strlen(mini_char));
                    memset(mini_check, '\0' strlen(mini_check));
                    it = 0;
                    break;
                  }
                  it++;
                }
              }
            default:
              break;
          }
        }
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          if (it > 0)
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (mini_char[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
    case PIPE_COMMAND:
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 0; i < strlen(mini_buffer); i++)
      {
        mini_char[it] = mini_buffer[i];
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
              break;
            }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (char_stream[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
    case SEQUENCE_COMMAND:
      // Inside a sequence command can be subshell, pipe, seqeuence, simple 
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 0; i < strlen(mini_buffer); i++)
      {
        mini_char[it] = mini_buffer[i];
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          if (it > 0)
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (char_stream[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
    case SUBSHELL_COMMAND:
      // Inside a subshell command can be subshell, pipe, sequence, simple
      new_cmd->u.word[0] = mini_buffer;
      for (int i = 1; i < strlen(mini_buffer); i++)
      {
        mini_char[it] = mini_buffer[i];
        // newline behavior
        if (mini_char[it] == '\n')
        {
          if (it == 0)
            continue;
          if (it > 0)
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; mini_char[it] != '\n' && mini_char[it] != ' ' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
            sc_marker++;
          }
          if (sc_marker > 1)
          {
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SEQUENCE_COMMAND);
                break;
              }
            memset(mini_char, '\0', strlen(mini_char));
            it = 0;
            break;
          }
          else
            for (int i = 0; i < 3; i++)
              if (new_cmd->u.command[i] == NULL)
              {
                new_cmd->u.command[i] = make_cmd(mini_char, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
        {
          for(; mini_char[it] != '\n' && mini_char[it] != ';' && mini_char[it] != '\0'; it++)
          {
            mini_char[it] = mini_buffer[i];
            i++;
          }
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, PIPE_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
          continue;
        }
        // subshell behavior
        if (char_stream[it] == '(')
        {
          for (int i = 0; i < 3; i++)
            if (new_cmd->u.command[i] == NULL)
            {
              new_cmd->u.command[i] = make_cmd(mini_char, SUBSHELL_COMMAND);
              break;
            }
          memset(mini_char, '\0', strlen(mini_char));
          it = 0;
        }
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
  new_cmd->u.word[0] = word;
  return new_cmd;
}

// Initialization
command_stream_t
init_command_stream()
{
  command_stream_t cmd_stream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  cmd_stream->m_head = NULL;
  cmd_stream->m_curr = NULL;
  return cmd_stream;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    // GENERAL OUTLINE:
    // Check entire file for syntax errors
      // If invalid, print error messages and exit
    // Pass file to buffer for command creation/insertion
    // Handle redirects after command stream creation

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
  int done_count = 0;
  int do_count = 0;
  int right_paren_count = 0;
  int left_paren_count = 0;
  int paren_count = 0;
  int while_until_counter = 0;
  int else_count = 0;
  int then_count = 0;
  bool found_right_paren = false;
  bool found_command = false;

  // Initialize
  command_stream_t cmd_stream = init_command_stream();
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

  // Syntax checking
  // Invalid characters
  for(int i = 0; i < buffer_size; i++)
  {
    if(buffer[i] == '\n')
    line++;
    if (!valid_char(buffer_size[i]))
    {
      print_err(line);
      printf("invalid character found");
      // exit
    }
  } 
  // Checking for do done
  line = 1;
  for(int i = 0; i < buffer_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'd')
    {
      if(buffer[i+1] != '\0' && buffer[i+1] == 'o')
      {
        if(buffer[i+2] != '\0' && buffer[i+2] == 'n')
          if(buffer[i+3] != '\0' && buffer[i+3] == 'e')
            done_count++;
        if(buffer[i+2] != '\0' && (buffer[i+2] == '\n' || buffer[i+2] == ' '))
          do_count++;
      }
    }    
    if(done_count > while_until_counter || do_count > while_until_counter)
    {
      print_err(line);
      printf("do or done found before while or until");
      //exit
    }      
    if(buffer[i] == 'w')
      if(while_check(buffer,i))
        while_until_counter++;
    if(buffer[i] == 'u')
      if(until_check(buffer,i))
        while_until_counter++;
    if(while_until_counter != do_count || while_until_counter != done_count)
    {
        print_err(line);
        printf("more do or dones found than total while/untils");
        //exit
    }
  }
  // Checking for fi
  line = 1;
  for(int i = 0; i < buffer_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'f')
      if(buffer[i+1] != '\0' && buffer[i+1] == 'i')
        if(buffer[i+2] != '\0' && (buffer[i+2] == '\n' || buffer[i+2] == ' '))
          fi_count++;
    if(fi_count > if_count)
    {
      print_err(line);
      printf("fi found before if");
      //exit
    }
    if(buffer[i] == 'i')
      if(if_check(buffer,i))
        if_count++;
  }
    
  if(fi_count != if_count)
  {
    print_err(line);
    printf("number of fi's not equal to number of if's");
    //exit
  }
    
  // checking if if then else have commands after them before a semicolon or newline
  line = 1;
  for(int i = 0; i < buffer_size;)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'i')
    {
      if(if_check(buffer, i))
      {
        i += 2;
        // checking if there is a command or if between an if and its semicolon or newline
        if (if_check(buffer, i))
        {
          while (buffer[i] != '\0')
          {
            if(buffer[i] == '\n')
              line++;
            if(is_word(buffer[i]))
            {
              if((buffer[i] == 'e' && buffer[i+1] == 'l' &&
                  buffer[i+2] == 's' && buffer[i+3] == 'e') ||
                  (buffer[i] == 't' && buffer[i+1] == 'h' &&
                  buffer[i+2] == 'e' && buffer[i+3] == 'n'))
              {
                print_err(line);
                printf("found else/then after if");
                //exit
              }
              else
              {
                found_command = true;
                break;
              }
            }
            if (buffer[i] == ';' || buffer[i] == '\n')
              if(!found_command)
              {
                print_err(line);
                printf("did not find command after an if");
                //exit
              }
            i++;
          }
        }
        // checking if there is a command between else and its semicolon or newline
        if(buffer[i]] == 'e')
          if(buffer[i+1] == 'l')
            if(buffer[i+2] == 's')
              if(buffer[i+3] == 'e')
              {
                else_count++;
                i += 4;
                if(else_count > then_count)
                {
                  print_err(line)
                  printf("found an else before a then");
                  //exit
                }
                while(buffer[i] != '\0')
                {
                  if(buffer[i] == '\n')
                    line++;
                  if(is_word(buffer[i]))
                  {
                    if((buffer[i] == 'e' && buffer[i+1] == 'l' &&
                        buffer[i+2] == 's' && buffer[i+3] == 'e') ||
                       (buffer[i] == 't' && buffer[i+1] == 'h' &&
                        buffer[i+2] == 'e' && buffer[i+3] == 'n'))
                    {
                      print_err(line);
                      printf("found else/then after else");
                      //exit
                    }
                    else
                    {
                      found_command = true;
                      break;
                    }
                  }
                  if (buffer[i] == ';' || buffer[i] == '\n')
                    if(!found_command)
                    {
                      print_err(line);
                      printf("did not find command after an else");
                      //exit
                    }
                  i++;
                }
              }
        // checking if there is a command between then and its semicolon or newline
        if(buffer[i] == 't')
          if(buffer[i+1] == 'h')
            if(buffer[i+2] == 'e')
              if(buffer[i+3] == 'n')
              {
                then_count++;
                i += 4;
                while(buffer[i] != '\0')
                { // ?
                  if(buffer[i] == '\n')
                    line++;
                  if(is_word(buffer[i]))
                  { 
                    if((buffer[i] == 'e' && buffer[i+1] == 'l' &&
                        buffer[i+2] == 's' && buffer[i+3] == 'e') ||
                       (buffer[i] == 't' && buffer[i+1] == 'h' &&
                        buffer[i+2] == 'e' && buffer[i+3] == 'n'))
                    {
                      print_err(line);
                      printf("found else/then after then");
                      //exit
                    }
                    else
                    {
                      found_command = true;
                      break;
                    }
                  }
                  if (buffer[i] == ';'|| buffer[i] == '\n')
                    if(!found_command)
                    {
                      print_err(line);
                      printf("did not find command after a then");
                      //exit
                    }
                  i++;
                }
              }
      }
    }
  }
  if(then_count > if_count)
  {
    print_err(line);
    printf("found more then's than ifs")
    //exit
  }
  if(else_count > if_count)
  {
    print_err(line);
    printf("found more else's than ifs");
    //exit
  }
  //subshell syntax
  line = 1;
  for(int i = 0; i < buffer_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == ')')
    {
      right_paren_count++;
      if(right_paren_count > left_paren_count)
      {
        print_err(line);
        printf("found right parentheses before left");
        //exit
      }
    }
    if (buffer[i] == '(')
      left_paren_count++;
    if (buffer[i] == ')' && buffer[i - 1] == '(')
    {
      print_err(line);
      printf("no command in subshell");
      //exit
    }  
  }
  if(right_paren_count != left_paren_count)
  {
    print_err(line);
    printf("found unclosed left parentheses");
    //exit
  }
  // pipe syntax
  line = 1;
  for(int i = 0; i < buffer_size; i++)
  {
    if (buffer[i] == '\n')
      line++;
    if(buffer[i] == '|')
    {
      if(i == 0)
      {
        print_err(line);
        printf("found pipe in beginning of file");
        //exit
      }
      //checking for invalidness before and after pipe
      for(int k = i + 1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
        if(buffer[k] != ' ')
          if(buffer[k] == ';' || buffer[k] == '|' || buffer[k] == ')')
          {
            print_err(line);
            printf("found invalid token or no command after pipe");
            //exit
          }
      for(int z = i-1; z > -1; z--)
        if(buffer[z] != ' ')
          if(buffer[z] != ')' || z == 0 || !is_word(buffer[z]))
          {
            print_err(line);
            printf("found a newline or invalid token before pipe");
            //exit
          }
    }
  }
  //semicolon syntax
  line = 1;
  for(int i = 0; i < buffer_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if (buffer[i] == ';')
    {
      if (i == 0)
      {
        print_err(line);
        printf("found semicolon at beginning of file");
        // exit
      }
      //checking for validity before and after semicolon
      for(int k = i+1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
        if(buffer[k] != ' ')
          if(buffer[k] == ';' || buffer[k] == '|' || buffer[k] == ')')
          {
            print_err(line);
            printf("found invalid token or no command after semicolon");
            //exit
          }
      for(int z = i-1; z > -1; z--)
        if(buffer[z]!= ' ')
          if(buffer[z] != ')' || z == 0 || !(is_word(buffer[z])))
          {
            print_err(line);
            printf("found a newline or invalid token before semicolon");
            //exit
          }
    }
  }
  char_stream = (char*) checked_malloc(buffer_size);
  for(int i = 0; i < strlen(buffer); i++)
  {
    char_stream[it] = buffer[i];
    // if-while-until check
    if (char_stream[it] == 'i' || char_stream[it] == 'w' || char_stream[it] == 'u')
    {
      check = i;
      switch(char_stream[it])
      {
        case 'i':
          if (if_check(buffer, i))
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
                if (fi_check(buffer, i + 1))
                  counter--;
              }
              if (counter == 0)
              {
                for (int k = 1; k < 4; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 3;
                i += 3;
                insert_cmd(char_stream, IF_COMMAND, cmd_stream);
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
                if (done_check(buffer, i + 1))
                  counter--;
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 6;
                i += 6;
                insert_cmd(char_stream, WHILE_COMMAND, cmd_stream);
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
                if (done_check(buffer, i + 1))
                  counter--;
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                it += 6;
                i += 6;
                insert_cmd(char_stream, UNTIL_COMMAND, cmd_stream);
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
      if (it > 0 && char_stream[it - 1] == '\n')
        insert_cmd(char_stream, SIMPLE_COMMAND, cmd_stream);
    // semi-colon behavior
    if (char_stream[it] == ';')
    {
      for(; char_stream[it] != '\n' && char_stream[it] != ' ' && char_stream[it] != '\0'; it++)
      {
        char_stream[it] = buffer[i];
        i++;
        sc_marker++;
      }
      if (sc_marker > 1)
      {
        insert_cmd(char_stream, SEQUENCE_COMMAND, cmd_stream);
        memset(char_stream, '\0', strlen(char_stream));
        break;
      }
      else insert_cmd(char_stream, SIMPLE_COMMAND, cmd_stream);
    }
    // pipe behavior
    if (char_stream[it] == '|')
    {
      for(; char_stream[it] != '\n' && char_stream[it] != ';' && char_stream[it] != '\0'; it++)
      {
        char_stream[it] = buffer[i];
        i++;
      }
      insert_cmd(char_stream, PIPE_COMMAND, cmd_stream);
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
        if (char_stream[it] == '(')
          paren_count++;
        if (char_stream[it] == ')')
          paren_count--;
        if (paren_count == 0)
          found_right_paren = true;
      }
      insert_cmd(char_stream, SUBSHELL_COMMAND, cmd_stream);
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