# Makefile for Video-AI Processing Pipeline
# Compiles: Input_Collector, Master_Splitter, Merge_Master, Output_Dispatcher, Video_AI_Coordinator

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
INCLUDES = -I. -IInput_Collector -IMaster_Splitter -IMerge_Master -IOutput_Dispatcher -IVideo_AI_Coordinator
LIBS = -lcurl -lstdc++fs -pthread

# Source files with new folder structure
INPUT_COLLECTOR_SOURCES = Input_Collector/Input_Collector.cpp
MASTER_SPLITTER_SOURCES = Master_Splitter/Master_Splitter.cpp
MERGE_MASTER_SOURCES = Merge_Master/Merge_Master.cpp
OUTPUT_DISPATCHER_SOURCES = Output_Dispatcher/Output_Dispatcher.cpp
COORDINATOR_SOURCES = Video_AI_Coordinator/Video_AI_Coordinator.cpp
USAGE_SOURCES = Video_AI_Coordinator/Video_AI_Pipeline_Usage.cpp

# Object files
INPUT_COLLECTOR_OBJECTS = $(INPUT_COLLECTOR_SOURCES:.cpp=.o)
MASTER_SPLITTER_OBJECTS = $(MASTER_SPLITTER_SOURCES:.cpp=.o)
MERGE_MASTER_OBJECTS = $(MERGE_MASTER_SOURCES:.cpp=.o)
OUTPUT_DISPATCHER_OBJECTS = $(OUTPUT_DISPATCHER_SOURCES:.cpp=.o)
COORDINATOR_OBJECTS = $(COORDINATOR_SOURCES:.cpp=.o)
USAGE_OBJECTS = $(USAGE_SOURCES:.cpp=.o)

# All objects
ALL_OBJECTS = $(INPUT_COLLECTOR_OBJECTS) $(MASTER_SPLITTER_OBJECTS) $(MERGE_MASTER_OBJECTS) $(OUTPUT_DISPATCHER_OBJECTS) $(COORDINATOR_OBJECTS)

# Target executable
TARGET = video_ai_pipeline
TEST_TARGET = video_ai_test

# Default target
all: $(TARGET)

# Build main pipeline executable
$(TARGET): $(ALL_OBJECTS) $(USAGE_OBJECTS)
	@echo "🔗 Linking Video-AI Pipeline executable..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(ALL_OBJECTS) $(USAGE_OBJECTS) $(LIBS)
	@echo "✅ Video-AI Pipeline built successfully: $(TARGET)"

# Build test executable (without main usage)
test: $(ALL_OBJECTS)
	@echo "🔗 Linking Video-AI Test executable..."
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(ALL_OBJECTS) $(LIBS)
	@echo "✅ Video-AI Test built successfully: $(TEST_TARGET)"

# Compile source files
%.o: %.cpp
	@echo "🔧 Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f *.o
	rm -f $(TARGET)
	rm -f $(TEST_TARGET)
	rm -f *.log
	rm -f *.txt
	rm -rf Master_Chunks Merge_Output Terminal_Chunks
	@echo "✅ Clean completed"

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "📦 Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y libcurl4-openssl-dev build-essential
	@echo "✅ Dependencies installed"

# Run the pipeline
run: $(TARGET)
	@echo "🚀 Running Video-AI Pipeline..."
	./$(TARGET)

# Run with debug
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)
	@echo "🐛 Running Video-AI Pipeline in debug mode..."
	./$(TARGET)

# Check for required tools
check-tools:
	@echo "🔍 Checking required tools..."
	@which g++ > /dev/null || (echo "❌ g++ not found" && exit 1)
	@which curl-config > /dev/null || (echo "❌ libcurl-dev not found" && exit 1)
	@echo "✅ All required tools found"

# Create directories for pipeline
setup-dirs:
	@echo "📁 Creating pipeline directories..."
	mkdir -p Master_Chunks
	mkdir -p Merge_Output
	mkdir -p Terminal_Chunks
	mkdir -p processed_chunks
	mkdir -p merged_output
	@echo "✅ Directories created"

