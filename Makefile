# ============================================================================
# UserlandVM - Unified Build System
# Supports: Classic VM, Haiku API Virtualizer, Modular, WebKit, Libroot
# ============================================================================

# Configuration
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -no-pie -O2 -Wall -Wextra -fPIC -g
CFLAGS = -std=c99 -O2 -Wall -Wextra -fPIC -g
INCLUDES = -I. -Iplatform -Iplatform/haiku -Ihaiku/headers
LDFLAGS = -lstdc++ -lpthread -ldl

# ============================================================================
# TARGETS
# ============================================================================

# Main targets
MASTER_VM = userlandvm
HAIKU_API = userlandvm_haiku_api

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

# Haiku API Virtualizer components (all 6 kits)
HAIKU_API_SOURCES = \
	haiku/implementation/HaikuAPIVirtualizer.cpp \
	haiku/implementation/HaikuSupportKit.cpp \
	haiku/implementation/HaikuStorageKit.cpp \
	haiku/implementation/HaikuInterfaceKit.cpp \
	haiku/implementation/HaikuInterfaceKitSimple.cpp \
	haiku/implementation/HaikuApplicationKit.cpp \
	haiku/implementation/HaikuNetworkKit.cpp

# Main entry points
MAIN_HAIKU_API = Main_HaikuAPI.cpp

# All sources
ALL_SOURCES = $(CORE_SOURCES) $(HAIKU_API_SOURCES) $(MAIN_HAIKU_API)
OBJECTS = $(ALL_SOURCES:.cpp=.o)

# ============================================================================
# BUILD RULES
# ============================================================================

.PHONY: all clean help haiku api debug install test

# Default: build main VM
all: $(MASTER_VM)

# Build Haiku API virtualizer
api: $(HAIKU_API)

$(MASTER_VM): $(filter-out $(HAIKU_API_SOURCES) $(MAIN_HAIKU_API),$(OBJECTS))
	@echo "Building UserlandVM..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(filter-out $(HAIKU_API_SOURCES) $(MAIN_HAIKU_API),$(ALL_SOURCES:.cpp=.o)) $(LDFLAGS)
	@echo "✅ Built: $@"

$(HAIKU_API): $(OBJECTS)
	@echo "================================================================="
	@echo "  Building UserlandVM Haiku API Virtualizer"
	@echo "================================================================="
	@echo "  ✨ Complete Haiku/BeOS API Implementation"
	@echo "  📁 Storage Kit | 🎨 Interface Kit | 🔗 Application Kit"
	@echo "  📦 Support Kit | 🌐 Network Kit | 🎬 Media Kit"
	@echo "================================================================="
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "✅ Haiku API Virtualizer built: $@"
	@ls -lh $@

%.o: %.cpp
	@echo "🔨 Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
debug: CXXFLAGS += -DDEBUG -O0
debug: all

# Clean
clean:
	@echo "🧹 Cleaning..."
	rm -f *.o $(MASTER_VM) $(HAIKU_API)
	@echo "✅ Clean completed"

# Install
install: $(HAIKU_API)
	@echo "📦 Installing..."
	sudo cp $(HAIKU_API) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(HAIKU_API)
	@echo "✅ Installed to /usr/local/bin/$(HAIKU_API)"

# Test
test: $(HAIKU_API)
	@echo "🧪 Testing..."
	./$(HAIKU_API) --test
	@echo "✅ Tests completed"

# Help
help:
	@echo "UserlandVM Build System"
	@echo "======================="
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build main VM (default)"
	@echo "  api       - Build Haiku API Virtualizer (all 6 kits)"
	@echo "  debug     - Build with debug flags"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to system"
	@echo "  test      - Run tests"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build main VM"
	@echo "  make api          # Build Haiku API virtualizer"
	@echo "  make api install  # Build and install"
	@echo ""
	@echo "Haiku Kits Available:"
	@echo "  • Storage Kit     - File system operations"
	@echo "  • Interface Kit  - GUI and windows"
	@echo "  • Application Kit - Messaging and app lifecycle"
	@echo "  • Support Kit    - BString, BList, etc."
	@echo "  • Network Kit    - Sockets and HTTP"
	@echo "  • Media Kit     - Audio/video"
