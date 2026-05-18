# =========================================================
# Sulfur++ Build System
# Runtime: combust
# =========================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra -Wpedantic
DEBUGFLAG := -g -DDEBUG -O0

LDFLAGS   :=

TARGET    := combust

SRCDIR    := src
INCDIR    := include
BUILDDIR  := build
EXAMPLEDIR:= examples

# =========================================================
# Source Discovery
# =========================================================

SRCS := $(wildcard $(SRCDIR)/*.cpp)

OBJS := $(patsubst $(SRCDIR)/%.cpp,\
        $(BUILDDIR)/%.o,\
        $(SRCS))

# =========================================================
# Colors
# =========================================================

GREEN  := \033[0;32m
RED    := \033[0;31m
BLUE   := \033[0;34m
YELLOW := \033[1;33m
NC     := \033[0m

# =========================================================
# PHONY
# =========================================================

.PHONY: all debug clean run example install help format init

# =========================================================
# Default Build
# =========================================================

all: $(TARGET)

# =========================================================
# Debug Build
# =========================================================

debug: CXXFLAGS += $(DEBUGFLAG)
debug: $(TARGET)

# =========================================================
# Linking
# =========================================================

$(TARGET): $(OBJS)
	@echo "$(BLUE)[LINKING]$(NC) Building $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)[SUCCESS]$(NC) Built: $(TARGET)"

# =========================================================
# Object Compilation
# =========================================================

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo "$(YELLOW)[COMPILING]$(NC) $<"
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

# =========================================================
# Run Runtime
# =========================================================

run: $(TARGET)
	@echo "$(BLUE)[RUNNING]$(NC) Launching runtime..."
	./$(TARGET)

# =========================================================
# Run Example
# =========================================================

example: $(TARGET)
	@echo "$(BLUE)[EXAMPLE]$(NC) Running example..."
	./$(TARGET) $(EXAMPLEDIR)/hello.sfpp

# =========================================================
# Clean Build
# =========================================================

clean:
	@echo "$(RED)[CLEAN]$(NC) Removing build files..."
	rm -rf $(BUILDDIR)
	rm -f $(TARGET)

# =========================================================
# Install Runtime
# =========================================================

install: $(TARGET)
	@echo "$(BLUE)[INSTALL]$(NC) Installing combust..."
	cp $(TARGET) /usr/local/bin/combust
	@echo "$(GREEN)[SUCCESS]$(NC) Installed to /usr/local/bin"

# =========================================================
# Formatter Placeholder
# =========================================================

format:
	@echo "$(YELLOW)[FORMAT]$(NC) Formatting source..."
	@echo "sfmt not implemented yet."

# =========================================================
# Initialize Project Structure
# =========================================================

init:
	@mkdir -p $(SRCDIR)
	@mkdir -p $(INCDIR)
	@mkdir -p $(EXAMPLEDIR)
	@mkdir -p $(BUILDDIR)

	@touch $(SRCDIR)/main.cpp
	@touch $(EXAMPLEDIR)/hello.sfpp

	@echo "$(GREEN)[SUCCESS]$(NC) Sulfur++ project initialized."

# =========================================================
# Help
# =========================================================

help:
	@echo ""
	@echo "Sulfur++ Build System"
	@echo "==============================="
	@echo "make           -> Release build"
	@echo "make debug     -> Debug build"
	@echo "make run       -> Run runtime"
	@echo "make example   -> Run example"
	@echo "make clean     -> Clean build"
	@echo "make install   -> Install combust"
	@echo "make init      -> Initialize folders"
	@echo "make format    -> Format source"
	@echo ""