# Build individual components for testing
input-collector: Input_Collector/Input_Collector.o
	$(CXX) $(CXXFLAGS) -o input_collector_test Input_Collector/Input_Collector.o $(LIBS)

master-splitter: Input_Collector/Input_Collector.o Master_Splitter/Master_Splitter.o
	$(CXX) $(CXXFLAGS) -o master_splitter_test Input_Collector/Input_Collector.o Master_Splitter/Master_Splitter.o $(LIBS)

merge-master: Merge_Master/Merge_Master.o
	$(CXX) $(CXXFLAGS) -o merge_master_test Merge_Master/Merge_Master.o $(LIBS)

output-dispatcher: Output_Dispatcher/Output_Dispatcher.o
	$(CXX) $(CXXFLAGS) -o output_dispatcher_test Output_Dispatcher/Output_Dispatcher.o $(LIBS)

# Performance test
perf-test: CXXFLAGS += -O3 -DNDEBUG
perf-test: $(TARGET)
	@echo "⚡ Running performance test..."
	time ./$(TARGET)

# Memory check with valgrind
memcheck: $(TARGET)
	@echo "🔍 Running memory check..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Static analysis
analyze:
	@echo "🔍 Running static analysis..."
	cppcheck --enable=all --std=c++17 *.cpp *.hpp

# Generate documentation
docs:
	@echo "📚 Generating documentation..."
	doxygen Doxyfile 2>/dev/null || echo "Doxygen not configured"

# Package for distribution
package: clean
	@echo "📦 Packaging Video-AI Pipeline..."
	tar -czf video_ai_pipeline.tar.gz *.cpp *.hpp Makefile README.md
	@echo "✅ Package created: video_ai_pipeline.tar.gz"

# Help target
help:
	@echo "🎬 Video-AI Pipeline Makefile"
	@echo "=============================="
	@echo ""
	@echo "Available targets:"
	@echo "  all           - Build the complete pipeline"
	@echo "  test          - Build test executable"
	@echo "  clean         - Remove build artifacts"
	@echo "  install-deps  - Install required dependencies"
	@echo "  run           - Build and run the pipeline"
	@echo "  debug         - Build and run with debug symbols"
	@echo "  check-tools   - Verify required tools are installed"
	@echo "  setup-dirs    - Create required directories"
	@echo "  perf-test     - Run performance test"
	@echo "  memcheck      - Run memory leak check"
	@echo "  analyze       - Run static analysis"
	@echo "  docs          - Generate documentation"
	@echo "  package       - Create distribution package"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Component targets:"
	@echo "  input-collector    - Build Input_Collector test"
	@echo "  master-splitter   - Build Master_Splitter test"
	@echo "  merge-master      - Build Merge_Master test"
	@echo "  output-dispatcher - Build Output_Dispatcher test"

# Phony targets
.PHONY: all test clean install-deps run debug check-tools setup-dirs perf-test memcheck analyze docs package help
.PHONY: input-collector master-splitter merge-master output-dispatcher

# Dependencies with new folder structure
Input_Collector/Input_Collector.o: Input_Collector/Input_Collector.cpp Input_Collector/Input_Collector.hpp
Master_Splitter/Master_Splitter.o: Master_Splitter/Master_Splitter.cpp Master_Splitter/Master_Splitter.hpp Input_Collector/Input_Collector.hpp
Merge_Master/Merge_Master.o: Merge_Master/Merge_Master.cpp Merge_Master/Merge_Master.hpp
Output_Dispatcher/Output_Dispatcher.o: Output_Dispatcher/Output_Dispatcher.cpp Output_Dispatcher/Output_Dispatcher.hpp
Video_AI_Coordinator/Video_AI_Coordinator.o: Video_AI_Coordinator/Video_AI_Coordinator.cpp Video_AI_Coordinator/Video_AI_Coordinator.hpp Input_Collector/Input_Collector.hpp Master_Splitter/Master_Splitter.hpp Merge_Master/Merge_Master.hpp Output_Dispatcher/Output_Dispatcher.hpp
Video_AI_Coordinator/Video_AI_Pipeline_Usage.o: Video_AI_Coordinator/Video_AI_Pipeline_Usage.cpp Video_AI_Coordinator/Video_AI_Coordinator.hpp
