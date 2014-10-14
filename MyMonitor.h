#pragma once

#include "MyDaemon.h"

class MyMonitor
{
public:
  MyMonitor(size_t _secondsToConsumeFileOld = 60) : m_daemon(_secondsToConsumeFileOld) {}
  int LoadConfig(const char *fileName) { m_daemon.LoadConfig(fileName); };
  int Run();
private:
  MyDaemon m_daemon;
};
