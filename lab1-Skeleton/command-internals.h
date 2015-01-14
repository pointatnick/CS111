enum command_type
  {
    IF_COMMAND,		 // if A then B else C fi
    PIPE_COMMAND,        // A | B
    SEQUENCE_COMMAND,    // A ; B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
    UNTIL_COMMAND,	 // until A do B done
    WHILE_COMMAND,	 // while A do B done
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // For SIMPLE_COMMAND.
    char **word;

    // For all other kinds of commands.  Trailing entries are unused.
    // Only IF_COMMAND uses all three entries.
    struct command *command[3];
  } u;
};
