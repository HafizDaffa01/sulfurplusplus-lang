CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter
LDFLAGS  :=
TARGET   := combust
SRCDIR   := src
INCDIR   := include
BUILDDIR := build

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all clean debug install

all: $(TARGET)

debug: CXXFLAGS += -g -DDEBUG -O0
debug: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Built: $(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(wildcard $(INCDIR)/*.hpp)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

install: $(TARGET)
	@cp $(TARGET) /usr/local/bin/combust
	@echo "✓ Installed: /usr/local/bin/combust"

clean:
	rm -rf $(BUILDDIR) $(TARGET)
