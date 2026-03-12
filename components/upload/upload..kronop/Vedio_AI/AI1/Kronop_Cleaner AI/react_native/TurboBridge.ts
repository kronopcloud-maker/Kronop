import { NativeModules } from 'react-native';

const { KronopNativeInterface } = NativeModules;

interface TurboBridgeOptions {
  width: number;
  height: number;
  enableVulkan?: boolean;
  turboMode?: boolean;
}

interface ProcessingResult {
  success: boolean;
  outputData?: string;
  processingTime?: number;
  fps?: number;
  mode?: string;
  error?: string;
}

class TurboBridge {
  private isInitialized: boolean = false;
  private options: TurboBridgeOptions | null = null;

  /**
   * Initialize the Turbo Bridge with video parameters
   * Automatically checks Vulkan support and falls back to CPU if not available
   */
  async initialize(options: TurboBridgeOptions): Promise<boolean> {
    try {
      this.options = options;
      
      // If enableVulkan not specified, default to true to attempt GPU acceleration
      const attemptVulkan = options.enableVulkan !== false;
      
      const result = await KronopNativeInterface.initialize(
        options.width,
        options.height,
        attemptVulkan
      );

      if (result.success) {
        this.isInitialized = true;
        
        // Check if Vulkan is actually enabled
        const stats = await this.getStatistics();
        if (stats && !stats.vulkanEnabled && attemptVulkan) {
          console.log('Vulkan not supported on this device, automatically using CPU mode');
        } else if (stats && stats.vulkanEnabled) {
          console.log('Vulkan GPU acceleration enabled');
        }
        
        return true;
      } else {
        // If initialization failed with Vulkan attempt, try with CPU
        if (attemptVulkan) {
          console.log('Vulkan initialization failed, falling back to CPU mode');
          const cpuResult = await KronopNativeInterface.initialize(
            options.width,
            options.height,
            false // Force CPU mode
          );
          
          if (cpuResult.success) {
            this.isInitialized = true;
            console.log('CPU mode initialized successfully');
            return true;
          }
        }
        
        console.warn('TurboBridge initialization failed:', result.message);
        return false;
      }
    } catch (error) {
      // If Vulkan attempt fails, try CPU
      if (options.enableVulkan !== false) {
        console.log('Vulkan initialization error, falling back to CPU mode');
        try {
          const cpuResult = await KronopNativeInterface.initialize(
            options.width,
            options.height,
            false
          );
          
          if (cpuResult.success) {
            this.isInitialized = true;
            console.log('CPU mode initialized successfully after Vulkan error');
            return true;
          }
        } catch (cpuError) {
          console.error('Both Vulkan and CPU initialization failed:', cpuError);
        }
      }
      
      console.error('TurboBridge initialization error:', error);
      return false;
    }
  }

  /**
   * Set performance mode
   */
  async setPerformanceMode(mode: number): Promise<boolean> {
    if (!this.isInitialized) {
      return false;
    }

    try {
      const result = await KronopNativeInterface.setPerformanceMode(mode);
      return result.success;
    } catch (error) {
      console.warn('Failed to set performance mode:', error);
      return false;
    }
  }

  /**
   * Get statistics
   */
  async getStatistics(): Promise<any> {
    if (!this.isInitialized) {
      return null;
    }

    try {
      return await KronopNativeInterface.getStatistics();
    } catch (error) {
      console.warn('Failed to get statistics:', error);
      return null;
    }
  }

  /**
   * Shutdown the bridge
   */
  async shutdown(): Promise<boolean> {
    if (!this.isInitialized) {
      return true;
    }

    try {
      const result = await KronopNativeInterface.shutdown();
      this.isInitialized = false;
      return result.success;
    } catch (error) {
      console.error('TurboBridge shutdown error:', error);
      this.isInitialized = false;
      return false;
    }
  }
}

export default new TurboBridge();
