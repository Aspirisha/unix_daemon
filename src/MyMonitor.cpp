#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h> 
#include <sys/wait.h>
#include "MyMonitor.h"
#include "MyLog.h"

int MyMonitor::Run()
{
  int pid;
  int status;
  int need_start = 1;
  sigset_t sigset;
  siginfo_t siginfo;

  sigemptyset(&sigset);

  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGCHLD);
  sigaddset(&sigset, SIGHUP);

  sigprocmask(SIG_BLOCK, &sigset, NULL);

  bool run = true;
  m_daemon.InitDaemon();
  while (run)
  {
    if (need_start)
      pid = fork();
    need_start = 1;

    if (pid == -1) // error occured
    {
      WriteLog("[MONITOR] Fork failed (%s)\n", strerror(errno));
    }
    else if (!pid) // we are in child body
    {
      status = m_daemon.Run(); // function that contains daemon work
      exit(status);
    }
    else // if we are parent
    {
      sigwaitinfo(&sigset, &siginfo);
      switch(siginfo.si_signo)
      {
      case SIGCHLD:
        wait(&status);
        status = WEXITSTATUS(status);
        if (status == CHILD_NEED_TERMINATE)
        {
          WriteLog("[MONITOR] Child stopped\n");
          run = false;
        }
        else if (status == CHILD_NEED_WORK)
        {
          //m.DestroyWorkThread();
          WriteLog("[MONITOR] Child restart\n");
        }
        break;
      case SIGHUP: // reload config signal
        kill(pid, SIGHUP);
        need_start = 0;
        break;
      default:
        WriteLog("[MONITOR] Signal %s\n", strsignal(siginfo.si_signo));
        kill(pid, SIGTERM);
        status = 0;
        run = false;      
      }
    }
  }

  WriteLog("[MONITOR] Stop\n");
  m_daemon.DeinitDaemon();
  return status;
}
