/*
 * Copyright 2025, Haiku Imposible Team.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "HaikuGUIStub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// VirtualFramebuffer Implementation
// ============================================================================

VirtualFramebuffer::VirtualFramebuffer(uint32_t width, uint32_t height)
	: fWidth(width), fHeight(height)
{
	fData = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	if (fData) {
		memset(fData, 0, width * height * sizeof(uint32_t));
	}
}

VirtualFramebuffer::~VirtualFramebuffer()
{
	if (fData) {
		free(fData);
		fData = NULL;
	}
}

void VirtualFramebuffer::Clear(uint32_t color)
{
	if (!fData) return;
	for (uint32_t i = 0; i < fWidth * fHeight; i++) {
		fData[i] = color;
	}
}

void VirtualFramebuffer::SetPixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x >= fWidth || y >= fHeight) return;
	fData[y * fWidth + x] = color;
}

uint32_t VirtualFramebuffer::GetPixel(uint32_t x, uint32_t y) const
{
	if (x >= fWidth || y >= fHeight) return 0;
	return fData[y * fWidth + x];
}

void VirtualFramebuffer::FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	for (uint32_t yy = y; yy < y + h && yy < fHeight; yy++) {
		for (uint32_t xx = x; xx < x + w && xx < fWidth; xx++) {
			SetPixel(xx, yy, color);
		}
	}
}

void VirtualFramebuffer::DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color)
{
	// Simple Bresenham's line algorithm
	int dx = (int)x2 - (int)x1;
	int dy = (int)y2 - (int)y1;
	int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);
	
	if (steps == 0) {
		SetPixel(x1, y1, color);
		return;
	}
	
	float xIncrement = (float)dx / steps;
	float yIncrement = (float)dy / steps;
	
	float x = (float)x1;
	float y = (float)y1;
	
	for (int i = 0; i <= steps; i++) {
		SetPixel((uint32_t)x, (uint32_t)y, color);
		x += xIncrement;
		y += yIncrement;
	}
}

// ============================================================================
// VirtualBView Implementation
// ============================================================================

VirtualBView::VirtualBView(const char* name, uint32_t flags)
	: fName(name ? name : "View"), fFlags(flags), fX(0), fY(0), fWidth(100), fHeight(100),
	  fFramebuffer(NULL)
{
	fViewColor[0] = 192;  // R
	fViewColor[1] = 192;  // G
	fViewColor[2] = 192;  // B
	fViewColor[3] = 255;  // A
}

VirtualBView::~VirtualBView()
{
	// Don't delete framebuffer - it's owned by window
}

void VirtualBView::SetFrame(float x, float y, float w, float h)
{
	fX = x;
	fY = y;
	fWidth = w;
	fHeight = h;
}

void VirtualBView::GetFrame(float& x, float& y, float& w, float& h) const
{
	x = fX;
	y = fY;
	w = fWidth;
	h = fHeight;
}

void VirtualBView::SetViewColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	fViewColor[0] = r;
	fViewColor[1] = g;
	fViewColor[2] = b;
	fViewColor[3] = a;
}

void VirtualBView::GetViewColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const
{
	r = fViewColor[0];
	g = fViewColor[1];
	b = fViewColor[2];
	a = fViewColor[3];
}

void VirtualBView::DrawString(const char* string, float x, float y)
{
	if (!fFramebuffer || !string) return;
	
	// Simple stub - just log for now
	printf("[GUI] DrawString at (%.0f, %.0f): %s\n", x, y, string);
}

void VirtualBView::FillRect(float x, float y, float w, float h)
{
	if (!fFramebuffer) return;
	
	uint32_t color = (fViewColor[0] << 24) | (fViewColor[1] << 16) | 
	                  (fViewColor[2] << 8) | fViewColor[3];
	
	fFramebuffer->FillRect((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, color);
}

void VirtualBView::StrokeRect(float x, float y, float w, float h)
{
	if (!fFramebuffer) return;
	
	uint32_t color = (fViewColor[0] << 24) | (fViewColor[1] << 16) | 
	                  (fViewColor[2] << 8) | fViewColor[3];
	
	// Draw rectangle outline
	fFramebuffer->DrawLine((uint32_t)x, (uint32_t)y, (uint32_t)(x+w), (uint32_t)y, color);
	fFramebuffer->DrawLine((uint32_t)(x+w), (uint32_t)y, (uint32_t)(x+w), (uint32_t)(y+h), color);
	fFramebuffer->DrawLine((uint32_t)(x+w), (uint32_t)(y+h), (uint32_t)x, (uint32_t)(y+h), color);
	fFramebuffer->DrawLine((uint32_t)x, (uint32_t)(y+h), (uint32_t)x, (uint32_t)y, color);
}

// ============================================================================
// VirtualBWindow Implementation
// ============================================================================

uint32_t VirtualBWindow::sNextID = 1;

VirtualBWindow::VirtualBWindow(const char* title, float x, float y, float w, float h)
	: fID(sNextID++), fTitle(title ? title : "Window"), fX(x), fY(y), fWidth(w), fHeight(h),
	  fHidden(true), fFramebuffer(800, 600)
{
	printf("[GUI] BWindow created: \"%s\" (%d) at (%.0f, %.0f) size (%.0f x %.0f)\n",
		fTitle.c_str(), fID, fX, fY, fWidth, fHeight);
	
	// Register with global state
	HaikuGUIState::Instance().RegisterWindow(fID, this);
}

VirtualBWindow::~VirtualBWindow()
{
	for (auto& pair : fChildren) {
		delete pair.second;
	}
	fChildren.clear();
	
	HaikuGUIState::Instance().UnregisterWindow(fID);
	printf("[GUI] BWindow destroyed: %d\n", fID);
}

void VirtualBWindow::AddChild(VirtualBView* view)
{
	if (!view) return;
	
	std::string name = view->Name();
	fChildren[name] = view;
	view->SetFramebuffer(&fFramebuffer);
	
	printf("[GUI] View added to window %d: %s\n", fID, name.c_str());
}

void VirtualBWindow::RemoveChild(VirtualBView* view)
{
	if (!view) return;
	
	std::string name = view->Name();
	auto it = fChildren.find(name);
	if (it != fChildren.end()) {
		fChildren.erase(it);
		printf("[GUI] View removed from window %d: %s\n", fID, name.c_str());
	}
}

VirtualBView* VirtualBWindow::FindView(const char* name)
{
	if (!name) return NULL;
	
	auto it = fChildren.find(name);
	if (it != fChildren.end()) {
		return it->second;
	}
	return NULL;
}

void VirtualBWindow::SetTitle(const char* title)
{
	if (title) {
		fTitle = title;
		printf("[GUI] Window %d title changed to: %s\n", fID, fTitle.c_str());
	}
}

void VirtualBWindow::Show()
{
	fHidden = false;
	printf("[GUI] Window %d shown\n", fID);
	
	// Render all views
	fFramebuffer.Clear(0xC0C0C0FF);  // Gray background
}

void VirtualBWindow::Hide()
{
	fHidden = true;
	printf("[GUI] Window %d hidden\n", fID);
}

void VirtualBWindow::SetFrame(float x, float y, float w, float h)
{
	fX = x;
	fY = y;
	fWidth = w;
	fHeight = h;
	printf("[GUI] Window %d frame changed to (%.0f, %.0f, %.0f x %.0f)\n",
		fID, fX, fY, fWidth, fHeight);
}

void VirtualBWindow::GetFrame(float& x, float& y, float& w, float& h) const
{
	x = fX;
	y = fY;
	w = fWidth;
	h = fHeight;
}

// ============================================================================
// VirtualBApplication Implementation
// ============================================================================

VirtualBApplication::VirtualBApplication()
	: fQuitting(false), fExitCode(0)
{
	printf("[GUI] BApplication created\n");
	HaikuGUIState::Instance().SetApplication(this);
}

VirtualBApplication::~VirtualBApplication()
{
	for (auto& pair : fWindows) {
		delete pair.second;
	}
	fWindows.clear();
	
	HaikuGUIState::Instance().SetApplication(NULL);
	printf("[GUI] BApplication destroyed\n");
}

void VirtualBApplication::AddWindow(VirtualBWindow* window)
{
	if (!window) return;
	fWindows[window->ID()] = window;
	printf("[GUI] Window %d added to application\n", window->ID());
}

void VirtualBApplication::RemoveWindow(VirtualBWindow* window)
{
	if (!window) return;
	auto it = fWindows.find(window->ID());
	if (it != fWindows.end()) {
		fWindows.erase(it);
		printf("[GUI] Window %d removed from application\n", window->ID());
	}
}

VirtualBWindow* VirtualBApplication::FindWindow(const char* title)
{
	if (!title) return NULL;
	
	for (auto& pair : fWindows) {
		if (pair.second->GetTitle() == title) {
			return pair.second;
		}
	}
	return NULL;
}

VirtualBWindow* VirtualBApplication::FindWindowByID(uint32_t id)
{
	auto it = fWindows.find(id);
	if (it != fWindows.end()) {
		return it->second;
	}
	return NULL;
}

void VirtualBApplication::Quit()
{
	fQuitting = true;
	printf("[GUI] BApplication quit requested\n");
}

int32_t VirtualBApplication::Run()
{
	printf("[GUI] BApplication::Run() called\n");
	
	// Simple event loop simulation
	// In a real implementation, this would process messages
	int iterations = 0;
	while (!fQuitting && iterations < 10000) {
		iterations++;
		// Simulate event processing
	}
	
	printf("[GUI] BApplication::Run() returning with code %d\n", fExitCode);
	return fExitCode;
}

// ============================================================================
// HaikuGUIState Implementation
// ============================================================================

HaikuGUIState& HaikuGUIState::Instance()
{
	static HaikuGUIState instance;
	return instance;
}

HaikuGUIState::HaikuGUIState()
	: fApplication(NULL)
{
	printf("[GUI] GUI State initialized\n");
}

HaikuGUIState::~HaikuGUIState()
{
	printf("[GUI] GUI State destroyed\n");
}

void HaikuGUIState::RegisterWindow(uint32_t id, VirtualBWindow* window)
{
	if (window) {
		fWindowMap[id] = window;
	}
}

VirtualBWindow* HaikuGUIState::FindWindow(uint32_t id)
{
	auto it = fWindowMap.find(id);
	if (it != fWindowMap.end()) {
		return it->second;
	}
	return NULL;
}

void HaikuGUIState::UnregisterWindow(uint32_t id)
{
	auto it = fWindowMap.find(id);
	if (it != fWindowMap.end()) {
		fWindowMap.erase(it);
	}
}

bool HaikuGUIState::DumpFramebuffer(const char* filename)
{
	if (!filename) return false;
	
	printf("[GUI] Dumping framebuffers to file: %s\n", filename);
	
	// For now, just log - proper PPM/PNG dump would go here
	FILE* f = fopen(filename, "w");
	if (!f) return false;
	
	fprintf(f, "# Haiku GUI Framebuffer Dump\n");
	fprintf(f, "# Windows: %zu\n", fWindowMap.size());
	
	for (const auto& pair : fWindowMap) {
		VirtualBWindow* window = pair.second;
		fprintf(f, "# Window %u: %s\n", pair.first, window->GetTitle());
	}
	
	fclose(f);
	return true;
}
