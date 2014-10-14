#include "MyDaemon.h"
#include "MyLog.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include <signal.h>

int MyDaemon::LoadConfig(const char *fileName)
{
  m_configFileName = fileName;
  std::ifstream in(fileName);
  std::string s;
  in >> s;
  if (s != "directory:")
  {
    WriteLog("[DAEMON] Wrong .cfg format: 'directory: ' tag not found.\n");
    return 0;
  }
  in >> m_conf.directory;
  WriteLog("[DAEMON] read directory to clear from config file: dir = %s.\n", m_conf.directory.c_str());

  while (in >> s)
  {
    if (s == "seconds_interval:")
    {
      in >> m_conf.seconds_interval;
      WriteLog("[DAEMON] read seconds interval from config file: sec_int = %i.\n", m_conf.seconds_interval);
    }
    else if (s == "minutes_interval:")
    {
      in >> m_conf.minutes_interval;
      WriteLog("[DAEMON] read minutes interval from config file: min_int = %i.\n", m_conf.minutes_interval);
    }
    else if (s == "hours_interval:")
    {
      in >> m_conf.hours_interval;
      WriteLog("[DAEMON] read hours interval from config file: hour_int = %i.\n", m_conf.hours_interval);
    }
    else if (s == "days_interval:") {
      in >> m_conf.days_interval;
      WriteLog("[DAEMON] read days interval from config file: day_int = %i.\n", m_conf.days_interval);
    }
    else {
      WriteLog("[DAEMON] read unknown parameter from config file: %s\n", s.c_str());
    }
  }
  
  m_configLoaded = true;
  return 1;
}

void *DaemonFunction(void *ptr)
{
  const MyDaemon *daemon = (const MyDaemon*)ptr;
  int interval = daemon->GetInterval();
  size_t minDt = daemon->GetMinTimeToConsumeFileOld();
  while (true)
  {
    std::string path = daemon->GetDirectory();
    
    DIR *d = opendir(path.c_str());
    if (d == nullptr)
    {
      WriteLog("[DAEMON] Couldn't find directory '%s'.\n", path.c_str());
      return 0;
    }

    
    struct dirent *de = NULL;
    while(de = readdir(d))
    {
      std::string fileName = path;
      fileName.append(de->d_name);
      
      struct stat attrs;
      int stat_result = stat(fileName.c_str(), &attrs);
      bool isdir = S_ISDIR(attrs.st_mode);
      if (isdir)
        continue;
      time_t birth_time = attrs.st_mtime;
      time_t cur_time = 0;
      time(&cur_time);

      if (cur_time - birth_time > minDt)
      {   
        WriteLog("[DAEMON] Deleting file %s.\n", fileName.c_str());
        remove(fileName.c_str());
      }
    }
    closedir(d);
    WriteLog("Files from %s older than 1 minute, were deleted.\n", path.c_str());
    sleep(interval);
  }
}

int MyDaemon::m_ReloadConfig()
{
  if (!m_configLoaded)
  {
    // ...
    return 0;
  }
  
  sem_wait(&m_semaphore);
  chdir(m_configFilePath.c_str());
  int result = LoadConfig(m_configFileName.c_str());
  sem_post(&m_semaphore);
  
  chdir("/");
  if (result)
    WriteLog("[DAEMON] Config file was reloaded succesfully.\n");
  else
    WriteLog("[DAEMON] Error reloading config file.\n");
  return result;
}

void m_signal_error(int sig, siginfo_t *si, void *ptr)
{
    WriteLog("[DAEMON] Signal: %s, Addr: 0x%0.16X\n", strsignal(sig), si->si_addr);

    exit(CHILD_NEED_WORK);
}

int MyDaemon::Run()
{
  struct sigaction sigact;
  sigset_t sigset;
  int signo;
  int status;
  
  sigact.sa_flags = SA_SIGINFO; // signal handler wiil take 3 arguments, not 1
  sigact.sa_sigaction = m_signal_error;

  sigemptyset(&sigact.sa_mask);
  // last param is old sigaction (if not null, store it there)
  sigaction(SIGFPE, &sigact, 0); // FPU error
  sigaction(SIGILL, &sigact, 0); // wrong instruction
  sigaction(SIGSEGV, &sigact, 0); 
  sigaction(SIGBUS, &sigact, 0); 
 
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGHUP);
  sigprocmask(SIG_BLOCK, &sigset, NULL); // block signals which we will work with on our own

  WriteLog("[DAEMON] Started\n");
  
  status = InitWorkThread();
  if (!status)
  {
    // waiting for signals
    for (;;)
    {
      sigwait(&sigset, &signo);
      if (signo == SIGHUP) // this message tells us to reload config
      {
        status = m_ReloadConfig();
      }
      else if (signo == SIGTERM) // terminate daemon
      {
        WriteLog("[DAEMON] SIGTERM caught! Exiting...\n");
        break;
      }
      else 
      {
        WriteLog("[DAEMON] ");
        break;
      }
    }

    DestroyWorkThread();
  }
  else
  {
    WriteLog("[DAEMON] Create work thread failed\n");
  }

  WriteLog("[DAEMON] Stopped\n");

  // return code indicationg we don't need re-running
  return CHILD_NEED_TERMINATE;
}

int MyDaemon::InitWorkThread()
{
  int iret1 = pthread_create(&m_thread, NULL, DaemonFunction, this);
  if (iret1 == 0)
  {
    WriteLog("[DAEMON] Deleter thread was created.\n");
  }
  else 
  {
    WriteLog("[DAEMON] Deleter thread creation failure.\n");
  }
  return 0;
}

void MyDaemon::DestroyWorkThread()
{
  void *val;
  int res = pthread_cancel(m_thread);
  pthread_join(m_thread, &val);
  if (val == PTHREAD_CANCELED)
    WriteLog("[DAEMON] Deleter thread was cancelled.\n");
  else
    WriteLog("[DAEMON] Error cancelling deleter thread.\n");
}

int MyDaemon::InitDaemon()
{
  sem_init(&m_semaphore, 0, 1);
  umask(0);
  setsid();

  char path[MAX_PATH_LENGTH + 1];
  getcwd(path, MAX_PATH_LENGTH);
  m_configFilePath = path;
  chdir("/");
  
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  
  return 0;
}

void MyDaemon::DeinitDaemon()
{
  sem_destroy(&m_semaphore); /* destroy semaphore */
}
