#pragma once

#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define MAX_PATH_LENGTH 1000
#define CHILD_NEED_TERMINATE 1
#define CHILD_NEED_WORK 2

struct ConfigInfo
{
  ConfigInfo() : seconds_interval(0), minutes_interval(0), hours_interval(0), days_interval(0) {}
  size_t seconds_interval;
  size_t minutes_interval;
  size_t hours_interval;
  size_t days_interval;

  std::string directory;
};



class MyDaemon

{

public:

  MyDaemon(size_t _secondsToConsumeFileOld = 60) : m_secondsToConsumeFileOld(_secondsToConsumeFileOld), m_configLoaded(false), m_thread(0) {}

  int LoadConfig(const char *fileName);

  int InitDaemon();

  void DeinitDaemon();

  int Run();
  std::string GetDirectory() const
  { 
    sem_wait(&m_semaphore);
    std::string ret = m_conf.directory;
    sem_post(&m_semaphore);
    return ret;    
  }
  size_t GetInterval() const
  {
    sem_wait(&m_semaphore);
    size_t ret = m_conf.seconds_interval + 60 * (m_conf.minutes_interval + 60 * (m_conf.hours_interval + 24 * m_conf.days_interval));
    sem_post(&m_semaphore);
    return ret;
  }
  size_t GetMinTimeToConsumeFileOld() const
  {
    sem_wait(&m_semaphore);
    size_t ret = m_secondsToConsumeFileOld;
    sem_post(&m_semaphore);
    return ret;
  }
private:
  int m_ReloadConfig();
  int InitWorkThread();
  void DestroyWorkThread();

  pthread_t m_thread;
  ConfigInfo m_conf;
  size_t m_secondsToConsumeFileOld;
  std::string m_configFileName;
  std::string m_configFilePath;
  mutable sem_t m_semaphore;
  bool m_configLoaded;

};

 void signal_error(int sig, siginfo_t *si, void *ptr);
  void *DaemonFunction(void *ptr);
