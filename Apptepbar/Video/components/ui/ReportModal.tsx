// Report Modal Component - Report inappropriate content

import React, { useState } from 'react';
import { View, Text, StyleSheet, Modal, Pressable, ScrollView } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, typography } from '../../ThemeConstants';

interface ReportModalProps {
  visible: boolean;
  onClose: () => void;
  videoTitle: string;
}

interface ReportOption {
  id: string;
  title: string;
  description: string;
  icon: keyof typeof MaterialIcons.glyphMap;
}

const REPORT_OPTIONS: ReportOption[] = [
  {
    id: 'spam',
    title: 'Spam or misleading',
    description: 'Misleading text, tags, or thumbnails',
    icon: 'error-outline',
  },
  {
    id: 'harassment',
    title: 'Harassment or bullying',
    description: 'Abusive content targeting individuals',
    icon: 'people-outline',
  },
  {
    id: 'hateful',
    title: 'Hateful or abusive content',
    description: 'Promotes hatred or violence',
    icon: 'block',
  },
  {
    id: 'violence',
    title: 'Violent or graphic content',
    description: 'Disturbing imagery or violence',
    icon: 'warning',
  },
  {
    id: 'adult',
    title: 'Adult content',
    description: 'Sexually explicit material',
    icon: 'no-adult-content',
  },
  {
    id: 'copyright',
    title: 'Copyright infringement',
    description: 'Unauthorized use of copyrighted content',
    icon: 'copyright',
  },
];

export function ReportModal({ visible, onClose, videoTitle }: ReportModalProps) {
  const [selectedReason, setSelectedReason] = useState<string | null>(null);
  const [isSubmitted, setIsSubmitted] = useState(false);

  const handleSubmit = () => {
    if (selectedReason) {
      setIsSubmitted(true);
      setTimeout(() => {
        setIsSubmitted(false);
        setSelectedReason(null);
        onClose();
      }, 2000);
    }
  };

  const handleClose = () => {
    setSelectedReason(null);
    setIsSubmitted(false);
    onClose();
  };

  if (isSubmitted) {
    return (
      <Modal
        visible={visible}
        transparent
        animationType="fade"
        onRequestClose={handleClose}
      >
        <View style={styles.overlay}>
          <View style={styles.successContainer}>
            <View style={styles.successIcon}>
              <MaterialIcons name="check-circle" size={64} color={colors.primary} />
            </View>
            <Text style={styles.successTitle}>Report Submitted</Text>
            <Text style={styles.successText}>Thank you for reporting. We will review this content.</Text>
          </View>
        </View>
      </Modal>
    );
  }

  return (
    <Modal
      visible={visible}
      animationType="slide"
      onRequestClose={handleClose}
    >
      <View style={styles.container}>
        <View style={styles.header}>
          <Pressable onPress={handleClose} style={styles.closeButton}>
            <MaterialIcons name="close" size={24} color={colors.text} />
          </Pressable>
          <Text style={styles.headerTitle}>Report</Text>
          <View style={{ width: 40 }} />
        </View>

        <ScrollView style={styles.content} contentContainerStyle={styles.contentContainer}>
          <View style={styles.infoBox}>
            <MaterialIcons name="info-outline" size={20} color={colors.primary} />
            <Text style={styles.infoText}>
              Help us understand the problem. What is wrong with this video?
            </Text>
          </View>

          <Text style={styles.videoTitle} numberOfLines={2}>{videoTitle}</Text>

          <View style={styles.optionsList}>
            {REPORT_OPTIONS.map((option) => (
              <Pressable
                key={option.id}
                style={[
                  styles.optionItem,
                  selectedReason === option.id && styles.selectedOption
                ]}
                onPress={() => setSelectedReason(option.id)}
              >
                <MaterialIcons 
                  name={option.icon} 
                  size={24} 
                  color={selectedReason === option.id ? colors.primary : colors.textMuted} 
                />
                <View style={styles.optionContent}>
                  <Text style={[
                    styles.optionTitle,
                    selectedReason === option.id && styles.selectedText
                  ]}>
                    {option.title}
                  </Text>
                  <Text style={styles.optionDescription}>{option.description}</Text>
                </View>
                {selectedReason === option.id && (
                  <MaterialIcons name="check-circle" size={24} color={colors.primary} />
                )}
              </Pressable>
            ))}
          </View>
        </ScrollView>

        <View style={styles.footer}>
          <Pressable 
            style={[styles.submitButton, !selectedReason && styles.submitButtonDisabled]}
            onPress={handleSubmit}
            disabled={!selectedReason}
          >
            <Text style={[styles.submitButtonText, !selectedReason && styles.submitButtonTextDisabled]}>
              Submit Report
            </Text>
          </Pressable>
        </View>
      </View>
    </Modal>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  overlay: {
    flex: 1,
    backgroundColor: 'rgba(0, 0, 0, 0.8)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingTop: spacing.xl,
    paddingHorizontal: spacing.md,
    paddingBottom: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.surfaceLight,
  },
  closeButton: {
    padding: spacing.xs,
  },
  headerTitle: {
    ...typography.h3,
    color: colors.text,
    fontWeight: '700',
  },
  content: {
    flex: 1,
  },
  contentContainer: {
    padding: spacing.md,
  },
  infoBox: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    gap: spacing.sm,
    padding: spacing.md,
    backgroundColor: colors.surface,
    borderRadius: 8,
    marginBottom: spacing.lg,
  },
  infoText: {
    flex: 1,
    fontSize: 13,
    color: colors.text,
    lineHeight: 18,
  },
  videoTitle: {
    fontSize: 14,
    color: colors.textSubtle,
    marginBottom: spacing.lg,
    fontStyle: 'italic',
  },
  optionsList: {
    gap: spacing.sm,
  },
  optionItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.md,
    padding: spacing.md,
    backgroundColor: colors.surface,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: 'transparent',
  },
  selectedOption: {
    borderColor: colors.primary,
    backgroundColor: colors.surfaceLight,
  },
  optionContent: {
    flex: 1,
  },
  optionTitle: {
    fontSize: 15,
    color: colors.text,
    fontWeight: '600',
    marginBottom: 2,
  },
  selectedText: {
    color: colors.primary,
  },
  optionDescription: {
    fontSize: 12,
    color: colors.textMuted,
    lineHeight: 16,
  },
  footer: {
    padding: spacing.md,
    borderTopWidth: 1,
    borderTopColor: colors.surfaceLight,
  },
  submitButton: {
    backgroundColor: colors.primary,
    paddingVertical: spacing.md,
    borderRadius: 8,
    alignItems: 'center',
  },
  submitButtonDisabled: {
    backgroundColor: colors.surface,
  },
  submitButtonText: {
    fontSize: 16,
    color: colors.text,
    fontWeight: '700',
  },
  submitButtonTextDisabled: {
    color: colors.textMuted,
  },
  successContainer: {
    backgroundColor: colors.surface,
    borderRadius: 16,
    padding: spacing.xl,
    alignItems: 'center',
    maxWidth: '80%',
  },
  successIcon: {
    marginBottom: spacing.lg,
  },
  successTitle: {
    ...typography.h3,
    color: colors.text,
    fontWeight: '700',
    marginBottom: spacing.sm,
  },
  successText: {
    fontSize: 14,
    color: colors.textSubtle,
    textAlign: 'center',
    lineHeight: 20,
  },
});
