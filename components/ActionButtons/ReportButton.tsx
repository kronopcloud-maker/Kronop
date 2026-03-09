import React from 'react';
import { Pressable, Text, StyleSheet } from 'react-native';
import { Ionicons } from '@expo/vector-icons';

interface ReportButtonProps {
  onPress: () => void;
}

export function ReportButton({ onPress }: ReportButtonProps) {
  return (
    <Pressable style={styles.actionButton} onPress={onPress}>
      <Ionicons name="flag-outline" size={26} color="#FFF" />
      <Text style={styles.actionText}>Report</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  actionButton: {
    alignItems: 'center',
    gap: 4,
  },
  actionText: {
    color: '#FFF',
    fontSize: 12,
    fontWeight: '600',
  },
});
