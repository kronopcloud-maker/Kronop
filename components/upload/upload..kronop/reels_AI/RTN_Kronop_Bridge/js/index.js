/**
 * React Native Bridge for Kronop Video AI
 * Cross-platform C++ core for Android & iOS
 */

import { NativeModules, Platform } from 'react-native';
import RTNKronopBridgeSpec from './RTNKronopBridgeSpec';

const { RTNKronopBridge } = NativeModules;

class RTNKronopBridgeManager {
  constructor() {
    this.isInitialized = false;
    this.currentProcessingVideo = null;
    this.progressCallbacks = new Map();
    this.thermalCallbacks = new Set();
    this.workerCallbacks = new Set();
    
    // Platform-specific initialization
    this.initializePlatform();
  }

  async initialize(config = {}) {
    try {
      if (this.isInitialized) {
        console.log('RTNKronopBridge already initialized');
        return true;
      }

      const defaultConfig = {
        inputDirectory: Platform.OS === 'android' ? '/sdcard/Kronop/Input' : `${RNFS.DocumentDirectoryPath}/Kronop/Input`,
        outputDirectory: Platform.OS === 'android' ? '/sdcard/Kronop/Output' : `${RNFS.DocumentDirectoryPath}/Kronop/Output`,
        maxConcurrentJobs: 3,
        thermalThresholds: {
          warning: 35.0,
          critical: 40.0,
          emergency: 45.0
        },
        enableDebugMode: false
      };

      const finalConfig = { ...defaultConfig, ...config };

      // Initialize native bridge
      const success = await RTNKronopBridge.initialize(
        finalConfig.inputDirectory,
        finalConfig.outputDirectory,
        finalConfig.maxConcurrentJobs
      );

      if (success) {
        // Set thermal thresholds
        await RTNKronopBridge.setThermalThresholds(
          finalConfig.thermalThresholds.warning,
          finalConfig.thermalThresholds.critical,
          finalConfig.thermalThresholds.emergency
        );

        // Enable debug mode if requested
        if (finalConfig.enableDebugMode) {
          await RTNKronopBridge.enableDebugMode(true);
        }

        // Start monitoring
        this.startMonitoring();

        this.isInitialized = true;
        console.log('RTNKronopBridge initialized successfully');
        return true;
      } else {
        console.error('Failed to initialize RTNKronopBridge');
        return false;
      }
    } catch (error) {
      console.error('RTNKronopBridge initialization error:', error);
      return false;
    }
  }

  async initializePlatform() {
    if (Platform.OS === 'android') {
      // Android-specific initialization
      try {
        const hasPermission = await PermissionsAndroid.check(PermissionsAndroid.WRITE_EXTERNAL_STORAGE);
        if (!hasPermission) {
          const granted = await PermissionsAndroid.request(PermissionsAndroid.WRITE_EXTERNAL_STORAGE);
          if (granted !== PermissionsAndroid.RESULTS.GRANTED) {
            console.warn('Storage permission not granted');
          }
        }
      } catch (error) {
        console.warn('Android permission check failed:', error);
      }
    } else if (Platform.OS === 'ios') {
      // iOS-specific initialization
      // iOS thermal monitoring is handled by native code
      console.log('iOS thermal monitoring enabled');
    }
  }

  async processVideo(videoPath, options = {}) {
    try {
      if (!this.isInitialized) {
        throw new Error('RTNKronopBridge not initialized');
      }

      const defaultOptions = {
        priority: 'normal',
        timeout: 300, // 5 minutes
        callback: null
      };

      const finalOptions = { ...defaultOptions, ...options };

      // Validate video path
      if (!videoPath || typeof videoPath !== 'string') {
        throw new Error('Invalid video path');
      }

      console.log(`Processing video: ${videoPath}`);

      // Start processing
      if (finalOptions.callback) {
        // Async processing with callback
        const videoId = await RTNKronopBridge.processVideoAsync(videoPath, (result) => {
          finalOptions.callback(result);
        });
        
        this.currentProcessingVideo = videoId;
        return videoId;
      } else {
        // Synchronous processing
        const success = await RTNKronopBridge.processVideo(videoPath);
        
        if (success) {
          const videoId = this.generateVideoId(videoPath);
          this.currentProcessingVideo = videoId;
          return videoId;
        } else {
          throw new Error('Failed to start video processing');
        }
      }
    } catch (error) {
      console.error('Video processing error:', error);
      throw error;
    }
  }

