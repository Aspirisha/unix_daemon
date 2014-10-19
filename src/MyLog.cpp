#include "MyLog.h"
#include <cstdarg>
#include <stdio.h>
#include <syslog.h>

void WriteLog(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int len = vsnprintf(0, 0, format, args) + 1;
  char *buffer = new char[len];
  vsnprintf(buffer, len, format, args);

  openlog("my_daemon", 0, 0);
  syslog(LOG_INFO, "%s", buffer);
  closelog();

  va_end(args);
  delete[] buffer;
}
