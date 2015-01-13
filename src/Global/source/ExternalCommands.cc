//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#include "ExternalCommands.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <signal.h>

static int ext_cmd_pipes[2]; // pipes to communicate with child
static int ext_cmd_return[2]; // return value
static pid_t child_pid = -1;

void external_commands_init(void){
  if (pipe(ext_cmd_pipes)<0){
    perror("failed to create pipes");
    exit(1);
  }

  if (pipe(ext_cmd_return)<0){
    perror("failed to create pipes");
    exit(1);
  }


  if ((child_pid = fork()) < 0)
    perror("failed to fork");

  if (child_pid == 0) {
    // the child
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    close(ext_cmd_pipes[1]);
    close(ext_cmd_return[0]);
    fcntl(ext_cmd_pipes[0], F_SETFD, FD_CLOEXEC);
    fcntl(ext_cmd_return[1], F_SETFD, FD_CLOEXEC);

    for (;;){ // repeat forever
      char buffer[1024];

      if (read(ext_cmd_pipes[0], buffer, 1024)==0){
          sleep(1);
          break;
      }
#ifdef DEBUG
      printf("Executing the command: %s\n", buffer);
#endif
      int ret = system(buffer);
#ifdef DEBUG
      printf("Command finished with return val=%d\n", ret);
#endif
      if (write(ext_cmd_return[1], &ret, sizeof(int))!=sizeof(int)){
        perror("failed to write an int as a result");
        exit(1);
      }
      fsync(ext_cmd_return[1]);
    }
  } else {
    // the parent
    close(ext_cmd_pipes[0]);
    close(ext_cmd_return[1]);
    sleep(1);
  }
}


int execute_command(const char* command){
  write(ext_cmd_pipes[1], command, strlen(command)+1);
  fsync(ext_cmd_pipes[1]);
  int rez;
  if (read(ext_cmd_return[0], &rez, sizeof(int))!=sizeof(int)){
    printf("WHY AM I NOT GETTING MY REPLY?\n");
    exit(1);
  }

  return rez;
}

