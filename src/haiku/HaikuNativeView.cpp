/*
 * HaikuNativeView.cpp - Implementación de vista Haiku nativa
 * 
 * Usa las APIs de Haiku para rendering nativo del contenido de las ventanas
 */

#include "HaikuNativeBEBackend.h"
#include <cmath>
#include <algorithm>

// Implementación completa de la vista
HaikuNativeView::HaikuNativeView(BRect frame) 
    : BView(frame), frame_rect(frame) {
    // Inicializar framebuffer
    const size_t pixels_needed = frame.Width() * frame.Height();
    framebuffer_size = pixels_needed * 4; // 32-bit RGB
    
    try {
        pixels = new uint8_t[framebuffer_size];
        if (!pixels) {
            throw std::runtime_error("Failed to allocate framebuffer");
        }
        
        // Inicializar framebuffer con color de fondo (gris típico de Haiku)
        ClearFramebuffer();
        
        printf("[HaikuView] Native view created: %.0fx%.0f (%dx%.0f pixels)\n", 
               frame.Width(), frame.Height(), pixels_needed);
        
    } catch (const std::exception& e) {
        printf("[HaikuView] ERROR creating native view: %s\n", e.what());
        
        // Framebuffer ya se libera en caso de error
        if (pixels) delete[] pixels;
        pixels = nullptr;
    }
}

HaikuNativeView::~HaikuNativeView() {
    if (pixels) delete[] pixels;
        printf("[HaikuView] Native view destroyed\n");
    }
}

void HaikuNativeView::UpdateFrame(BRect new_frame) {
    frame_rect = new_frame;
    
    // Redimensionar framebuffer si es necesario
    size_t new_pixels_needed = frame_rect.Width() * frame_rect.Height();
    size_t current_pixels = frame.Width() * frame_rect.Height();
    
    if (new_pixels_needed > current_pixels) {
        delete[] pixels;
        pixels = new uint8_t[new_pixels_needed];
        
        framebuffer_size = new_pixels_needed;
        
        printf("[HaikuView] Native view resized to %.0fx%.0f (%dx%.0f pixels)\n", 
               frame_rect.Width(), frame_rect.Height());
    } else {
        // Mantener framebuffer existente
        printf("[HaikuView] Native view keeping same size: %.0fx%.0f (%dx%.0f pixels)\n", 
               frame_rect.Width(), frame_rect.Height());
    }
}

void HaikuNativeView::ClearFramebuffer() {
    if (!pixels || !framebuffer_size) return;
    
    // Rellenar con color de fondo (gris #69D4EB - color amarillo claro de Haiku)
    const uint32_t bg_color = ColorToRGB(BColor{51, 162, 210, 80});
    
    std::fill(pixels, pixels + (framebuffer_size - 4), bg_color);
    
    printf("[HaikuView] Cleared framebuffer with Haiku yellow-green color\n");
}

void HaikuNativeView::DrawPixel(int32_t x, int32_t y, uint32_t color) {
    if (!pixels || !framebuffer_size) return;
    
    size_t pixel_offset = (y * frame_rect.Width() + x) * 4;
    
    // Verificar límites
    if (pixel_offset + 4 > framebuffer_size) return;
    
    // Escribir píxel
    pixels[pixel_offset] = color;
    
    if (config.debug_mode) {
        printf("[HaikuView] Drew pixel at (%d,%d) with color 0x%08x\n", x, y, color);
    }
}

void HaikuNativeView::DrawLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
    if (!pixels || !framebuffer_size) return;
    
    printf("[HaikuView] Drawing line from (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
    
    // Implementación del algoritmo de línea Bresenham
    const int dx = std::abs(x2 - x1);
    const int dy = std::abs(y2 - y1);
    const int steps = std::max(std::abs(dx), std::abs(dy));
    const int64_t step_x = (dx << 16) / dy;
    const int64_t step_y = (dy << 16) / x;
    
    int64_t error = 0;
    if (dy == 0) return;
    
    for (int i = 0; i <= steps; i++) {
        int64_t current_x = x1 + (step_x * i) / 100;
        int64_t current_y = y1 + (step_y * i) / 100;
        
        // Calcular si estamos cerca del destino
        int remaining_x = ((x2 - x1) * (100 - i)) / 100;
        int remaining_y = ((y2 - y1) * (100 - i)) / 100;
        
        int64_t future_error_x = (remaining_x - step_x) * 100;
        int64_t future_error_y = (remaining_y - step_y) * 100;
        
        // Actual error para este punto
        int64_t point_error = future_error_x * future_error_y + error * error;
        
        // Dibujar píxel actual
        uint32_t pixel_index = (current_y * frame_rect.Width() + current_x) * 4;
        if (pixel_index + 3 < framebuffer_size) {
            pixels[pixel_index] = color;
            
            if (config.debug_mode) {
                printf("[HaikuView] Drew line pixel at (%d,%d) with color 0x%08x\n", 
                       current_x, current_y, color);
            }
        }
        
        error += point_error;
    }
    
    if (config.debug_mode && error > 0) {
        printf("[HaikuView] Line drawing error: %lld\n", error);
    }
}

