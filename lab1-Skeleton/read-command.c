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
const char* fi_cmd = "fi ";
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

void free_cmd(command_t cmd)
{
  if (cmd == NULL)
    return;
  if (cmd->u.word)
  {
    free(cmd->u.word);
    cmd->u.word = NULL;
  }
  for (int i = 0; i < 3; i++)
    if (cmd->u.command[i])
    {
      free_cmd(cmd->u.command[i]);
      cmd->u.command[i] = NULL;
    }
  free(cmd);
  cmd = NULL;
  return;
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

bool if_check(const char *word, int iter)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[iter + i] == if_cmd[i] || (i == 2 && word[iter + i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool fi_check(const char *word, int iter)
{
  for(int i = 0; i < 3; i++)
  {
    if (word[iter + i] == fi_cmd[i] || (i == 2 && (word[iter + i] == '\n' || word[iter + i] == ';' || word[iter + i] == '\0')))
      continue;
    else return false;
  }
  return true;
}

bool while_check(const char *word, int iter)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[iter + i] == while_cmd[i] || (i == 5 && word[iter + i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool until_check(const char *word, int iter)
{
  for(int i = 0; i < 6; i++)
  {
    if (word[iter + i] == until_cmd[i] || (i == 5 && word[iter + i] == '\n'))
      continue;
    else return false;
  }
  return true;
}

bool done_check(const char *word, int iter)
{
  for(int i = 0; i < 5; i++)
  {
    if (word[iter + i] == done_cmd[i] || (i == 4 && (word[iter + i] == '\n' || word[iter + i] == '\t' || word[iter + i] == '\0')))
      continue;
    else return false;
  }
  return true;
}

bool valid_char(char c)
{
  return (is_word(c)) || (is_token(c)) || (c == ' ') ||
         (c == '\n') || (c == '\t') || (c == '\0') || (c == '#');
}

// Print error message
void print_err(int line)
{
  fprintf(stderr, "%d: Incorrect syntax", line);
  exit(-1);
}

command_t make_simple_cmd(char *word)
{
  char* input_stream = (char*) checked_malloc(strlen(word)+1);
  char* output_stream = (char*) checked_malloc(strlen(word)+1);
  char* cmd_word = (char*) checked_malloc(strlen(word)+1);
  size_t p = 0;
  size_t inpos = 0;
  size_t outpos = 0;
  if (word[0] == '\n' || word[0] == ' ' || word[0] == '\t')
    for (; !is_word(word[p]) && !is_token(word[p]); p++);
  for (size_t i = 0; word[p] != ';' && word[p] != '\0' && word[p] != '\n'; i++, p++)
    cmd_word[i] = word[p];
  bool input_filled = false;
  bool output_filled = false;
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = SIMPLE_COMMAND;
  new_cmd->status = -1;
  for (size_t i = 0; i < strlen(cmd_word); i++)
    if (cmd_word[i] == '<')
    {
      if (cmd_word[i+1] == ' ')
        for (size_t m = i; cmd_word[m] != '\0'; m++)
          cmd_word[m+1] = cmd_word[m+2];
      if (cmd_word[i-1] == ' ')
        for (size_t m = i-1; cmd_word[m] != '\0'; m++)
          cmd_word[m] = cmd_word[m+1];
      for (i = 0; cmd_word[i] != '<'; i++);
      inpos = i;
      int k = 0;
      for (size_t j = i+1; j < strlen(cmd_word); j++, k++)
      {
        if (cmd_word[j] == ' ' || cmd_word[j] == '\n' ||
            cmd_word[j] == '>' || cmd_word[j] == '\t')
          break;
        else
        {
          input_stream[k] = cmd_word[j];
          input_filled = true;
        }
      }
      input_stream[k+1] = '\0';
      break;
    }
  for (size_t i = 0; i < strlen(cmd_word); i++)
    if (cmd_word[i] == '>')
    {
      if (cmd_word[i+1] == ' ')
        for (size_t m = i; cmd_word[m] != '\0'; m++)
          cmd_word[m+1] = cmd_word[m+2];
      if (cmd_word[i-1] == ' ')
        for (size_t m = i-1; cmd_word[m] != '\0'; m++)
          cmd_word[m] = cmd_word[m+1];
      for (i = 0; cmd_word[i] != '>'; i++);
      outpos = i;
      int k = 0;
      for (size_t j = i+1; j < strlen(cmd_word); j++, k++)
      {
        if (cmd_word[j] == ' ' || cmd_word[j] == '\n' || cmd_word[j] == '\t')
          break;
        else
        {
          output_stream[k] = cmd_word[j];
          output_filled = true;
        }
      }
      output_stream[k+1] = '\0';
      break;
    }
  if (input_filled)
    cmd_word[inpos] = '\0';
  else if (output_filled)
    cmd_word[outpos] = '\0';
  new_cmd->u.word = (char**) checked_malloc(sizeof(char*));
  *new_cmd->u.word = cmd_word;
  if (input_filled)
    new_cmd->input = input_stream;
  else
    new_cmd->input = NULL;
  if (output_filled)
    new_cmd->output = output_stream;
  else
    new_cmd->output = NULL;
//  if(input_stream)
//  {
//    free(input_stream);
//    input_stream = NULL;
//  }
//  if(output_stream)
//  {
//    free(output_stream);
//    output_stream = NULL;
//  }
  return new_cmd;
}

command_t make_cmd(char *word, enum command_type cmd_type)
{
  char* mini_char = (char*) checked_malloc(strlen(word) + 1);
  char* subshell_cmd = (char*) checked_malloc(strlen(word) + 1);
  char mini_check[6];
  int check = 0;
  int it = 0;
  int counter = 0;
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  new_cmd->type = cmd_type;
  new_cmd->status = -1;
  new_cmd->input = NULL;
  new_cmd->output = NULL;
  size_t w_it = 0;
  if (word[0] == '\n' || word[0] == ' ' || word[0] == '\t')
    for (; !is_word(word[w_it]) && !is_token(word[w_it]); w_it++);
  switch(cmd_type)
  {
    case IF_COMMAND:
    {
      bool pipe_found = false;
      bool sequence_found = false;
      bool if_found = false;
      bool while_found = false;
      bool until_found = false;
      bool subshell_made = false;
      bool input_filled = false;
      bool output_filled = false;
      char* input_stream = (char*) checked_malloc(strlen(word)+1);
      char* output_stream = (char*) checked_malloc(strlen(word)+1);
      int sub_count = 0;
      for(int h = 0; h < 3; h++)
        new_cmd->u.command[h] = NULL;
      for (w_it += 2; w_it < strlen(word); w_it++, it++)
      {
        // then/else/fi to delimit commands
        if (word[w_it] == 't')
          if (word[w_it + 1] == 'h')
            if (word[w_it + 2] == 'e')
              if (word[w_it + 3] == 'n')
                if (word[w_it + 4] == ' ' || word[w_it + 4] == '\n')
                {
                  if (subshell_made)
                    break;
                  if (sequence_found)
                  {
                    new_cmd->u.command[0] = make_cmd(mini_char, SEQUENCE_COMMAND);
                    sequence_found = false;
                  }
                  else if (pipe_found)
                  {
                    new_cmd->u.command[0] = make_cmd(mini_char, PIPE_COMMAND);
                    pipe_found = false;
                  }
                  else if (if_found)
                  {
                    new_cmd->u.command[0] = make_cmd(mini_char, IF_COMMAND);
                    if_found = false;
                  }
                  else if (while_found)
                  {
                    new_cmd->u.command[0] = make_cmd(mini_char, WHILE_COMMAND);
                    while_found = false;
                  }
                  else if (until_found)
                  {
                    new_cmd->u.command[0] = make_cmd(mini_char, UNTIL_COMMAND);
                    until_found = false;
                  }
                  else
                    new_cmd->u.command[0] = make_cmd(mini_char, SIMPLE_COMMAND);
                  memset(mini_char, '\0', strlen(mini_char));
                  it = -1;
                  w_it += 3;
                  continue;
                }
        if (word[w_it] == 'e')
          if (word[w_it + 1] == 'l')
            if (word[w_it + 2] == 's')
              if (word[w_it + 3] == 'e')
                if (word[w_it + 4] == ' ' || word[w_it + 4] == '\n')
                {
                  if (subshell_made)
                    break;
                  if (sequence_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, SEQUENCE_COMMAND);
                    sequence_found = false;
                  }
                  else if (pipe_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, PIPE_COMMAND);
                    pipe_found = false;
                  }
                  else if (if_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, IF_COMMAND);
                    if_found = false;
                  }
                  else if (while_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, WHILE_COMMAND);
                    while_found = false;
                  }
                  else if (until_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, UNTIL_COMMAND);
                    until_found = false;
                  }
                  else
                    new_cmd->u.command[1] = make_cmd(mini_char, SIMPLE_COMMAND);
                  memset(mini_char, '\0', strlen(mini_char));
                  it = -1;
                  w_it += 3;
                  continue;
                }
        if (word[w_it] == 'f')
          if (word[w_it+1] == 'i')
            if (word[w_it+2] == ' ' || word[w_it+2] == '\n' ||
                word[w_it+2] == '\0')
            {
              for (size_t r = w_it+1; r < strlen(word); r++)
                if (word[r] == '<')
                {
                  if (word[r+1] == ' ')
                    for (size_t m = r; word[m] != '\0'; m++)
                      word[m+1] = word[m+2];
                  if (word[r-1] == ' ')
                    for (size_t m = r-1; word[m] != '\0'; m++)
                      word[m] = word[m+1];
                  for (r = 0; word[r] != '<'; r++);
                  int k = 0;
                  for (size_t j = r+1; j < strlen(word); j++, k++)
                  {
                    if (word[j] == ' ' || word[j] == '\n' ||
                        word[j] == '\t')
                      break;
                    else
                    {
                      input_stream[k] = word[j];
                      input_filled = true;
                    }
                  }
                  input_stream[k+1] = '\0';
                  break;
                }
              for (size_t r = w_it+1; r < strlen(word); r++)
                if (word[r] == '>')
                {
                  if (word[r+1] == ' ')
                    for (size_t m = r; word[m] != '\0'; m++)
                      word[m+1] = word[m+2];
                  if (word[r-1] == ' ')
                    for (size_t m = r-1; word[m] != '\0'; m++)
                      word[m] = word[m+1];
                  for (r = 0; word[r] != '>'; r++);
                  int k = 0;
                  for (size_t j = r+1; j < strlen(word); j++, k++)
                  {
                    if (word[j] == ' ' || word[j] == '\n' ||
                        word[j] == '\t')
                      break;
                    else
                    {
                      output_stream[k] = word[j];
                      output_filled = true;
                    }
                  }
                  output_stream[k+1] = '\0';
                  break;
                }
              if (input_filled)
                new_cmd->input = input_stream;
              else
                new_cmd->input = NULL;
              if (output_filled)
                new_cmd->output = output_stream;
              else
                new_cmd->output = NULL;
              if (sequence_found)
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, SEQUENCE_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, SEQUENCE_COMMAND);
                sequence_found = false;
              }
              else if (pipe_found)
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, PIPE_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, PIPE_COMMAND);
                pipe_found = false;
              }
              else if (if_found)
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, IF_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, IF_COMMAND);
                if_found = false;
              }
              else if (while_found)
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, WHILE_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, WHILE_COMMAND);
                while_found = false;
              }
              else if (until_found)
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, UNTIL_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, UNTIL_COMMAND);
                until_found = false;
              }
              else
              {
                if (new_cmd->u.command[1])
                  new_cmd->u.command[2] = make_cmd(mini_char, SIMPLE_COMMAND);
                else
                  new_cmd->u.command[1] = make_cmd(mini_char, SIMPLE_COMMAND);
              }
              break;
            }
        mini_char[it] = word[w_it];
        // if-while-until check
        check = w_it;
        switch (word[w_it])
        {
          case 'i':
          {
            for (int j = 0; j < 3; j++, check++)
              mini_check[j] = word[check];
            if (if_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 3; j++, check++)
                  mini_check[j] = word[check];
                if (if_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'f')
                  if(fi_check(word, w_it))
                      counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 2; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 1;
                  w_it += 1;
                  if_found = true;
                  break;
                }
              }
            break;
          }
          case 'w':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (while_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (while_check(mini_check, 0))
                  counter++;
                if (until_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 3;
                  w_it += 3;
                  while_found = true;
                  break;
                }
              }
            break;
          }  
          case 'u':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (until_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (until_check(mini_check, 0))
                  counter++;
                if (while_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 3;
                  w_it += 3;
                  until_found = true;
                  break;
                }
              }
            break;
          }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; word[w_it] != '\n' && word[w_it] != ' ' &&
                word[w_it + 1] != '\0'; it++, w_it++, sc_marker++)
            mini_char[it] = word[w_it];
          if (sc_marker > 1)
            sequence_found = true;
        }
        // pipe behavior
        if (mini_char[it] == '|')
          pipe_found = true;
        // subshell behavior
        if (mini_char[it] == '(')
        {
          int p = 0;
          for (;; p++, it++)
          {
            if (word[w_it+p] == '(')
              sub_count++;
            if (word[w_it+p+1] == ')')
              sub_count--;
            if (sub_count == 0)
              break;
            subshell_cmd[p] = word[w_it+p+1];
          }
          for (int u = 0; u < 3; u++)
            if (new_cmd->u.command[u] == NULL)
            {
              new_cmd->u.command[u] = make_cmd(subshell_cmd, SUBSHELL_COMMAND);
              memset(subshell_cmd, '\0', strlen(subshell_cmd));
              memset(mini_char, '\0', strlen(mini_char));
              subshell_made = true;
              it = -1;
              w_it += p+1;
              break;
            }
        }
      }
      break;
    }
    case WHILE_COMMAND:
    case UNTIL_COMMAND:
    {
      bool pipe_found = false;
      bool sequence_found = false;
      bool if_found = false;
      bool while_found = false;
      bool until_found = false;
      bool subshell_made = false;
      bool input_filled = false;
      bool output_filled = false;
      char* input_stream = (char*) checked_malloc(strlen(word)+1);
      char* output_stream = (char*) checked_malloc(strlen(word)+1);      
      int sub_count = 0;
      for(int h = 0; h < 2; h++)
        new_cmd->u.command[h] = NULL;
      for (w_it += 5; w_it < strlen(word); w_it++, it++)
      {
        // do/done to delimit commands
        if (word[w_it] == 'd')
          if (word[w_it+1] == 'o')
          {
            if (word[w_it+2] == ' ' || word[w_it+2] == '\n')
            {
              if (subshell_made)
                break;
              if (sequence_found)
              {
                new_cmd->u.command[0] = make_cmd(mini_char, SEQUENCE_COMMAND);
                sequence_found = false;
              }
              else if (pipe_found)
              {
                new_cmd->u.command[0] = make_cmd(mini_char, PIPE_COMMAND);
                pipe_found = false;
              }
              else if (if_found)
              {
                new_cmd->u.command[0] = make_cmd(mini_char, IF_COMMAND);
                if_found = false;
              }
              else if (while_found)
              {
                new_cmd->u.command[0] = make_cmd(mini_char, WHILE_COMMAND);
                while_found = false;
              }
              else if (until_found)
              {
                new_cmd->u.command[0] = make_cmd(mini_char, UNTIL_COMMAND);
                until_found = false;
              }
              else
                new_cmd->u.command[0] = make_cmd(mini_char, SIMPLE_COMMAND);
              memset(mini_char, '\0', strlen(mini_char));
              it = -1;
              w_it += 2;
              continue;
            }
            if (word[w_it+2] == 'n')
              if (word[w_it+3] == 'e')
                if (word[w_it+4] == ' ' || word[w_it+4] == '\n' ||
                    word[w_it+4] == '\0')
                {
                  for (size_t r = w_it+1; r < strlen(word); r++)
                    if (word[r] == '<')
                    {
                      if (word[r+1] == ' ')
                        for (size_t m = r; word[m] != '\0'; m++)
                          word[m+1] = word[m+2];
                      if (word[r-1] == ' ')
                        for (size_t m = r-1; word[m] != '\0'; m++)
                          word[m] = word[m+1];
                      for (r = 0; word[r] != '<'; r++);
                      int k = 0;
                      for (size_t j = r+1; j < strlen(word); j++, k++)
                      {
                        if (word[j] == ' ' || word[j] == '\t' ||
                            word[j] == '>' || word[j] == '\n')
                          break;
                        else
                        {
                          input_stream[k] = word[j];
                          input_filled = true;
                        }
                      }
                      input_stream[k+1] = '\0';
                      break;
                    }
                  for (size_t r = w_it+1; r < strlen(word); r++)
                    if (word[r] == '>')
                    {
                      if (word[r+1] == ' ')
                        for (size_t m = r; word[m] != '\0'; m++)
                          word[m+1] = word[m+2];
                      if (word[r-1] == ' ')
                        for (size_t m = r-1; word[m] != '\0'; m++)
                          word[m] = word[m+1];
                      for (r = 0; word[r] != '>'; r++);
                      int k = 0;
                      for (size_t j = r+1; j < strlen(word); j++, k++)
                      {
                        if (word[j] == ' ' || word[j] == '\n' ||
                            word[j] == '\t')
                          break;
                        else
                        {
                          output_stream[k] = word[j];
                          output_filled = true;
                        }
                      }
                      output_stream[k+1] = '\0';
                      break;
                    }
                  if (input_filled)
                    new_cmd->input = input_stream;
                  else
                    new_cmd->input = NULL;
                  if (output_filled)
                    new_cmd->output = output_stream;
                  else
                    new_cmd->output = NULL;
                  if (subshell_made)
                    break;
                  if (sequence_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, SEQUENCE_COMMAND);
                    sequence_found = false;
                  }
                  else if (pipe_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, PIPE_COMMAND);
                    pipe_found = false;
                  }
                  else if (if_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, IF_COMMAND);
                    if_found = false;
                  }
                  else if (while_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, WHILE_COMMAND);
                    while_found = false;
                  }
                  else if (until_found)
                  {
                    new_cmd->u.command[1] = make_cmd(mini_char, UNTIL_COMMAND);
                    until_found = false;
                  }
                  else
                    new_cmd->u.command[1] = make_cmd(mini_char, SIMPLE_COMMAND);
                  break;
                }
          }
        mini_char[it] = word[w_it];
        // if-while-until check
        check = w_it;
        switch (word[w_it])
        {
          case 'i':
          {
            for (int j = 0; j < 3; j++, check++)
              mini_check[j] = word[check];
            if (if_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 3; j++, check++)
                  mini_check[j] = word[check];
                if (if_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'f')
                  if(fi_check(word, w_it))
                      counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 2; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 1;
                  w_it += 1;
                  if_found = true;
                  break;
                }
              }
            break;
          }
          case 'w':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (while_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (while_check(mini_check, 0))
                  counter++;
                if (until_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 3;
                  w_it += 3;
                  while_found = true;
                  break;
                }
              }
            break;
          }  
          case 'u':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (until_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (until_check(mini_check, 0))
                  counter++;
                if (while_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it+k] = word[w_it+k];
                  it += 3;
                  w_it += 3;
                  until_found = true;
                  for (size_t q = w_it; q < strlen(word); q++)
                  {
                    if (word[q] == 'd')
                      if (done_check(word, q))
                        break;
                    if (word[q] != '\n' || word[q] != '\t' || word[q] != ' ')
                      sequence_found = true;
                  }
                  break;
                }
              }
            break;
          }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; word[w_it] != '\n' && word[w_it] != ' ' &&
                word[w_it+1] != '\0'; it++, w_it++, sc_marker++)
            mini_char[it] = word[w_it];
          if (sc_marker > 1)
            sequence_found = true;
        }
        // pipe behavior
        if (mini_char[it] == '|')
          pipe_found = true;
        // subshell behavior
        if (mini_char[it] == '(')
        {
          int p = 0;
          for (;; p++, it++)
          {
            if (word[w_it+p] == '(')
              sub_count++;
            if (word[w_it+p+1] == ')')
              sub_count--;
            if (sub_count == 0)
              break;
            subshell_cmd[p] = word[w_it+p+1];
          }
          for (int u = 0; u < 3; u++)
            if (new_cmd->u.command[u] == NULL)
            {
              new_cmd->u.command[u] = make_cmd(subshell_cmd, SUBSHELL_COMMAND);
              memset(subshell_cmd, '\0', strlen(subshell_cmd));
              memset(mini_char, '\0', strlen(mini_char));
              subshell_made = true;
              it = -1;
              w_it += p+1;
              break;
            }
        }
      }
      break;
    }
    case PIPE_COMMAND:
    {
      bool sequence_found = false;
      int pipes = 0;
      int sub_count = 0;
      char* left_cmd;
      char* right_cmd = (char*) checked_malloc(strlen(mini_char) + 1);
      for(int h = 0; h < 2; h++)
        new_cmd->u.command[h] = NULL;
      for (; w_it < strlen(word); w_it++, it++)
      {
        mini_char[it] = word[w_it];
        if (mini_char[it] == '\n' || word[it + 1] == '\0')
        {
          for (int j = it; j > -1; j--)
            if (mini_char[j] == '|')
            {
              pipes--;
              int x = j;
              for(; !is_word(mini_char[x-1]); x--);
              left_cmd = (char*) checked_malloc((size_t) x);
              for(int k = 0; mini_char[j+1] != '\n' && mini_char[j+1] != '\0';
                  k++, j++)
                right_cmd[k] = mini_char[j+1];
              new_cmd->u.command[1] = make_cmd(right_cmd, SIMPLE_COMMAND);
              for(int z = 0; z < x; z++)
              {
                left_cmd[z] = mini_char[z];
                left_cmd[z+1] = '\0';
              }
              if (pipes > 0)
                new_cmd->u.command[0] = make_cmd(left_cmd, PIPE_COMMAND);
              else if (sequence_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, SEQUENCE_COMMAND); 
              else
                new_cmd->u.command[0] = make_cmd(left_cmd, SIMPLE_COMMAND);
              break;
            }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; word[w_it] != '\n' && word[w_it] != ' ' &&
                word[w_it+1] != '\0'; it++, w_it++, sc_marker++)
            mini_char[it] = word[w_it];
          if (sc_marker > 1)
            sequence_found = true;
          else
            for (int j = it; j > -1; j--)
              if (mini_char[j] == '|')
              {
                pipes--;
                int x = j;
                for(; !is_word(mini_char[x-1]); x--);
                left_cmd = (char*) checked_malloc((size_t) x);
                for(int k = 0; mini_char[j+1] != ';' && mini_char[j+1] != '\n' &&
                               mini_char[j+1] != '\0'; k++, j++)
                  right_cmd[k] = mini_char[j+1];
                new_cmd->u.command[1] = make_cmd(right_cmd, SIMPLE_COMMAND);
                for(int z = 0; z < x; z++)
                {
                  left_cmd[z] = mini_char[z];
                  left_cmd[z+1] = '\0';
                }
                if (pipes > 0)
                  new_cmd->u.command[0] = make_cmd(left_cmd, PIPE_COMMAND);
                else if (sequence_found)
                  new_cmd->u.command[0] = make_cmd(left_cmd, SEQUENCE_COMMAND);
                else
                  new_cmd->u.command[0] = make_cmd(left_cmd, SIMPLE_COMMAND);
                break;
              }
        }
        // pipe behavior
        if (mini_char[it] == '|')
          pipes++;
        // subshell behavior
        if (mini_char[it] == '(')
        {
          int p = 0;
          for (;; p++, it++)
          {
            if (mini_char[it] == '(')
              sub_count++;
            if (mini_char[it] == ')')
              sub_count--;
            if (sub_count == 0)
              break;
            subshell_cmd[p] = mini_char[it + 1];
          }
          for (int u = 0; u < 3; u++)
            if (new_cmd->u.command[u] == NULL)
            {
              new_cmd->u.command[u] = make_cmd(subshell_cmd, SUBSHELL_COMMAND);
              memset(subshell_cmd, '\0', strlen(subshell_cmd));
              memset(mini_char, '\0', strlen(mini_char));
              it = -1;
              w_it += p+1;
              break;
            }
        }
      }
      /*if(left_cmd)
      {
        free(left_cmd);
        left_cmd = NULL;
      }
      if(right_cmd)
      {
        free(right_cmd);
        right_cmd = NULL;
      }*/
      break;
    }
    case SEQUENCE_COMMAND:
    {
      bool if_found = false;
      bool while_found = false;
      bool until_found = false;      
      bool pipe_found = false;
      int seqs = 0;
      int sub_count = 0;
      size_t cmd_end;
      char* left_cmd;
      char* right_cmd = (char*) checked_malloc(strlen(mini_char) + 1);
      for(int h = 0; h < 2; h++)
        new_cmd->u.command[h] = NULL;
      for (size_t o = strlen(word); o != 0; o--)
        if (is_word(word[o]) || is_token(word[o]))
        {
          cmd_end = o;
          break;
        }
      for (; w_it < cmd_end + 1; w_it++, it++)
      {
        mini_char[it] = word[w_it];
        check = w_it;
        if (w_it == cmd_end)
        {
          for (int j = it; j > -1; j--)
            if (mini_char[j] == ';' || mini_char[j] == '\n')
            {
              seqs--;
              int x = j;
              for(; !is_word(mini_char[x-1]); x--);
              left_cmd = (char*) checked_malloc((size_t) x + 1);
              for(int k = 0; mini_char[j+1] != '\n' && mini_char[j+1] != '\0';
                  k++, j++)
                right_cmd[k] = mini_char[j+1];
              new_cmd->u.command[1] = make_cmd(right_cmd, SIMPLE_COMMAND);
              for(int z = 0; z < x; z++)
              {
                left_cmd[z] = mini_char[z];
                left_cmd[z+1] = '\0';
              }
              if (seqs > 0)
                new_cmd->u.command[0] = make_cmd(left_cmd, SEQUENCE_COMMAND);
              else if (pipe_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, PIPE_COMMAND);
              else if (if_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, IF_COMMAND);
              else if (while_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, WHILE_COMMAND);
              else if (until_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, UNTIL_COMMAND);
              else
                new_cmd->u.command[0] = make_cmd(left_cmd, SIMPLE_COMMAND);
              break;
            }
        }
        switch (word[w_it])
        {
          case 'i':
          {
            for (int j = 0; j < 3; j++, check++)
              mini_check[j] = word[check];
            if (if_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 3; j++, check++)
                  mini_check[j] = word[check];
                if (if_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'f')
                  if(fi_check(word, w_it))
                      counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 2; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 1;
                  w_it += 1;
                  if_found = true;
                  break;
                }
              }
            break;
          }
          case 'w':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (while_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (while_check(mini_check, 0))
                  counter++;
                if (until_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 3;
                  w_it += 3;
                  while_found = true;
                  break;
                }
              }
            break;
          }  
          case 'u':
          {
            for (int j = 0; j < 6; j++, check++)
              mini_check[j] = word[check];
            if (until_check(mini_check, 0))
              for(; w_it < strlen(word) && word[w_it] != '\0'; w_it++, it++)
              {
                mini_char[it] = word[w_it];
                check = w_it;
                for (int j = 0; j < 6; j++, check++)
                  mini_check[j] = word[check];
                if (until_check(mini_check, 0))
                  counter++;
                if (while_check(mini_check, 0))
                  counter++;
                if (word[w_it] == 'd')
                  if (done_check(word, w_it))
                    counter--;
                if (counter == 0)
                {
                  for (int k = 0; k < 4; k++)
                    mini_char[it + k] = word[w_it + k];
                  it += 3;
                  w_it += 3;
                  until_found = true;
                  break;
                }
              }
            break;
          }
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; word[w_it+1] != '\0' ||
               (word[w_it+1] != ';' && word[w_it+2] != '\0'); it++, w_it++)
          {
            mini_char[it] = word[w_it];
            if (word[w_it] == ';')
              sc_marker++;
          }
          seqs = sc_marker;
          for (int j = it; j > -1; j--)
            if (mini_char[j] == ';' || mini_char[j] == '\n')
            {
              seqs--;
              int x = j;
              for(; !is_word(mini_char[x-1]); x--);
              left_cmd = (char*) checked_malloc((size_t) x + 1);
              for(int k = 0; mini_char[j+1] != ';' && mini_char[j+1] != '\n' &&
                             mini_char[j+1] != '\0'; k++, j++)
                right_cmd[k] = mini_char[j+1];
              new_cmd->u.command[1] = make_cmd(right_cmd, SIMPLE_COMMAND);
              for(int z = 0; z < x + 1; z++)
              {
                left_cmd[z] = mini_char[z];
                left_cmd[z+1] = '\0';
              }
              if (seqs > 0)
                new_cmd->u.command[0] = make_cmd(left_cmd, SEQUENCE_COMMAND);
              else if (pipe_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, PIPE_COMMAND);
              else if (if_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, IF_COMMAND);
              else if (while_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, WHILE_COMMAND);
              else if (until_found)
                new_cmd->u.command[0] = make_cmd(left_cmd, UNTIL_COMMAND);
              else
                new_cmd->u.command[0] = make_cmd(left_cmd, SIMPLE_COMMAND);
              break;
            }
        }
        // pipe behavior
        if (mini_char[it] == '|')
          pipe_found = true;
        // subshell behavior
        if (mini_char[it] == '(')
        {
          int p = 0;
          for (;; p++, it++)
          {
            if (mini_char[it] == '(')
              sub_count++;
            if (mini_char[it] == ')')
              sub_count--;
            if (sub_count == 0)
              break;
            subshell_cmd[p] = mini_char[it + 1];
          }
          for (int u = 0; u < 3; u++)
            if (new_cmd->u.command[u] == NULL)
            {
              new_cmd->u.command[u] = make_cmd(subshell_cmd, SUBSHELL_COMMAND);
              memset(subshell_cmd, '\0', strlen(subshell_cmd));
              memset(mini_char, '\0', strlen(mini_char));
              it = -1;
              w_it += p+1;
              break;
            }
        }
      }
      /*if(left_cmd)
      {
        free(left_cmd);
        left_cmd = NULL;
      }
      if(right_cmd)
      {
        free(right_cmd);
        right_cmd = NULL;
      }*/
      break;
    }
    case SUBSHELL_COMMAND:
    {
      bool sequence_found = false;
      bool pipe_found = false;
      int sub_count = 0;
      for(int h = 0; h < 2; h++)
        new_cmd->u.command[h] = NULL;
      for (; w_it < strlen(word); w_it++, it++)
      {
        mini_char[it] = word[w_it];
        if (mini_char[it] == '\n' || word[it + 1] == '\0')
        {
          if (pipe_found)
            new_cmd->u.command[0] = make_cmd(mini_char, PIPE_COMMAND);
          else if (sequence_found)
            new_cmd->u.command[0] = make_cmd(mini_char, SEQUENCE_COMMAND);
          else
            new_cmd->u.command[0] = make_cmd(mini_char, SIMPLE_COMMAND);
          break;
        }
        // semi-colon behavior
        if (mini_char[it] == ';')
        {
          int sc_marker = 0;
          for(; word[w_it] != '\n' && word[w_it] != ' ' &&
                word[w_it + 1] != '\0'; it++, w_it++, sc_marker++)
            mini_char[it] = word[w_it];
          if (sc_marker > 1)
            sequence_found = true;
          else
            if (pipe_found)
              new_cmd->u.command[0] = make_cmd(mini_char, PIPE_COMMAND);
            else if (sequence_found)
              new_cmd->u.command[0] = make_cmd(mini_char, SEQUENCE_COMMAND);
            else
              new_cmd->u.command[0] = make_cmd(mini_char, SIMPLE_COMMAND);
            break;
        }
        // pipe behavior
        if (mini_char[it] == '|')
          pipe_found = true;
        // subshell behavior
        if (mini_char[it] == '(')
        {
          int p = 0;
          for (;; p++, it++)
          {
            if (mini_char[it] == '(')
              sub_count++;
            if (mini_char[it] == ')')
              sub_count--;
            if (sub_count == 0)
              break;
            subshell_cmd[p] = mini_char[it + 1];
          }
          for (int u = 0; u < 3; u++)
            if (new_cmd->u.command[u] == NULL)
            {
              new_cmd->u.command[u] = make_cmd(subshell_cmd, SUBSHELL_COMMAND);
              memset(subshell_cmd, '\0', strlen(subshell_cmd));
              memset(mini_char, '\0', strlen(mini_char));
              it = -1;
              w_it += p+1;
              break;
            }
        }
      }
      break;
    }
    default:
    {
      new_cmd = make_simple_cmd(word);
      break;
    }
  }
