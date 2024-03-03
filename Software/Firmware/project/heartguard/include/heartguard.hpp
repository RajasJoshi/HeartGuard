#ifndef HEARTGUARD_HPP
#define HEARTGUARD_HPP

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>  // flock
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "ads1115.hpp"
#include "max30102.hpp"
// Add your function declarations, classes, etc. here

#endif  // HEARTGUARD_HPP