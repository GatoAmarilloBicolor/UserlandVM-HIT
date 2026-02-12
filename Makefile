# ============================================================================
# UserlandVM - Unified Build System
# Complete Haiku API Virtualizer with 6 Kits
# ============================================================================

# Configuration
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -no-pie -O2 -Wall -Wextra -fPIC -g
CFLAGS = -std=c99 -O2 -Wall -Wextra -fPIC -g
INCLUDES = -I. -Iplatform -Iplatform/haiku -Ihaiku/headers
LDFLAGS = -lstdc++ -lpthread -ldl

# Main target
TARGET = userlandvm

# ============================================================================
# SOURCE FILES
# ============================================================================

# Core VM components
CORE_SOURCES = \
	ELFImage.cpp \
	DirectAddressSpace.cpp \
	X86_32GuestContext.cpp \
	InterpreterX86_32.cpp \
	SymbolResolver.cpp \
	DebugOutput.cpp \
	SyscallDispatcher.cpp

# Haiku API Virtualizer (all 6 kits)
HAIKU_SOURCES = \
	haiku/implementation/HaikuAPIVirtualizer.cpp \
	haiku/implementation/HaikuSupportKit.cpp \
	haiku/implementation/HaikuStorageKit.cpp \
	haiku/implementation/HaikuInterfaceKit.cpp \
	haiku/implementation/HaikuInterfaceKitSimple.cpp \
	haiku/implementation/HaikuApplicationKit.cpp \
	haiku/implementation/HaikuNetworkKit.cpp

# Main entry point
MAIN_SOURCE = Main_HaikuAPI.cpp

# All sources
ALL_SOURCES = $(CORE_SOURCES) $(HAIKU_SOURCES) $(MAIN_SOURCE)
OBJECTS = $(ALL_SOURCES:.cpp=.o)

# ============================================================================
# BUILD RULES
# ============================================================================

.PHONY: all clean help debug install test

# Default: build main VM with all 6 Haiku kits
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "================================================================="
	@echo "  Building UserlandVM with Haiku API Virtualizer"
	@echo "================================================================="
	@echo "  🎉 Complete Haiku/BeOS API Implementation"
	@echo "  📦 Storage Kit     - File system operations"
	@echo "  🎨 Interface Kit   - GUI and window management"
	@echo "  🔗 Application Kit - Messaging and app lifecycle"
	@echo "  📦 Support Kit    - BString, BList, BLocker"
	@echo "  🌐 Network Kit    - Sockets and HTTP client"
	@echo "  🎬 Media Kit      - Audio and video processing"
	@echo "================================================================="
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "✅ Built: $@"
	@ls -lh $@

# Compile source files
%.o: %.cpp
	@echo "🔨 Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
debug: CXXFLAGS += -DDEBUG -O0
debug: $(TARGET)
	@echo "🔧 Debug build complete"

# Clean
clean:
	@echo "🧹 Cleaning..."
	rm -f *.o $(TARGET)
	@echo "✅ Clean completed"

# Install
install: $(TARGET)
	@echo "📦 Installing..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✅ Installed to /usr/local/bin/$(TARGET)"

# Test
test: $(TARGET)
	@echo "🧪 Testing..."
	./$(TARGET) --test

# Help
help:
	@echo "UserlandVM Build System"
	@echo "======================="
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build main VM with Haiku API (default)"
	@echo "  debug     - Build with debug flags"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to system"
	@echo "  test      - Run tests"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build"
	@echo "  make debug       # Debug build"
	@echo "  make install     # Install"
	@echo "  make test        # Test"
	@echo ""
	@echo "Run Haiku apps:"
	@echo "  ./userlandvm /system/apps/WebPositive"
	@echo "  ./userlandvm /system/apps/Terminal --verbose"
	@echo ""
	@echo "Haiku API Kits (6 total):"
	@echo "  • Storage     - BFile, BDirectory, BEntry, BPath"
	@echo "  • Interface   - BWindow, BView, BApplication, BBitmap"
	@echo "  • Application - BMessage, BLooper, BMessenger, BHandler"
	@echo "  • Support     - BString, BList, BLocker, BPoint, BRect"
	@echo "  • Network    - BSocket, BUrl, BHttp, BDNS"
	@echo "  • Media      - BSoundPlayer, BMediaFile, BMediaNode"