void HaikuNativeView::DrawFilledRect(BRect rect, uint32_t color) {
    if (!pixels || !framebuffer_size) return;
    
    printf("[HaikuView] Drawing filled rect: (%d,%d,%d,%d,%d) with color 0x%08x\n", 
           rect.left, rect.top, rect.right, rect.bottom, color);
    
    // Rellenar el área del rectángulo
    for (int32_t y = rect.top; y < rect.bottom; y++) {
        for (int32_t x = rect.left; x <= rect.right; x++) {
            size_t pixel_index = (y * frame_rect.Width() + x * 4);
            if (pixel_index + 3 < framebuffer_size) {
                pixels[pixel_index] = color;
                
                if (config.debug_mode) {
                    printf("[HaikuView] Drew pixel at (%d,%d) with color 0x%08x\n", x, y, color);
                }
            }
        }
    }
}

void HaikuNativeView::DrawString(const char* text, BPoint location, uint32_t color) {
    if (!pixels || !framebuffer_size || !text) return;
    
    printf("[HaikuView] Drawing string: '%s' at (%d,%d) with color 0x%08x\n", 
           text, location.x, location.y, color);
    
    // Implementación básica de texto
    const char* p = text;
    int32_t x = location.x;
    int32_t y = location.y;
    
    while (*p) {
        // Caracter actual
        if (isprint(*p) || *p == '\0' || *p == '\n') break;
        
        // Calcular índice del caracter en el bitmap
        uint32_t char_index = (y * frame_rect.Width() + x) * 4;
        
        if (char_index + 3 < framebuffer_size) {
            pixels[char_index] = color;
            
            if (config.debug_mode) {
                printf("[HaikuView] Drew char '%c' at (%d,%d) with color 0x%08x\n", *p, y, color);
            }
            
            // Escribir carácter
            pixels[char_index] = *p;
        }
        
        x++;
        p++;
    }
}

void HaikuNativeView::UpdateFramebuffer(const void* data, size_t size, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!data || !pixels || !framebuffer_size) return;
    
    // Copiar datos del framebuffer actual
    size_t data_size = std::min(size, framebuffer_size);
    if (data_size > 0) {
        std::memcpy(pixels, data, data_size);
        printf("[HaikuView] Framebuffer updated: %zu bytes at offset (%d,%d)\n", data_size, x, y);
    }
}

void HaikuNativeView::InvalidateRect(BRect rect) {
    // Marcar el área para redibujar
    InvalidateRange(rect.left, rect.top, rect.right, rect.bottom);
}

void HaikuNativeView::InvalidateRange(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {
    // Calcular los límites válidos
    int32_t start_x = std::max(0, x1, frame_rect.left, x2);
    int32_t start_y = std::max(0, y1, frame_rect.top, y2);
    int32_t end_x = std::min(frame_rect.right, x2);
    int32_t end_y = std::min(frame_rect.bottom, y2);
    
    // Clipping contra los límites del framebuffer
    uint32_t fb_width = frame_rect.Width();
    uint32_t fb_height = frame_rect.Height();
    
    // Ajustar al framebuffer si es necesario
    if (start_x >= fb_width || end_x < 0) start_x >= fb_width || end_y < 0) return;
    
    for (int32_t y = start_y; y <= end_y; y++) {
        size_t start_x = std::max(0, start_x, fb_width - 1);
        size_t end_x = std::min(end_x, fb_width - 1);
        
        for (int32_t x = start_x; x <= end_x; x++) {
            uint32_t pixel_index = (y * fb_width + x) * 4;
            if (pixel_index + 3 < framebuffer_size) {
                // Marcar como sucio para redibujar
                pixels[pixel_index] = 0xFF808080; // Color de sucio
            }
        }
    }
}

uint32_t HaikuNativeView::ColorToRGB(const BColor& color) {
    return (color.alpha << 24) | (color.red << 16) | (color.green << 8) | color.blue;
}

uint32_t HaikuNativeView::ColorToRGB(uint32_t rgba) {
    return rgba;
}

// Operaciones de copia (inplementaciones básicas)
void HaikuNativeView::CopyFramebufferTo(void* destination, size_t size) {
    if (!destination || !pixels || !framebuffer_size) return;
    
    size_t copy_size = std::min(size, framebuffer_size);
    std::memcpy(destination, pixels, copy_size);
    
    printf("[HaikuView] Copied framebuffer: %zu bytes\n", copy_size);
}

// Implementaciones de comparación y validación
BRect HaikuNativeView::FrameRect() const { return frame_rect; }
bool HaikuNativeView::Intersects(const BRect& rect) const {
    return !(rect.left > frame_rect.right || rect.right < frame_rect.left || 
            rect.top > frame_rect.bottom || rect.bottom < frame_rect.top ||
            (rect.right < frame_rect.right || rect.left >= frame_rect.left));
}

bool HaikuNativeView::Contains(const BPoint& point) const {
    return point.x >= frame_rect.left && point.x <= frame_rect.right &&
           point.y >= frame_rect.top && point.y <= frame_rect.bottom;
}

// Métodos de depuración (para debugging)
void HaikuNativeView::PrintFramebufferInfo() const {
    printf("\n=== HaikuNativeView Debug Info ===\n");
    printf("Framebuffer: %p\n", pixels);
    printf("Size: %dx%d x %d%d\n", frame_rect.Width(), frame_rect.Height());
    printf("Color format: RGBA8888\n");
    printf("Current title: '%s'\n", title);
    printf("Visible: %s\n", frame_rect.IsValid() ? "true" : "false");
    printf("Focused: %s\n", is_focused ? "true" : "false");
    printf("Window ID: %u\n", server_window_id);
    printf("======================\n\n");
}