//  if(mini_char)
//  {
//    free(mini_char);
//    mini_char = NULL;
//  }
//  if(subshell_cmd)
//  {
//    free(subshell_cmd);
//    subshell_cmd = NULL;
//  }
  return new_cmd;
}

void insert_cmd(char *word, enum command_type cmd_type, command_stream_t cmd_stream)
{
  char* cmd_prep = (char*) checked_malloc(strlen(word) + 1);
  node_t new_node = (node_t) checked_malloc(sizeof(struct node));
  command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
  memcpy((void*) cmd_prep, (void*) word, strlen(word));
  new_cmd = make_cmd(cmd_prep, cmd_type);
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

// Initialization
command_stream_t
init_command_stream()
{
  command_stream_t stream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
  stream->m_head = NULL;
  stream->m_curr = NULL;
  return stream;
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
  char check_stream[6];
  size_t file_size = 0;
  int line = 1;
  int it = 0;
  int counter = 0;
  int check;
  int sc_marker = 0;
  int done_count = 0;
  int do_count = 0;
  int if_count = 0;
  int fi_count = 0;
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
  buffer = (char*) checked_malloc(file_size + 1);
  // Copy file to buffer
  for(size_t i = 0; (c = get_next_byte(get_next_byte_argument)) != EOF; i++)
  {
    file_size++;
    buffer = (char*) checked_realloc(buffer, file_size + 1);
    buffer[i] = c;
    buffer[i+1] = '\0';
  }
  // Syntax checking
  // Invalid characters
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
    line++;
    if (!valid_char(buffer[i]))
      print_err(line);
  }
  // Checking for do done
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'd')
      if(buffer[i+1] != '\0' && buffer[i+1] == 'o')
      {
        if(buffer[i+2] != '\0' && buffer[i+2] == 'n')
          if(buffer[i+3] != '\0' && buffer[i+3] == 'e')
          {
            int j = i + 4;
            while(buffer[j] == ' ')
            {
              if (buffer[j] == '\n')
                break;     
              j++;
            }
            if (buffer[j] != '\n' && buffer[j] != '>' &&
                buffer[j] != '\0')
              print_err(line);
            done_count++;
          }
        if(buffer[i+2] != '\0' && (buffer[i+2] == '\n' || buffer[i+2] == ' '))
          do_count++;
      }
    if(done_count > while_until_counter || do_count > while_until_counter)
      print_err(line);
    if(buffer[i] == 'w')
      if(while_check(buffer,i))
        while_until_counter++;
    if(buffer[i] == 'u')
      if(until_check(buffer,i))
        while_until_counter++;
  }
  if(while_until_counter != do_count || while_until_counter != done_count)
    print_err(line);
  // Checking for fi
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'f')
      if(buffer[i+1] != '\0' && buffer[i+1] == 'i')
        if(buffer[i+2] == '\0' || buffer[i+2] == '\n' || buffer[i+2] == ' ')
          fi_count++;
    if(fi_count > if_count)
      print_err(line);
    if(buffer[i] == 'i')
      if (buffer[i+1] != '\0' && buffer[i+1] == 'f')
        if(buffer[i+2] != '\0' && (buffer[i+2] == '\n' || buffer[i+2] == ' '))
          if_count++;
  }
  if(fi_count != if_count)
    print_err(line);
  // checking if if then else have commands after them before a semicolon or newline
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    found_command = false;
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'i')
      if(if_check(buffer, i))
      {
        i += 2;
        // checking if there is a command or if between an if and its semicolon or newline
        if (if_check(buffer, i))
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
                print_err(line);
              else
              {
                found_command = true;
                break;
              }
            }
            if (buffer[i] == ';' || buffer[i] == '\n')
              if(!found_command)
                print_err(line);
            i++;
          }
      }
    // checking if there is a command between else and its semicolon or newline
    if(buffer[i] == 'e')
      if(buffer[i+1] == 'l')
        if(buffer[i+2] == 's')
          if(buffer[i+3] == 'e')
          {
            else_count++;
            i += 4;
            if(else_count > then_count)
              print_err(line);
            while(buffer[i] != '\0')
            {
              if(buffer[i] == '\n')
                line++;
              if(is_word(buffer[i]))
              {
                if((buffer[i] == 'e' && buffer[i+1] == 'l' &&
                    buffer[i+2] == 's' && buffer[i+3] == 'e') ||
                   (buffer[i] == 't' && buffer[i+1] == 'h' &&
                    buffer[i+2] == 'e' && buffer[i+3] == 'n') ||
                   (buffer[i] == 'f' && buffer[i+1] == 'i' &&
                   (buffer[i+2] == '\n' || buffer[i+2] == '\0' ||
                    buffer[i+2] == '\t')))
                  print_err(line);
                else
                {
                  found_command = true;
                  break;
                }
              }
              if (buffer[i] == ';')
                if(!found_command)
                  print_err(line);
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
            {
              if(buffer[i] == '\n')
                line++;
              if(is_word(buffer[i]))
              { 
                if((buffer[i] == 'e' && buffer[i+1] == 'l' &&
                    buffer[i+2] == 's' && buffer[i+3] == 'e') ||
                   (buffer[i] == 't' && buffer[i+1] == 'h' &&
                    buffer[i+2] == 'e' && buffer[i+3] == 'n') ||
                   (buffer[i] == 'f' && buffer[i+1] == 'i' &&
                   (buffer[i+2] == '\n' || buffer[i+2] == '\0' ||
                    buffer[i+2] == '\t')))
                  print_err(line);
                else
                {
                  found_command = true;
                  break;
                }
              }
              if (buffer[i] == ';')
                if(!found_command)
                  print_err(line);
              i++;
            }
          }
  }
  if(then_count > if_count)
    print_err(line);
  if(else_count > if_count)
    print_err(line);
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    found_command = false;
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == 'w')
      if(while_check(buffer, i))
      {
        i += 5;
        // checking if there is command after while or another while
        if (while_check(buffer, i))
          while (buffer[i] != '\0')
          {
            if(buffer[i] == '\n')
              line++;
            if(is_word(buffer[i]))
            {
              if((buffer[i] == 'd' && buffer[i+1] == 'o') ||
                 (buffer[i] == 'd' && buffer[i+1] == 'o' &&
                  buffer[i+2] == 'n' && buffer[i+3] == 'e'))
                print_err(line);
              else
              {
                found_command = true;
                break;
              }
            }
            if (buffer[i] == ';')
              if(!found_command)
                  print_err(line);
            i++;
          }
        // checking if there is a command between do and its semicolon or newline
        if(buffer[i] == 'd')
          if(buffer[i+1] == 'o')
          {
            if(buffer[i+2] == ' ' || buffer[i+2] == '\n')
            {
              i+=2;
              while(buffer[i] != '\0')
              {
                if(buffer[i] == '\n')
                  line++;
                if(is_word(buffer[i]))
                {
                  if((buffer[i] == 'd' && buffer[i+1] == 'o' &&
                      buffer[i+2] == 'n' && buffer[i+3] == 'e'))
                    print_err(line);
                  else
                  {
                    found_command = true;
                    break;
                  }
                }
                if (buffer[i] == ';')
                  if(!found_command)
                    print_err(line);
                i++;
              }
            }
          }
      }
    // checking if there is a command between until and its semi colon and newlines
    if(buffer[i] == 'u')
      if(until_check(buffer , i))
      {
        i += 5;
        while(buffer[i] != '\0')
        {
          if(buffer[i] == '\n')
            line++;
          if(is_word(buffer[i]))
          {
            if((buffer[i] == 'd' && buffer[i+1] == 'o') ||
               (buffer[i] == 'd' && buffer[i+1] == 'o' &&
                buffer[i+2] == 'n' && buffer[i+3] == 'e'))
              print_err(line);
            else
            {
              found_command = true;
              break;
            }
          }
          if (buffer[i] == ';'|| buffer[i] == '\n')
            if(!found_command)
              print_err(line);
          i++;
        }
      }
  }
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == '<')
    {
      if(i == 0)
        print_err(line);
      if (buffer[i + 1] == '\0' || buffer[i + 1] == '\n')
        print_err(line);
      for(int k = i + 1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
      {
        if(buffer[k] != ' ')
        {
          if(buffer[k] == '<' || buffer[k] == ';' ||
             buffer[k] == '|' || buffer[k] == ')' ||
             buffer[k] == '>' || buffer[k] == '(' ||
             buffer[k] == '\t')
            print_err(line);
          else if(is_word(buffer[k]))
            break;
        }
      }
      for(int z = i-1; z > -1; z--)
      {
        if(buffer[z] != ' ')
        {
          if(buffer[z] == '<' || buffer[z] == ';' ||
             buffer[z] == '|' || buffer[z] == '(' ||
             buffer[z] == '>' || buffer[z] == '\n' ||
             buffer[z] == ')' || buffer[z] == '\t')
            print_err(line);
          else if(is_word(buffer[z]))
            break;
        }
      }
    }
    if(buffer[i] == '>')
    {
      if(i == 0)
        print_err(line);
      if (buffer[i + 1] == '\0' || buffer[i + 1] == '\n')
        print_err(line);
      for(int k = i + 1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
      {
        if(buffer[k] != ' ')
        {
          if(buffer[k] == '<' || buffer[k] == ';' ||
             buffer[k] == '|' || buffer[k] == ')' ||
             buffer[k] == '>' || buffer[k] == '(' ||
             buffer[k] == '\t')
            print_err(line);
          else if(is_word(buffer[k]))
            break;
        }
      }
      for(int z = i-1; z > -1; z--)
      {
        if(buffer[z] != ' ')
        {
          if(buffer[z] == '<' || buffer[z] == ';' ||
             buffer[z] == '|' || buffer[z] == '(' ||
             buffer[z] == '>' || buffer[z] == '\n' ||
             buffer[z] == ')' || buffer[z] == '\t')
            print_err(line);
          else if(is_word(buffer[z]))
            break;
        }
      }  
    }
  }
  //subshell syntax
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if(buffer[i] == ')')
    {
      right_paren_count++;
      if(right_paren_count > left_paren_count)
        print_err(line);
    }
    if (buffer[i] == '(')
      left_paren_count++;
    if (buffer[i] == ')' && buffer[i - 1] == '(')
      print_err(line);
    if (buffer[i] == ')')
      for(int z = i-1; buffer[z] != '('; z--)
      {
        if(is_word(buffer[z]))
          break;
        else if (buffer[z - 1] == '(')
          print_err(line);
      }
  }
  if(right_paren_count != left_paren_count)
    print_err(line);
  // pipe syntax
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if (buffer[i] == '\n')
      line++;
    if(buffer[i] == '|')
    {
      if(i == 0)
        print_err(line);
      //checking for invalidness before and after pipe
      for(int k = i + 1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
      {
        if(buffer[k] != ' ')
        {
          if(buffer[k] == '<' || buffer[k] == ';' ||
             buffer[k] == '|' || buffer[k] == ')' ||
             buffer[k] == '>' || buffer[k] == '\t')
            print_err(line);
          else break;
        }
      }
      for(int z = i-1; z > -1; z--)
      {
        if(buffer[z] != ' ')
        {
          if(buffer[z] == '<' || buffer[z] == ';' ||
             buffer[z] == '|' || buffer[z] == '(' ||
             buffer[z] == '>' || buffer[z] == '\t' ||
             buffer[z] == '\n')
            print_err(line);
          else break;
        }
      }
    }
  }
  //semicolon syntax
  line = 1;
  for(size_t i = 0; i < file_size; i++)
  {
    if(buffer[i] == '\n')
      line++;
    if (buffer[i] == ';')
    {
      if (i == 0)
        print_err(line);
      //checking for validity before and after semicolon
      for(int k = i+1; buffer[k] != '\0' && buffer[k] != '\n'; k++)
      {
        if(buffer[k] != ' ')
        {
          if(buffer[k] == '<' || buffer[k] == ';' ||
             buffer[k] == '|' || buffer[k] == ')' ||
             buffer[k] == '>' || buffer[k] == '\t')
            print_err(line);
          else break;
        }
      }
      for(int z = i-1; z > -1; z--)
      {
        if(buffer[z] != ' ')
        {
          if(buffer[z] == '<' || buffer[z] == ';' ||
             buffer[z] == '|' || buffer[z] == '(' ||
             buffer[z] == '>' || buffer[z] == '\t' ||
             buffer[z] == '\n')
            print_err(line);
          else break;
        }
      }
    }
  }
  char_stream = (char*) checked_malloc(strlen(buffer) + 1);
  for(size_t i = 0; i < strlen(buffer); i++, it++)
  {
    char_stream[it] = buffer[i];
    // if-while-until check
    if (char_stream[it] == 'i' || char_stream[it] == 'w' || char_stream[it] == 'u')
    {
      check = i;
      switch(char_stream[it])
      {
        case 'i':
        {
          if (if_check(buffer, i))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++, it++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 3; j++, check++)
                check_stream[j] = buffer[check];
              if (if_check(check_stream, 0))
                counter++;
              if (buffer[i] == 'f')
                if(fi_check(buffer, i))
                  counter--;
              if (counter == 0)
              {
                for (int k = 1; k < 4; k++)
                  char_stream[it + k] = buffer[i + k];
                i += 3;
                insert_cmd(char_stream, IF_COMMAND, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0', strlen(check_stream));
                it = -1;
                break;
              }
            }
          }
          break;
        }
        case 'w':
        {
          for (int j = 0; j < 6; j++, check++)
            check_stream[j] = buffer[check];
          if (while_check(check_stream, 0))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 5; j++, check++)
                check_stream[j] = buffer[check];
              if (while_check(check_stream, 0))
                counter++;
              if (until_check(check_stream, 0))
                counter++;
              if (buffer[i] == 'd')
                if (done_check(buffer, i))
                  counter--;
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                i += 6;
                insert_cmd(char_stream, WHILE_COMMAND, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0', strlen(check_stream));
                it = -1;
                break;
              }
              it++;
            }
          }
          break;
        }
        case 'u':
        {
          for (int j = 0; j < 6; j++, check++)
            check_stream[j] = buffer[check];
          if (until_check(check_stream, 0))
          {
            for(; i < strlen(buffer) && buffer[i] != '\0'; i++)
            {
              char_stream[it] = buffer[i];
              check = i;
              for (int j = 0; j < 5; j++, check++)
                check_stream[j] = buffer[check];
              if (until_check(check_stream, 0))
                counter++;
              if (while_check(check_stream, 0))
                counter++;
              if (buffer[i] == 'd')
                if (done_check(buffer, i))
                  counter--;
              if (counter == 0)
              {
                for (int k = 1; k < 6; k++)
                  char_stream[it + k] = buffer[i + k];
                i += 6;
                insert_cmd(char_stream, UNTIL_COMMAND, cmd_stream);
                memset(char_stream, '\0', strlen(char_stream));
                memset(check_stream, '\0', strlen(check_stream));
                it = -1;
                break;
              }
              it++;
            }
          }
          break;
        }
      }
    }
    // comment behavior
    if (buffer[i] == '#')
      if (it == 0 || !is_word(char_stream[it - 1]))
        for(; buffer[i] != '\n'; i++);
    // newline behavior
    if (char_stream[it] == '\n')
      if (it > 0 && char_stream[it - 1] == '\n')
      {
        bool word_present = false;
        for (size_t n = 0; n < strlen(char_stream); n++)
        {
          if (char_stream[n] != '\n' && char_stream[n] != ' ' && 
              char_stream[n] != '\t')
            word_present = true;
        }
        if (word_present)
        {
          char_stream[it-1] = '\0'; 
          insert_cmd(char_stream, SIMPLE_COMMAND, cmd_stream);
          memset(char_stream, '\0', strlen(char_stream));
          it = -1;
        }
        else continue;
      }
    // semi-colon behavior
    if (char_stream[it] == ';')
    {
      for(; char_stream[it] != '\n' && char_stream[it] != ' ' &&
            char_stream[it] != '\0'; it++, i++, sc_marker++)
        char_stream[it] = buffer[i];
      if (sc_marker > 1)
      {
        insert_cmd(char_stream, SEQUENCE_COMMAND, cmd_stream);
        memset(char_stream, '\0', strlen(char_stream));
        it = -1;
        break;
      }
      else
      {
        insert_cmd(char_stream, SIMPLE_COMMAND, cmd_stream);
        memset(char_stream, '\0', strlen(char_stream));
        it = -1;
      }
    }
    // pipe behavior
    if (char_stream[it] == '|')
    {
      for(; (char_stream[it] != '\n' && char_stream[it] != ';') &&
             buffer[i+1] != '\0'; i++, it++)
        char_stream[it] = buffer[i];
      insert_cmd(char_stream, PIPE_COMMAND, cmd_stream);
      memset(char_stream, '\0', strlen(char_stream));
      it = -1;
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
      it = -1;
    }
    if (buffer[i+1] == '\0')
    {
      if (char_stream[0] == '\0')
        break;
      insert_cmd(char_stream, SIMPLE_COMMAND, cmd_stream);
      memset(char_stream, '\0', strlen(char_stream));
      it = -1;
    }
  }
//  if(char_stream)
//  {
//    free(char_stream);
//    char_stream = NULL;
//  }
//  if(buffer)
//  {
//    free(buffer);
//    buffer = NULL;
//  }
  return cmd_stream;
}

command_t
read_command_stream (command_stream_t s)
{
  if (cmd_read == cmd_total)
    return NULL;
  if (s->m_head)
  {
//    node_t cmd_node = (node_t) checked_malloc(sizeof(struct node));
//    cmd_node = s->m_head;
    command_t cmd = (command_t) checked_malloc(sizeof(struct command));
    cmd = s->m_head->m_command;
    s->m_head = s->m_head->m_next;
//    if (cmd_node->m_command)
//    {
//      free_cmd(cmd_node->m_command);
//      cmd_node->m_command = NULL;
//    }
//    if(cmd_node)
//    {
//      free(cmd_node);
//      cmd_node = NULL;
//    }
    cmd_read++;
    return cmd;
  }
  fprintf(stderr, "Could not read command");
  exit(-1);
}