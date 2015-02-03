#include "command.h"
#include "command-internals.h"
#include <stdio.h>
#include <error.h>
#include <errno.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling(char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (1, 0, "command execution not yet implemented");
}

int
command_status(command_t c)
{
  return c->status;
}

void
exec_simple_cmd(command_t cmd)
{
	int state;
	pid_t pid = fork();
	if (pid < 0)
		//error
	else if (pid == 0)
	{
		//redirects
		if (strcmp(c->u.word[0], "exec") == 0)
			c->u.word++;
		execvp(c->u.word[0], c->u.word);
		//error
	}
	else
	{
		waitpid(pid, &state, 0);
		c->status = WEXITSTATUS(state);
	}
}

void
execute_command(command_t c, int profiling)
{
  switch(c->status)
  {
  	case IF_COMMAND:
  		execute_command(c->u.command[0]);
  		execute_command(c->u.command[1]);
  		if (c->u.command[2])
  		{
  			execute_command(c->u.command[2]);
  			c->status = command_status(c->u.command[2]);
  		}
  		else
  			c->status = command_status(c->u.command[1]);
  		break;
  	case WHILE_COMMAND:
  	case UNTIL_COMMAND:
	  	execute_command(c->u.command[0]);
  		execute_command(c->u.command[1]);
  		break;
  	case PIPE_COMMAND:
  		pid_t lpid;
  		pid_t rpid;
  		int fd[2];
  		int state;
  		lpid = fork();
  		if (lpid < 0)
  			//error
  		else if (lpid == 0)
  		{
  			if (dup2(fd[0], 0) < 0)
  				//error
  			close(fd[1]);
  			execute_command(c->u.command[1]);
  			_exit(c->u.command[1]->status);
  		}
  		else
  		{
  			rpid = fork();
  			if (rpid < 0)
  				//error
  			else if (rpid == 0)
  			{
  				if (dup2(fd[1], 1) < 0)
  					//error
  				close(fd[0]);
  				execute_command(c->u.command[0]);
  				_exit(c->u.command[0]->status);
  			}
  			else
  			{
  				//returnedPid = waitpid(-1, &state, 0);
  				close(fd[0]);
  				close(fd[1]);
  				if (rpid == returnedPid)
  				{
  					waitpid(lpid, &state, 0);
  					c->status = WEXITSTATUS(state);
  					return;
  				}
  				if (lpid == returnedPid)
  				{
  					waitpid(rpid, state, 0);
  					c->status = WEXITSTATUS(state);
  					return;
  				}
  			}
  		}
  		break;
  	case SEQUENCE_COMMAND:
  		execute_command(c->u.command[0]);
  		execute_command(c->u.command[1]);
  		c->status = command_status(c->u.command[1]);
  		break;
  	case SUBSHELL_COMMAND:
  		int state;
  		pid_t pid = fork();
  		if (pid < 0)
  			//error
  		else if (pid == 0)
  		{
  			//redirect
  			//execute the subshell command
  		}
  		else
  		{
  			waitpid(pid, &state, 0);
  			c->status = WEXITSTATUS(state);
  		}
  		break;
  	case SIMPLE_COMMAND:
  		exec_simple_cmd(c);
  		break;
  	default: break;
  }
}