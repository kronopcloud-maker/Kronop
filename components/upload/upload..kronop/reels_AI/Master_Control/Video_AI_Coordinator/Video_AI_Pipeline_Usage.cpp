/**
 * Video_AI_Pipeline_Usage.cpp
 * 
 * Complete usage example for the Video-AI Processing Pipeline
 * Demonstrates: Input_Collector -> Master_Splitter -> Merge_Master -> Output_Dispatcher
 */

#include "Video_AI_Coordinator.hpp"
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    std::cout << "🎬 Video-AI Processing Pipeline Demo" << std::endl;
    std::cout << "====================================" << std::endl;

    // Create and initialize the coordinator
    Video_AI_Coordinator coordinator;
    
    if (!coordinator.initializeSystem()) {
        std::cerr << "❌ Failed to initialize Video-AI system" << std::endl;
        return -1;
    }
    
    // Configure system
    coordinator.setServerUrl("https://api.kronop.com/videos/upload");
    coordinator.setProcessingTimeout(300); // 5 minutes
    coordinator.setMaxConcurrentJobs(3);
    
    // Start the system
    coordinator.startSystem();
    
    std::cout << "\n🚀 Video-AI Processing Pipeline Started!" << std::endl;
    std::cout << "Pipeline Flow: Input_Collector -> Master_Splitter -> Merge_Master -> Output_Dispatcher" << std::endl;
    
    // Example 1: Process single video
    std::cout << "\n📹 Example 1: Processing single video..." << std::endl;
    bool success1 = coordinator.processVideo("sample_video.mp4", "demo_video_001");
    if (success1) {
        std::cout << "✅ Single video processing initiated" << std::endl;
    } else {
        std::cout << "❌ Failed to start single video processing" << std::endl;
    }
    
    // Example 2: Process video sequence
    std::cout << "\n📹 Example 2: Processing video sequence..." << std::endl;
    std::vector<std::string> videoSequence = {
        "video_part1.mp4",
        "video_part2.mp4", 
        "video_part3.mp4"
    };
    
    bool success2 = coordinator.processVideoSequence(videoSequence);
    if (success2) {
        std::cout << "✅ Video sequence processing initiated" << std::endl;
    } else {
        std::cout << "❌ Failed to start video sequence processing" << std::endl;
    }
    
    // Monitor system status
    std::cout << "\n📊 Monitoring System Status..." << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        std::string status;
        coordinator.getSystemStatus(status);
        
        std::cout << "\n--- Status Update " << (i + 1) << " ---" << std::endl;
        std::cout << status << std::endl;
        
        // Show job statistics
        std::cout << "📈 Jobs: Active=" << coordinator.getActiveJobs()
                  << ", Completed=" << coordinator.getCompletedJobs()
                  << ", Failed=" << coordinator.getFailedJobs() << std::endl;
        
        // Wait before next status check
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    // Simulate AI processing callbacks (in real system, these would be called by AI modules)
    std::cout << "\n🤖 Simulating AI Processing Callbacks..." << std::endl;
    
    // Simulate chunk processing completion for demo_video_001
    for (int chunk = 1; chunk <= 5; ++chunk) {
        std::string processedPath = "processed_chunks/demo_video_001_chunk_" + std::to_string(chunk) + "_processed.mp4";
        coordinator.notifyChunkProcessed("demo_video_001", chunk, processedPath);
        std::cout << "📤 Notified: Chunk " << chunk << " processed for demo_video_001" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Simulate video merge completion
    std::string mergedPath = "merged_output/demo_video_001_merged.mp4";
    coordinator.notifyVideoMerged("demo_video_001", mergedPath);
    std::cout << "🔗 Notified: Video merged for demo_video_001" << std::endl;
    
    // Simulate dispatch completion
    coordinator.notifyVideoDispatched("demo_video_001", true);
    std::cout << "📤 Notified: Video dispatched for demo_video_001" << std::endl;
    
    // Show final job history
    std::cout << "\n📋 Final Job History:" << std::endl;
    auto jobHistory = coordinator.getJobHistory();
    
    for (const auto& job : jobHistory) {
        std::cout << "🎬 Job: " << job.videoId 
                  << " | Seq: " << job.sequenceIndex
                  << " | Status: ";
        
        switch (job.currentStatus) {
            case IDLE: std::cout << "IDLE"; break;
            case COLLECTING: std::cout << "COLLECTING"; break;
            case SPLITTING: std::cout << "SPLITTING"; break;
            case PROCESSING: std::cout << "PROCESSING"; break;
            case MERGING: std::cout << "MERGING"; break;
            case DISPATCHING: std::cout << "DISPATCHING"; break;
            case COMPLETED: std::cout << "✅ COMPLETED"; break;
            case ERROR: std::cout << "❌ ERROR"; break;
        }
        
        if (!job.errorMessage.empty()) {
            std::cout << " | Error: " << job.errorMessage;
        }
        
        std::cout << std::endl;
    }
    
    // Final status
    std::string finalStatus;
    coordinator.getSystemStatus(finalStatus);
    std::cout << "\n🏁 Final System Status:" << std::endl;
    std::cout << finalStatus << std::endl;
    
    // Stop the system
    std::cout << "\n🛑 Stopping Video-AI Processing Pipeline..." << std::endl;
    coordinator.stopSystem();
    
    std::cout << "\n✅ Video-AI Processing Pipeline Demo Completed!" << std::endl;
    std::cout << "\n📋 Key Features Demonstrated:" << std::endl;
    std::cout << "  ✅ Input_Collector: Video reception and queuing" << std::endl;
    std::cout << "  ✅ Master_Splitter: 5-chunk splitting with AI distribution" << std::endl;
    std::cout << "  ✅ Merge_Master: Sequential chunk merging (1,2,3,4,5)" << std::endl;
    std::cout << "  ✅ Output_Dispatcher: Server upload with retry logic" << std::endl;
    std::cout << "  ✅ Video_AI_Coordinator: Complete pipeline management" << std::endl;
    std::cout << "  ✅ Sequence-based processing to prevent video mixing" << std::endl;
    std::cout << "  ✅ Real-time monitoring and job tracking" << std::endl;
    std::cout << "  ✅ Error handling and recovery mechanisms" << std::endl;
    
    return 0;
}

