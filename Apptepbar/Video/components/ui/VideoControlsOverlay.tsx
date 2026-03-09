// Video Controls Overlay - Play/Pause and Previous/Next video controls

import React from 'react';
import { View, StyleSheet, Pressable } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing } from '../../ThemeConstants';

interface VideoControlsOverlayProps {
  isPlaying: boolean;
  onPlayPause: () => void;
  onPrevious: () => void;
  onNext: () => void;
  hasPrevious: boolean;
  hasNext: boolean;
  visible: boolean;
}

export function VideoControlsOverlay({ 
  isPlaying, 
  onPlayPause, 
  onPrevious, 
  onNext,
  hasPrevious,
  hasNext,
  visible 
}: VideoControlsOverlayProps) {
  if (!visible) return null;
  
  return (
    <View style={styles.overlay}>
      <View style={styles.controlsContainer}>
        <Pressable 
          style={[styles.controlButton, !hasPrevious && styles.controlButtonDisabled]}
          onPress={onPrevious}
          disabled={!hasPrevious}
        >
          <MaterialIcons 
            name="skip-previous" 
            size={28} 
            color={hasPrevious ? colors.text : colors.textMuted} 
          />
        </Pressable>

        <Pressable 
          style={[styles.controlButton, styles.playPauseButton]}
          onPress={onPlayPause}
        >
          <MaterialIcons 
            name={isPlaying ? 'pause' : 'play-arrow'} 
            size={32} 
            color={colors.text} 
          />
        </Pressable>

        <Pressable 
          style={[styles.controlButton, !hasNext && styles.controlButtonDisabled]}
          onPress={onNext}
          disabled={!hasNext}
        >
          <MaterialIcons 
            name="skip-next" 
            size={28} 
            color={hasNext ? colors.text : colors.textMuted} 
          />
        </Pressable>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  overlay: {
    ...StyleSheet.absoluteFillObject,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: 'transparent',
  },
  controlsContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.xl * 2,
  },
  controlButton: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: 'rgba(0, 0, 0, 0.6)',
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1.5,
    borderColor: 'rgba(255, 255, 255, 0.25)',
  },
  playPauseButton: {
    width: 56,
    height: 56,
    borderRadius: 28,
    backgroundColor: 'rgba(0, 0, 0, 0.65)',
    borderWidth: 2,
    borderColor: 'rgba(255, 255, 255, 0.3)',
  },
  controlButtonDisabled: {
    opacity: 0.3,
  },
});
