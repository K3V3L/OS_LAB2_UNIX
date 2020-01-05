#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <vector>
const long minute = 60;
const long hour = minute * 60;
const long day = hour * 24;
const double MB = 1024 * 1024;
int uptime, cpuhertz = sysconf(_SC_CLK_TCK);

enum sorttype { pid, cpu, name };

std::string parseState(char c) {
  switch (c) {
  case 'R':
    return "Running";
  case 'S':
    return "Sleeping";
  case 'D':
    return "Waiting for disk";
  case 'Z':
    return "Zombie";
  case 'T':
    return "Stopped";
  case 't':
    return "Tracing Stop";
  case 'X':
    return "Dead";
  case 'x':
    return "Dead";
  case 'K':
    return "Wakekill";
  case 'W':
    return "Waking";
  default:
    return "?";
  }
}
class memory {
public:
  memory() { refresh(); }
  void refresh() {
    struct sysinfo sinfo;
    sysinfo(&sinfo);
    memTotal = sinfo.totalram;
    memFree = sinfo.freeram;
    swapTotal = sinfo.totalswap;
    swapFree = sinfo.freeswap;
  }
  void printInfo() {
    printf("Memory Info:\n\tRAM:\t%.0lf MB / %.0lf MB\tFree: %.0lf MB\n",
           memTotal / MB - memFree / MB, memTotal / MB, memFree / MB);
  }

private:
  unsigned long memTotal, memFree, swapTotal, swapFree, SharedRam;
};
class process {
public:
  std::string command, state, user;
  int procID, mem, parID, cpu, userId, size, rsize;
  unsigned long utime, stime;
  unsigned long long startTime;
  long double totalTIme;
  long cutime, cstime, secondsSinceStart;
  void calculateCPU() {
    totalTIme = utime + stime;
    secondsSinceStart =
        (long double)uptime - ((long double)startTime / cpuhertz);
    cpu = std::abs(25 * ((totalTIme / cpuhertz) / (secondsSinceStart + 1)));
  }
};
process getProcInfo(char pID[256]) {

  unsigned long long var;
  char path[256];
  process proc;

  char cur_state, command[256];
  sprintf(path, "/proc/%s/stat", pID);
  FILE *file = fopen(path, "r");
  fscanf(file, "%d%s", &proc.procID, &command);
  proc.command = std::string(command);
  fscanf(file, "%c%c", &cur_state, &cur_state);
  proc.state = parseState(cur_state);

  fscanf(file, "%d", &proc.parID);
  fscanf(file, "%d%d%d%d%u%lu%lu%lu%lu", &var, &var, &var, &var, &var, &var,
         &var, &var, &var);
  fscanf(file, "%lu%lu%ld%ld", &proc.utime, &proc.stime, &proc.cutime,
         &proc.cstime);
  fscanf(file, "%ld%ld%ld%ld", &var, &var, &var, &var);
  fscanf(file, "%llu", &proc.startTime);
  fclose(file);

  sprintf(path, "/proc/%s/statm", pID);
  file = fopen(path, "r");
  fscanf(file, "%d", &proc.size);
  fscanf(file, "%d", &proc.rsize);
  fclose(file);

  proc.calculateCPU();
  sprintf(path, "/proc/%s/status", pID);
  file = fopen(path, "r");
  char buffer[100];
  while (fgetc(file) != EOF) {
    fscanf(file, "%s", &buffer);
    if (std::string(buffer) == "Uid:") {
      fscanf(file, "%s", &buffer);
      proc.user = std::string(buffer);
      break;
    }
  }
  fclose(file);

  return proc;
}

bool sortByName(const process &a, const process &b)
  {return a.command < b.command;}
bool sortByPID(const process &a, const process &b)
{return a.procID < b.procID;}
bool sortByCPU(const process &a, const process &b)
{return a.cpu < b.cpu;}


class processTable {
public:
  processTable() { refresh(); }

  void refresh() {
    processes.clear();
    FILE *file = fopen("/proc/uptime", "r");
    fscanf(file, "%d", &uptime);
    fclose(file);
    DIR *dir;
    struct dirent *de;
    dir = opendir("/proc");
    while ((de = readdir(dir)) != NULL) {
      if (!isdigit(de->d_name[0]))
        continue;
      processes.push_back(getProcInfo(de->d_name));
    }
  }
  void print(sorttype sort) {
    switch (sort)
    {
    case name:
      std::sort(processes.begin(), processes.end(), sortByName);
      break;
    case pid:
      std::sort(processes.begin(), processes.end(), sortByPID);
      break;
    case cpu:
      std::sort(processes.begin(), processes.end(), sortByCPU);
      break;
    default:
      break;
    }



    for (auto process : processes) {
      std::cout << std::setw(5) << process.procID << std::setw(10)
                << process.user << std::setw(40) << process.command
                << std::setw(16) << process.state << std::setw(10)
                << process.parID << std::setw(10) << process.cpu
                << std::setw(10) << std::abs(process.size) << std::setw(10)
                << std::abs(process.rsize) << std::endl;
    }
  }
  void printHeader() {
    std::cout << std::setw(5) << "PID" << std::setw(10) << "UserID"
              << std::setw(40) << "Name" << std::setw(16) << "State"
              << std::setw(10) << "Parent ID" << std::setw(10) << "CPU %"
              << std::setw(10) << "VSZ" << std::setw(10) << "RSS" << std::endl
              << std::endl;
  }

private:
  void sort() {}
  std::vector<process> processes;
};
int main(int argc, const char *argv[]) {
  sorttype sort;
  if (argc == 2) {
    argv[1];
    std::string mstr = std::string(argv[1]);
    std::cout << mstr;
    if (std::string(argv[1]) == std::string("cpu"))
      sort = cpu;
    else if (std::string(argv[1]) == std::string("name"))
      sort = name;
    else if (std::string(argv[1]) == std::string("pid"))
      sort = pid;
  }

  
  int period = 1;
  system("clear");
  memory mem;
  mem.printInfo();
  std::cout << std::endl;
  processTable processes;
  processes.printHeader();
  processes.print(sort);
  int refresh;
  while (0) {
    system("clear");
    mem.refresh();
    mem.printInfo();
    processes.refresh();
    processes.printHeader();
    processes.print(sort);

    sleep(period);
  }
  
  return 0;
}