/**
 * Usage Instructions:
 * 
 * 1. Initialize the system:
 *    Video_AI_Coordinator coordinator;
 *    coordinator.initializeSystem();
 *    coordinator.startSystem();
 * 
 * 2. Process videos:
 *    coordinator.processVideo("input.mp4", "video_id_001");
 *    coordinator.processVideoSequence(video_list);
 * 
 * 3. Monitor progress:
 *    coordinator.getSystemStatus(status);
 *    coordinator.getActiveJobs();
 *    coordinator.getJobHistory();
 * 
 * 4. AI Integration (called by AI modules):
 *    coordinator.notifyChunkProcessed(videoId, chunkIndex, processedPath);
 *    coordinator.notifyVideoMerged(videoId, mergedPath);
 *    coordinator.notifyVideoDispatched(videoId, success);
 * 
 * 5. Configuration:
 *    coordinator.setServerUrl("https://your-server.com/upload");
 *    coordinator.setMaxConcurrentJobs(5);
 *    coordinator.setProcessingTimeout(300);
 * 
 * Pipeline Flow:
 * Input_Collector -> Master_Splitter -> AI1-AI5 -> Merge_Master -> Output_Dispatcher
 * 
 * Sequence Management:
 * - Each video gets unique sequence index
 * - Chunks are processed in order (1,2,3,4,5)
 * - Merging respects sequence to prevent video mixing
 * - Dispatch maintains order for proper delivery
 */
