// Comments Modal Component - View and add comments

import React, { useState } from 'react';
import { View, Text, StyleSheet, Modal, Pressable, TextInput, FlatList, KeyboardAvoidingView, Platform } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import { Image } from 'expo-image';
import { colors, spacing, typography } from '../../ThemeConstants';

interface Comment {
  id: string;
  user: {
    name: string;
    avatar: string;
  };
  text: string;
  time: string;
  likes: number;
}

interface CommentsModalProps {
  visible: boolean;
  onClose: () => void;
  videoTitle: string;
  commentCount: number;
}

const MOCK_COMMENTS: Comment[] = [
  {
    id: '1',
    user: {
      name: 'Sarah Johnson',
      avatar: 'https://i.pravatar.cc/150?img=1',
    },
    text: 'This is amazing! Thanks for sharing.',
    time: '2 hours ago',
    likes: 45,
  },
  {
    id: '2',
    user: {
      name: 'Mike Chen',
      avatar: 'https://i.pravatar.cc/150?img=2',
    },
    text: 'Great content, keep it up!',
    time: '5 hours ago',
    likes: 23,
  },
  {
    id: '3',
    user: {
      name: 'Emma Wilson',
      avatar: 'https://i.pravatar.cc/150?img=3',
    },
    text: 'Very informative and well explained.',
    time: '1 day ago',
    likes: 67,
  },
];

export function CommentsModal({ visible, onClose, videoTitle, commentCount }: CommentsModalProps) {
  const [comments, setComments] = useState<Comment[]>(MOCK_COMMENTS);
  const [newComment, setNewComment] = useState('');

  const handleAddComment = () => {
    if (newComment.trim()) {
      const comment: Comment = {
        id: Date.now().toString(),
        user: {
          name: 'You',
          avatar: 'https://i.pravatar.cc/150?img=50',
        },
        text: newComment.trim(),
        time: 'Just now',
        likes: 0,
      };
      setComments([comment, ...comments]);
      setNewComment('');
    }
  };

  const renderComment = ({ item }: { item: Comment }) => (
    <View style={styles.commentItem}>
      <Image 
        source={{ uri: item.user.avatar }}
        style={styles.commentAvatar}
        contentFit="cover"
      />
      <View style={styles.commentContent}>
        <View style={styles.commentHeader}>
          <Text style={styles.commentUserName}>{item.user.name}</Text>
          <Text style={styles.commentTime}>{item.time}</Text>
        </View>
        <Text style={styles.commentText}>{item.text}</Text>
        <View style={styles.commentActions}>
          <Pressable style={styles.commentAction}>
            <MaterialIcons name="thumb-up-off-alt" size={16} color={colors.textMuted} />
            <Text style={styles.commentActionText}>{item.likes}</Text>
          </Pressable>
          <Pressable style={styles.commentAction}>
            <MaterialIcons name="reply" size={16} color={colors.textMuted} />
            <Text style={styles.commentActionText}>Reply</Text>
          </Pressable>
        </View>
      </View>
    </View>
  );

  return (
    <Modal
      visible={visible}
      animationType="slide"
      onRequestClose={onClose}
    >
      <KeyboardAvoidingView 
        style={styles.container}
        behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
      >
        <View style={styles.header}>
          <View style={styles.headerTop}>
            <Pressable onPress={onClose} style={styles.closeButton}>
              <MaterialIcons name="close" size={24} color={colors.text} />
            </Pressable>
            <Text style={styles.headerTitle}>Comments</Text>
            <View style={{ width: 40 }} />
          </View>
          <Text style={styles.commentCount}>{commentCount + comments.length} comments</Text>
        </View>

        <FlatList
          data={comments}
          renderItem={renderComment}
          keyExtractor={(item) => item.id}
          contentContainerStyle={styles.commentsList}
          ItemSeparatorComponent={() => <View style={styles.separator} />}
        />

        <View style={styles.inputContainer}>
          <Image 
            source={{ uri: 'https://i.pravatar.cc/150?img=50' }}
            style={styles.inputAvatar}
            contentFit="cover"
          />
          <TextInput
            style={styles.input}
            placeholder="Add a comment..."
            placeholderTextColor={colors.textMuted}
            value={newComment}
            onChangeText={setNewComment}
            multiline
          />
          <Pressable 
            style={[styles.sendButton, !newComment.trim() && styles.sendButtonDisabled]}
            onPress={handleAddComment}
            disabled={!newComment.trim()}
          >
            <MaterialIcons 
              name="send" 
              size={22} 
              color={newComment.trim() ? colors.primary : colors.textMuted} 
            />
          </Pressable>
        </View>
      </KeyboardAvoidingView>
    </Modal>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  header: {
    paddingTop: spacing.xl,
    paddingHorizontal: spacing.md,
    paddingBottom: spacing.md,
    borderBottomWidth: 1,
    borderBottomColor: colors.surfaceLight,
  },
  headerTop: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: spacing.sm,
  },
  closeButton: {
    padding: spacing.xs,
  },
  headerTitle: {
    ...typography.h3,
    color: colors.text,
    fontWeight: '700',
  },
  commentCount: {
    fontSize: 13,
    color: colors.textMuted,
  },
  commentsList: {
    padding: spacing.md,
  },
  commentItem: {
    flexDirection: 'row',
    gap: spacing.sm,
  },
  commentAvatar: {
    width: 36,
    height: 36,
    borderRadius: 18,
    backgroundColor: colors.surfaceLight,
  },
  commentContent: {
    flex: 1,
  },
  commentHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: spacing.sm,
    marginBottom: 4,
  },
  commentUserName: {
    fontSize: 13,
    color: colors.text,
    fontWeight: '600',
  },
  commentTime: {
    fontSize: 11,
    color: colors.textMuted,
  },
  commentText: {
    fontSize: 14,
    color: colors.text,
    lineHeight: 20,
    marginBottom: spacing.sm,
  },
  commentActions: {
    flexDirection: 'row',
    gap: spacing.lg,
  },
  commentAction: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  commentActionText: {
    fontSize: 12,
    color: colors.textMuted,
    fontWeight: '500',
  },
  separator: {
    height: spacing.md,
  },
  inputContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: spacing.md,
    borderTopWidth: 1,
    borderTopColor: colors.surfaceLight,
    gap: spacing.sm,
    backgroundColor: colors.surface,
  },
  inputAvatar: {
    width: 32,
    height: 32,
    borderRadius: 16,
    backgroundColor: colors.surfaceLight,
  },
  input: {
    flex: 1,
    backgroundColor: colors.background,
    borderRadius: 20,
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.sm,
    color: colors.text,
    fontSize: 14,
    maxHeight: 100,
  },
  sendButton: {
    padding: spacing.sm,
  },
  sendButtonDisabled: {
    opacity: 0.5,
  },
});