  async cancelProcessing(videoId) {
    try {
      if (!videoId) {
        videoId = this.currentProcessingVideo;
      }

      if (!videoId) {
        console.warn('No video to cancel');
        return false;
      }

      console.log(`Canceling processing for video: ${videoId}`);
      const success = await RTNKronopBridge.cancelProcessing(videoId);

      if (success) {
        if (videoId === this.currentProcessingVideo) {
          this.currentProcessingVideo = null;
        }
        console.log('Video processing canceled successfully');
      } else {
        console.warn('Failed to cancel video processing');
      }

      return success;
    } catch (error) {
      console.error('Cancel processing error:', error);
      return false;
    }
  }

  async getProcessingStatus(videoId) {
    try {
      if (!videoId) {
        videoId = this.currentProcessingVideo;
      }

      if (!videoId) {
        return null;
      }

      const status = await RTNKronopBridge.getProcessingStatus(videoId);
      return status;
    } catch (error) {
      console.error('Get processing status error:', error);
      return null;
    }
  }

  async getThermalStatus() {
    try {
      const thermalStatus = await RTNKronopBridge.getThermalStatus();
      
      // Notify thermal callbacks
      this.thermalCallbacks.forEach(callback => {
        try {
          callback(thermalStatus);
        } catch (error) {
          console.error('Thermal callback error:', error);
        }
      });

      return thermalStatus;
    } catch (error) {
      console.error('Get thermal status error:', error);
      return null;
    }
  }

  async getWorkerStatus() {
    try {
      const workerStatus = await RTNKronopBridge.getWorkerStatus();
      
      // Notify worker callbacks
      this.workerCallbacks.forEach(callback => {
        try {
          callback(workerStatus);
        } catch (error) {
          console.error('Worker callback error:', error);
        }
      });

      return workerStatus;
    } catch (error) {
      console.error('Get worker status error:', error);
      return null;
    }
  }

  async pauseWorker(workerId) {
    try {
      const success = await RTNKronopBridge.pauseWorker(workerId);
      
      if (success) {
        console.log(`Worker ${workerId} paused`);
      } else {
        console.warn(`Failed to pause worker ${workerId}`);
      }

      return success;
    } catch (error) {
      console.error('Pause worker error:', error);
      return false;
    }
  }

  async resumeWorker(workerId) {
    try {
      const success = await RTNKronopBridge.resumeWorker(workerId);
      
      if (success) {
        console.log(`Worker ${workerId} resumed`);
      } else {
        console.warn(`Failed to resume worker ${workerId}`);
      }

      return success;
    } catch (error) {
      console.error('Resume worker error:', error);
      return false;
    }
  }

  async getPausedWorkers() {
    try {
      const pausedWorkers = await RTNKronopBridge.getPausedWorkers();
      return pausedWorkers;
    } catch (error) {
      console.error('Get paused workers error:', error);
      return [];
    }
  }

  async getActiveWorkerCount() {
    try {
      const count = await RTNKronopBridge.getActiveWorkerCount();
      return count;
    } catch (error) {
      console.error('Get active worker count error:', error);
      return 0;
    }
  }

  async setMaxWorkers(maxWorkers) {
    try {
      const success = await RTNKronopBridge.setMaxWorkers(maxWorkers);
      
      if (success) {
        console.log(`Max workers set to ${maxWorkers}`);
      } else {
        console.warn(`Failed to set max workers to ${maxWorkers}`);
      }

      return success;
    } catch (error) {
      console.error('Set max workers error:', error);
      return false;
    }
  }

  async getMemoryUsage() {
    try {
      const usage = await RTNKronopBridge.getMemoryUsage();
      return usage;
    } catch (error) {
      console.error('Get memory usage error:', error);
      return 0;
    }
  }

  async clearCache() {
    try {
      const success = await RTNKronopBridge.clearCache();
      
      if (success) {
        console.log('Cache cleared successfully');
      } else {
        console.warn('Failed to clear cache');
      }

      return success;
    } catch (error) {
      console.error('Clear cache error:', error);
      return false;
    }
  }

