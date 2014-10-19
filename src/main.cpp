#include <iostream>
#include <cstring>
#include <unistd.h>
#include "MyMonitor.h"
#include "MyLog.h"

int main(int argc, char ** argv)
{
  if (argc != 2)
  {
    std::cout << "Usage: ./my_daemon filename.cfg\n";
    return 1;
  }

  MyMonitor monitor;
  int status = monitor.LoadConfig(argv[1]);

  if (!status)
  {
    std::cout << "Error: Load config failed\n";
    return -1;
  }

  int pid = fork();
  if (pid == -1)
  {
    std::cout << "Error: Start Daemon failed (" << strerror(errno) << ").\n";
    return -1;
  }
  else if (!pid) // it's child
  {
    status = monitor.Run();
    
    WriteLog("[MAIN] Exit from main...");    
    return status;
  }
  
  return 0; // it's parent so just leave
}
