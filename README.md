# Mini-Shell
Mini Shell in Linux

## Description
Mini Shell supports the execution of external commands with multiple arguments,
internal commands (`exit`/`quit`, `cd`), environment variables, redirection and
pipes.

## Operators (in descending order)
  * **|** pipe: `cmd1 | cmd2` will execute `cmd1` with its output redirected to
  the input on `cmd2`
  * **&&** and **||** conditional execution: `cmd1 | cmd2` will execute `cmd2`
  only if `cmd1` *did* / *did not* return 0
  * **&** parallelism: `cmd1 & cmd2` will execute `cmd1` and `cmd2` at the same
  time
  * **;** sequencing: `cmd1 ; cmd2` will execute `cmd2` after `cmd1` finished
  its execution 
  
## Implementation
Running a simple command first checks if the it's an internal command; if so,
run the internal command, otherwise `fork` and let the child execute the
command.

Redirection makes use of [dup](http://linux.die.net/man/2/dup) and
[dup2](http://linux.die.net/man/2/dup2) system calls. Check `redirect_all` function
in [utils-lin.c](https://github.com/Matei94/Mini-Shell/blob/master/utils-lin.c)
to see how redirection is handled.

Environment variables are handled using the
[setenv](http://linux.die.net/man/3/setenv) and
[getenv](http://linux.die.net/man/3/getenv) system calls.

Parallel commands and pipes are implemented by doing 2 forks; the difference
between them is that for pipes an anonymous pipe is created for inter-process
communication.
