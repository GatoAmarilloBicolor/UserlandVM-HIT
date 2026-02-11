#ifndef SIMPLE_HAIKU_GUI_H
#define SIMPLE_HAIKU_GUI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Simple GUI functions that create a basic window using app_server
extern "C" {
void CreateHaikuWindow(const char* title);
void ShowHaikuWindow();
void ProcessWindowEvents();
void DestroyHaikuWindow();
}

#endif
