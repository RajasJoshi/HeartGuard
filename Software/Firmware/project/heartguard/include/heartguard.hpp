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
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>

#include "ads1115.hpp"
#include "max30102.hpp"

// Add your function declarations, classes, etc. here

#endif  // HEARTGUARD_HPP