  async optimizeMemory() {
    try {
      const success = await RTNKronopBridge.optimizeMemory();
      
      if (success) {
        console.log('Memory optimization completed');
      } else {
        console.warn('Failed to optimize memory');
      }

      return success;
    } catch (error) {
      console.error('Optimize memory error:', error);
      return false;
    }
  }

  async setServerUrl(serverUrl) {
    try {
      const success = await RTNKronopBridge.setServerUrl(serverUrl);
      
      if (success) {
        console.log(`Server URL set to: ${serverUrl}`);
      } else {
        console.warn(`Failed to set server URL to: ${serverUrl}`);
      }

      return success;
    } catch (error) {
      console.error('Set server URL error:', error);
      return false;
    }
  }

  async setProcessingTimeout(timeoutSeconds) {
    try {
      const success = await RTNKronopBridge.setProcessingTimeout(timeoutSeconds);
      
      if (success) {
        console.log(`Processing timeout set to ${timeoutSeconds} seconds`);
      } else {
        console.warn(`Failed to set processing timeout to ${timeoutSeconds} seconds`);
      }

      return success;
    } catch (error) {
      console.error('Set processing timeout error:', error);
      return false;
    }
  }

  async enableDebugMode(enable) {
    try {
      const success = await RTNKronopBridge.enableDebugMode(enable);
      
      if (success) {
        console.log(`Debug mode ${enable ? 'enabled' : 'disabled'}`);
      } else {
        console.warn(`Failed to ${enable ? 'enable' : 'disable'} debug mode`);
      }

      return success;
    } catch (error) {
      console.error('Enable debug mode error:', error);
      return false;
    }
  }

  async getSystemInfo() {
    try {
      const info = await RTNKronopBridge.getSystemInfo();
      return info;
    } catch (error) {
      console.error('Get system info error:', error);
      return null;
    }
  }

  async getSupportedFormats() {
    try {
      const formats = await RTNKronopBridge.getSupportedFormats();
      return formats;
    } catch (error) {
      console.error('Get supported formats error:', error);
      return [];
    }
  }

  // Callback management
  onProgress(videoId, callback) {
    this.progressCallbacks.set(videoId, callback);
  }

  offProgress(videoId) {
    this.progressCallbacks.delete(videoId);
  }

  onThermal(callback) {
    this.thermalCallbacks.add(callback);
  }

  offThermal(callback) {
    this.thermalCallbacks.delete(callback);
  }

  onWorker(callback) {
    this.workerCallbacks.add(callback);
  }

  offWorker(callback) {
    this.workerCallbacks.delete(callback);
  }

  // Utility methods
  generateVideoId(videoPath) {
    const timestamp = Date.now();
    const filename = videoPath.split('/').pop().split('.')[0];
    return `${filename}_${timestamp}`;
  }

  startMonitoring() {
    // Start background monitoring for thermal and worker status
    const monitoringInterval = setInterval(async () => {
      try {
        await this.getThermalStatus();
        await this.getWorkerStatus();
      } catch (error) {
        console.error('Monitoring error:', error);
      }
    }, 1000); // Monitor every second

    // Store interval for cleanup
    this.monitoringInterval = monitoringInterval;
  }

  stopMonitoring() {
    if (this.monitoringInterval) {
      clearInterval(this.monitoringInterval);
      this.monitoringInterval = null;
    }
  }

  // Cleanup
  cleanup() {
    this.stopMonitoring();
    this.progressCallbacks.clear();
    this.thermalCallbacks.clear();
    this.workerCallbacks.clear();
    this.currentProcessingVideo = null;
    this.isInitialized = false;
  }

  // Status methods
  isReady() {
    return this.isInitialized;
  }

  isProcessing() {
    return this.currentProcessingVideo !== null;
  }

  getCurrentVideoId() {
    return this.currentProcessingVideo;
  }
}

// Create singleton instance
const rtnKronopBridge = new RTNKronopBridgeManager();

// Export the manager and native module
export default rtnKronopBridge;
export { RTNKronopBridge };

// Export types for TypeScript
export const ProcessingStatus = {
  IDLE: 0,
  PROCESSING: 1,
  COMPLETED: 2,
  ERROR: 3,
  PAUSED: 4
};

export const ThermalState = {
  NORMAL: 'NORMAL',
  WARNING: 'WARNING',
  CRITICAL: 'CRITICAL',
  EMERGENCY: 'EMERGENCY'
};
