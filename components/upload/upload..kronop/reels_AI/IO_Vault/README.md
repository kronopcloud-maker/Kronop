# IO_Vault - Data Treasury Department

## Purpose
यह फोल्डर 'Shared' होगा। सारे AI इसी तिजोरी से वीडियो उठाएंगे और इसी में अपना साफ़ किया हुआ डेटा डालेंगे

## Directory Structure
```
IO_Vault/
├── Input/                    # Raw video input files
├── Output/                   # Final processed videos
├── temp/                     # Temporary processing files
├── Shared_Memory_Manager.hpp # Memory management header
├── Shared_Memory_Manager.cpp # Memory management implementation
└── README.md                 # This file
```

## Key Features

### Shared Memory System
- **4GB Total Memory Pool**: 64 slots × 64MB each
- **Multi-Queue System**: Separate queues for different data types
- **Automatic Cleanup**: Expired data removal every 5 minutes
- **Overflow Protection**: Memory pressure monitoring

### Data Types Supported
1. **VIDEO_CHUNK**: Raw video chunks (30 frames each)
2. **PROCESSED_TILE**: Enhanced 512×512 tiles
3. **AUDIO_SEGMENT**: Processed audio segments
4. **MERGED_FRAME**: Reassembled video frames
5. **FINAL_OUTPUT**: Complete processed videos

### Queue Management
- **Video Input Queue**: For Master_Splitter → AI_Workers
- **Processed Tile Queue**: For AI_Workers → Merge_Master
- **Audio Queue**: For Audio processing pipeline
- **Output Queue**: For Merge_Master → Output_Dispatcher

## Memory Allocation Strategy

### Slot Assignment
```
Priority 1: Video Chunks (High priority)
Priority 2: Processed Tiles (Medium priority)
Priority 3: Audio Segments (Low priority)
Priority 4: Output Files (Lowest priority)
```

### Cleanup Policy
- **30 Minutes**: Slot expiration time
- **1 Hour**: Temp file removal
- **90%**: Memory usage threshold for cleanup

## Integration Points

### Thermal_Guard Integration
```cpp
// Memory pressure triggers thermal alerts
if (!isMemoryHealthy()) {
    // Notify Thermal_Guard
    thermalMonitor.sendAlert(CRITICAL, temperature, "Memory pressure detected");
}
```

### Master_Control Integration
```cpp
// Brain Department coordinates data flow
sharedMemoryManager.enqueueVideoInput(chunk);
sharedMemoryManager.dequeueProcessedTile(tile);
```

### AI_Workers Integration
```cpp
// Workers access shared data
DataPacket packet;
sharedMemoryManager.dequeueVideoInput(packet);
// Process packet...
sharedMemoryManager.enqueueProcessedTile(processedPacket);
```

## Performance Metrics

### Memory Utilization
- **Normal**: < 70% (2.8GB)
- **Warning**: 70-90% (2.8-3.6GB)
- **Critical**: > 90% (> 3.6GB)

### Queue Sizes
- **Video Input**: Max 100 chunks
- **Processed Tiles**: Max 500 tiles
- **Audio Segments**: Max 50 segments
- **Output**: Max 20 files

## Safety Features

### Data Integrity
- **Checksum Validation**: File corruption detection
- **Backup Storage**: Dual storage (memory + file)
- **Atomic Operations**: Thread-safe queue operations

### Error Recovery
- **Automatic Retry**: Failed operations retry logic
- **Fallback Storage**: File system backup for memory overflow
- **Graceful Degradation**: Reduced performance under pressure

## Monitoring & Logging

### Log Files
- **IO_Vault_Log.txt**: Data operation logging
- **Memory_Manager_Log.txt**: Memory allocation tracking
- **Emergency_Log.txt**: Critical events recording

### Real-time Metrics
- Memory usage percentage
- Queue fill levels
- Processing throughput
- Error rates

## Usage Examples

### Storing Video Chunk
```cpp
DataPacket chunk;
chunk.packetId = "VID_123456";
chunk.type = VIDEO_CHUNK;
chunk.data = videoFrameData;
chunk.dataSize = videoFrameData.size();
chunk.priority = 1;

sharedMemoryManager.storeDataPacket(chunk);
sharedMemoryManager.enqueueVideoInput(chunk);
```

### Retrieving Processed Tile
```cpp
DataPacket tile;
if (sharedMemoryManager.dequeueProcessedTile(tile)) {
    // Process tile...
    std::cout << "Processed tile: " << tile.packetId << std::endl;
}
```

## Emergency Procedures

### Memory Overflow
1. Trigger immediate cleanup
2. Notify Thermal_Guard
3. Pause new data intake
4. Clear expired slots
5. Resume normal operation

### System Failure
1. Save all in-memory data to files
2. Close all queue operations
3. Log emergency state
4. Wait for system recovery

## Configuration

### Memory Limits
```cpp
const size_t MAX_MEMORY_USAGE = 4ULL * 1024 * 1024 * 1024; // 4GB
const size_t SLOT_SIZE = 64 * 1024 * 1024; // 64MB per slot
const int MAX_SLOTS = 64;
```

### Timing Parameters
```cpp
const auto SLOT_EXPIRATION = std::chrono::minutes(30);
const auto TEMP_FILE_CLEANUP = std::chrono::hours(1);
const auto CLEANUP_INTERVAL = std::chrono::minutes(1);
```

This IO_Vault serves as the central data hub for the entire Video-AI processing pipeline, ensuring efficient, safe, and reliable data flow between all departments.
