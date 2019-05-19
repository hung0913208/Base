#include <Unittest.h>
#include <Macro.h>

#ifndef WINDOW

TEST(Popen, Simple){
#if DEV
  Base::Config::Disable(EWatchStopper);
#endif
}
#endif

int main(){
  return RUN_ALL_TESTS();
}

#if SAMPLE
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  char *args[]={"/bin/tr", "a-z", "A-Z", NULL};
  pid_t pid;
  int run=1;

  int wrpipe[2], rdpipe[2];
  pipe(wrpipe);	pipe(rdpipe);

  pid=fork();
  if(pid < 0)
    return(1);
  else if(pid == 0) // child
  {
    dup2(wrpipe[0], STDIN_FILENO);
    dup2(rdpipe[1], STDOUT_FILENO);

    close(wrpipe[0]);	close(wrpipe[1]);
    close(rdpipe[0]);	close(rdpipe[1]);

    execvp(args[0], args);
    fprintf(stderr, "exec failed\n");
    exit(1);
  }
  else
  {
    struct pollfd p[3];
    char buf_in[512];
    size_t pos_in=0;

    // watch stdin
    p[0].fd=STDIN_FILENO;
    p[0].events=POLLIN|POLLHUP;
    p[0].revents=0;

    // watch output pipe
    p[1].fd=wrpipe[1];
    p[1].events=POLLOUT|POLLHUP;
    p[1].revents=0;

    // watch input pipe
    p[2].fd=rdpipe[0];
    p[2].events=POLLIN|POLLPRI|POLLHUP;
    p[2].revents=0;

    close(wrpipe[0]);	close(rdpipe[1]);

    while(run)
    {
      int err=poll(p, 3, 0);

      if(err < 1)
        continue;

      // Events on stdin
      if(p[0].revents & (POLLIN|POLLHUP))
      {
        size_t max=512-pos_in;
        size_t len=read(p[0].fd, buf_in+pos_in, max);

        fprintf(stderr, "stdin read %d bytes\n", len);

        if(len >= 0) pos_in += len;

        // EOF.
        if(len == 0)
        {
          fprintf(stderr, "stdin EOF\n");
          // Write the last data
          write(wrpipe[1], buf_in, pos_in);
          close(wrpipe[1]);
          run=0;	// break the loop
          continue;
        }
      }

      if((p[1].revents & (POLLOUT | POLLHUP)) && pos_in )
      {
        size_t left;
        size_t len=write(p[1].fd, buf_in, pos_in);
        left=pos_in-len;

        fprintf(stderr, "pipe out wrote %d bytes\n", len);

        // Move anything unwritten to the start
        if(left > 0)
          memmove(buf_in, buf_in+len, left);
        pos_in=left;
      }

      if(p[2].revents & (POLLIN | POLLHUP))
      {
        char buf[512];
        size_t len=read(p[2].fd, buf, 512);

        fprintf(stderr, "pipe in read %d bytes\n", len);

        if(len == 0) // EOF
        {
          fprintf(stderr, "pipe in EOF\n");
          // Write the last data
          write(wrpipe[1], buf_in, pos_in);
          close(wrpipe[1]);
          run=0;	// break the loop
          continue;
        }

        write(STDOUT_FILENO, buf, len);
      }
    }

    // Keep reading until eof
    run=1;
    while(run)
    {
      char buf[512];
      size_t len=read(p[2].fd, buf, 512);

      if(len <= 0)
        break;

      fprintf(stderr, "read %d bytes\n", len);

      write(STDOUT_FILENO, buf, len);
    }
  }
}
#endif
