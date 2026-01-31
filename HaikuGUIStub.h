/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#pragma once

#include <SupportDefs.h>
#include <map>
#include <string>

/**
 * Stub implementation of Haiku GUI API (BApplication, BWindow, BView, etc.)
 * Provides virtual implementation so GUI programs can run without crashing
 * Output is rendered to an internal framebuffer that can be dumped or streamed
 */

// Haiku GUI message constants (subset)
#define B_QUIT_REQUESTED 1
#define B_WINDOW_ACTIVATED 2
#define B_MOUSE_DOWN 3
#define B_MOUSE_UP 4
#define B_MOUSE_MOVED 5
#define B_KEY_DOWN 6
#define B_KEY_UP 7

// View flags
#define B_WILL_DRAW 0x01
#define B_FRAME_EVENTS 0x02
#define B_NAVIGATE_CHARS 0x04
#define B_PULSE_NEEDED 0x08

/**
 * Virtual framebuffer for GUI rendering
 * Each pixel is 32-bit RGBA
 */
class VirtualFramebuffer {
public:
	VirtualFramebuffer(uint32_t width, uint32_t height);
	~VirtualFramebuffer();

	void Clear(uint32_t color);
	void SetPixel(uint32_t x, uint32_t y, uint32_t color);
	uint32_t GetPixel(uint32_t x, uint32_t y) const;
	void FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
	void DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);

	uint32_t Width() const { return fWidth; }
	uint32_t Height() const { return fHeight; }
	const uint32_t* Data() const { return fData; }

private:
	uint32_t fWidth;
	uint32_t fHeight;
	uint32_t* fData;
};

/**
 * Virtual BView implementation
 */
class VirtualBView {
public:
	VirtualBView(const char* name, uint32_t flags = 0);
	~VirtualBView();

	void SetFrame(float x, float y, float w, float h);
	void GetFrame(float& x, float& y, float& w, float& h) const;

	void SetViewColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	void GetViewColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

	void DrawString(const char* string, float x, float y);
	void FillRect(float x, float y, float w, float h);
	void StrokeRect(float x, float y, float w, float h);

	const char* Name() const { return fName.c_str(); }
	uint32_t Flags() const { return fFlags; }
	
	VirtualFramebuffer* Framebuffer() { return fFramebuffer; }
	void SetFramebuffer(VirtualFramebuffer* fb) { fFramebuffer = fb; }

private:
	std::string fName;
	uint32_t fFlags;
	float fX, fY, fWidth, fHeight;
	uint8_t fViewColor[4];  // RGBA
	VirtualFramebuffer* fFramebuffer;
};

/**
 * Virtual BWindow implementation
 */
class VirtualBWindow {
public:
	VirtualBWindow(const char* title, float x, float y, float w, float h);
	~VirtualBWindow();

	void AddChild(VirtualBView* view);
	void RemoveChild(VirtualBView* view);
	VirtualBView* FindView(const char* name);

	void SetTitle(const char* title);
	const char* GetTitle() const { return fTitle.c_str(); }

	void Show();
	void Hide();
	bool IsHidden() const { return fHidden; }

	void SetFrame(float x, float y, float w, float h);
	void GetFrame(float& x, float& y, float& w, float& h) const;

	VirtualFramebuffer* Framebuffer() { return &fFramebuffer; }
	
	uint32_t ID() const { return fID; }

private:
	uint32_t fID;
	std::string fTitle;
	float fX, fY, fWidth, fHeight;
	bool fHidden;
	VirtualFramebuffer fFramebuffer;
	std::map<std::string, VirtualBView*> fChildren;

	static uint32_t sNextID;
};

/**
 * Virtual BApplication implementation
 */
class VirtualBApplication {
public:
	VirtualBApplication();
	~VirtualBApplication();

	void AddWindow(VirtualBWindow* window);
	void RemoveWindow(VirtualBWindow* window);
	VirtualBWindow* FindWindow(const char* title);
	VirtualBWindow* FindWindowByID(uint32_t id);

	void Quit();
	bool IsQuitting() const { return fQuitting; }

	int32_t Run();  // Returns exit code

	// Get all windows
	const std::map<uint32_t, VirtualBWindow*>& Windows() const { return fWindows; }

private:
	std::map<uint32_t, VirtualBWindow*> fWindows;
	bool fQuitting;
	int32_t fExitCode;
};

/**
 * Global GUI state manager
 */
class HaikuGUIState {
public:
	static HaikuGUIState& Instance();

	VirtualBApplication* GetApplication() { return fApplication; }
	void SetApplication(VirtualBApplication* app) { fApplication = app; }

	void RegisterWindow(uint32_t id, VirtualBWindow* window);
	VirtualBWindow* FindWindow(uint32_t id);
	void UnregisterWindow(uint32_t id);

	// Screenshot functionality
	bool DumpFramebuffer(const char* filename);

private:
	HaikuGUIState();
	~HaikuGUIState();

	VirtualBApplication* fApplication;
	std::map<uint32_t, VirtualBWindow*> fWindowMap;
};
