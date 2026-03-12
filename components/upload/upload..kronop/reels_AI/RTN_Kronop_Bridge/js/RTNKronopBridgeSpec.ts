/**
 * React Native Turbo Module Spec for Kronop Video AI
 * This file defines the JavaScript interface for the native C++ bridge
 */

import type { TurboModule } from 'react-native/Libraries/TurboModule/RCTExport';
import { TurboModuleRegistry } from 'react-native';

// Export types for TypeScript
export interface VideoProcessingResult {
  videoId: string;
  status: 'IDLE' | 'PROCESSING' | 'COMPLETED' | 'ERROR' | 'PAUSED';
  outputPath?: string;
  errorMessage?: string;
  progressPercentage: number;
  framesProcessed: number;
  totalFrames: number;
  startTime?: number;
  endTime?: number;
}

export interface ThermalStatus {
  temperature: number;
  status: string;
  pausedWorkers: number[];
  activeWorkers: number[];
  emergencyMode: boolean;
}

export interface WorkerStatus {
  workerId: number;
  state: string;
  currentChunkId: number;
  progress: number;
  isPaused: boolean;
  pauseReason?: string;
}

export interface SystemInfo {
  platform: string;
  coreComponents: string;
  maxConcurrentJobs: number;
  memoryUsage: number;
  thermalStatus: string;
}

// Define the Turbo Module interface
export interface Spec extends TurboModule {
  // Core video processing methods
  processVideo(videoPath: string): Promise<boolean>;
  processVideoAsync(videoPath: string, callback: (result: VideoProcessingResult) => void): Promise<string>;
  cancelProcessing(videoId: string): Promise<boolean>;
  getProcessingStatus(videoId: string): Promise<VideoProcessingResult>;

  // Thermal management methods
  getThermalStatus(): Promise<ThermalStatus>;
  setThermalThresholds(warning: number, critical: number, emergency: number): Promise<boolean>;
  getPausedWorkers(): Promise<number[]>;
  resumeWorker(workerId: number): Promise<boolean>;
  pauseWorker(workerId: number): Promise<boolean>;

  // Worker management methods
  getAllWorkerStatus(): Promise<WorkerStatus[]>;
  getActiveWorkerCount(): Promise<number>;
  setMaxWorkers(maxWorkers: number): Promise<boolean>;

  // Memory management methods
  getMemoryUsage(): Promise<number>;
  clearCache(): Promise<boolean>;
  optimizeMemory(): Promise<boolean>;

  // Configuration methods
  setServerUrl(serverUrl: string): Promise<boolean>;
  setProcessingTimeout(timeoutSeconds: number): Promise<boolean>;
  enableDebugMode(enable: boolean): Promise<boolean>;

  // System information methods
  getSystemInfo(): Promise<SystemInfo>;
  getSupportedFormats(): Promise<string[]>;

  // Initialization method
  initialize(inputDir: string, outputDir: string, maxJobs: number): Promise<boolean>;
}

// Register the Turbo Module
export default TurboModuleRegistry.get<Spec>('RTNKronopBridge') as Spec | null;
