#ifndef HAIKU_GUI_BACKEND_H
#define HAIKU_GUI_BACKEND_H

#include <os/app/Application.h>
#include <os/interface/Window.h>
#include <os/interface/View.h>
#include <iostream>

using namespace BPrivate;

class VMApplicationWindow : public BWindow {
public:
    VMApplicationWindow(const char* title);
    virtual ~VMApplicationWindow();
    virtual void MessageReceived(BMessage* msg);
    virtual bool QuitRequested();
    
private:
    BView* view;
};

class VMApplication : public BApplication {
public:
    VMApplication(const char* app_name);
    virtual ~VMApplication();
    virtual void ReadyToRun();
    
private:
    BWindow* main_window;
};

// Global helper functions
void CreateHaikuWindow(const char* title);
void ShowHaikuWindow();
void ProcessWindowEvents();
void DestroyHaikuWindow();

#endif // HAIKU_GUI_BACKEND_H
