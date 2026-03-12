import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert, TextInput, ScrollView, ActivityIndicator } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import * as DocumentPicker from 'expo-document-picker';
import * as ImagePicker from 'expo-image-picker';
import { Image } from 'expo-image';
import { useRouter } from 'expo-router';

interface ReelData {
  title: string;
  description: string;
  category: string;
  tags: string[];
  coverPhoto?: any;
}

interface ReelsUploadProps {
  onClose: () => void;
  onUpload?: (fileUri: string, metadata: any) => Promise<void>;
  uploading?: boolean;
  uploadProgress?: number;
}

export default function ReelsUpload({ 
  onClose, 
  onUpload, 
  uploading = false, 
  uploadProgress = 0 
}: ReelsUploadProps) {
  const router = useRouter();
  const [selectedFile, setSelectedFile] = useState<any>(null);
  const [reelData, setReelData] = useState<ReelData>({ 
    title: '', 
    description: '', 
    category: '', 
    tags: [], 
    coverPhoto: null 
  });
  const [tagInput, setTagInput] = useState('');

  const categories = [
    'Entertainment', 'Music', 'Gaming', 'Sports', 'Comedy', 
    'Education', 'Travel', 'Food', 'Fashion', 'Technology', 'Other'
  ];

  const pickReel = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: ['video/*'],
        copyToCacheDirectory: true,
      });

      if (!result.canceled && result.assets && result.assets.length > 0) {
        const file = result.assets[0];
        
        // Basic validation
        const MAX_SIZE = 100 * 1024 * 1024; // 100MB
        if (file.size && file.size > MAX_SIZE) {
          Alert.alert('File Too Large', 'Video files must be less than 100MB');
          return;
        }

        setSelectedFile(file);
        if (!reelData.title) {
          setReelData(prev => ({
            ...prev,
            title: prev.title || file.name.split('.')[0]
          }));
        }
      }
    } catch (error) {
      console.error('Error picking reel:', error);
      Alert.alert('Error', 'Failed to pick video file');
    }
  };

  const addTag = () => {
    if (tagInput.trim() && !reelData.tags.includes(tagInput.trim())) {
      setReelData(prev => ({
        ...prev,
        tags: [...prev.tags, tagInput.trim()]
      }));
      setTagInput('');
    }
  };

  const removeTag = (tagToRemove: string) => {
    setReelData(prev => ({
      ...prev,
      tags: prev.tags.filter(tag => tag !== tagToRemove)
    }));
  };

  const handleUpload = async () => {
    if (!selectedFile) {
      Alert.alert('No File Selected', 'Please select a video file');
      return;
    }

    if (!reelData.title.trim()) {
      Alert.alert('Missing Title', 'Please enter a title for your reel');
      return;
    }

    // UI Component - Delegate to Bridge Controller
    if (onUpload) {
      // Bridge controls the upload
      await onUpload(selectedFile.uri, {
        ...reelData,
        size: selectedFile.size,
        type: selectedFile.mimeType || 'video/mp4',
        name: selectedFile.name
      });
    } else {
      // Fallback for standalone usage
      Alert.alert('Upload Disabled', 'Reel upload is currently disabled. The app will work normally once a new storage service is configured.');
    }
  };

  return (
    <ScrollView style={styles.container} showsVerticalScrollIndicator={false}>
      <View style={styles.uploadArea}>
        <TouchableOpacity 
          style={styles.uploadButton}
          onPress={pickReel}
          disabled={uploading}
        >
          <MaterialIcons name="video-library" size={32} color="#8B00FF" />
          <Text style={styles.uploadButtonText}>Choose Video</Text>
          <Text style={styles.uploadButtonSubtext}>Select MP4, MOV, AVI</Text>
        </TouchableOpacity>

        {selectedFile && (
          <View style={styles.selectedFileContainer}>
            <Text style={styles.selectedFileName}>{selectedFile.name}</Text>
            <Text style={styles.selectedFileSize}>
              {selectedFile.size ? `${(selectedFile.size / 1024 / 1024).toFixed(2)} MB` : 'Unknown size'}
            </Text>
          </View>
        )}
      </View>

      <View style={styles.formSection}>
        <View style={styles.inputGroup}>
          <Text style={styles.label}>Title *</Text>
          <TextInput
            style={styles.input}
            value={reelData.title}
            onChangeText={(text) => setReelData(prev => ({ ...prev, title: text }))}
            placeholder="Enter reel title..."
            placeholderTextColor="#666"
            maxLength={100}
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.label}>Description</Text>
          <TextInput
            style={[styles.input, styles.textArea]}
            value={reelData.description}
            onChangeText={(text) => setReelData(prev => ({ ...prev, description: text }))}
            placeholder="Add description..."
            placeholderTextColor="#666"
            multiline
            numberOfLines={3}
            maxLength={500}
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.label}>Category *</Text>
          <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.categoryScroll}>
            {categories.map((category) => (
              <TouchableOpacity
                key={category}
                style={[
                  styles.categoryChip,
                  reelData.category === category && styles.categoryChipSelected
                ]}
                onPress={() => setReelData(prev => ({ ...prev, category }))}
              >
                <Text style={[
                  styles.categoryChipText,
                  reelData.category === category && styles.categoryChipTextSelected
                ]}>
                  {category}
                </Text>
              </TouchableOpacity>
            ))}
          </ScrollView>
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.label}>Tags</Text>
          <View style={styles.tagInputContainer}>
            <TextInput
              style={styles.tagInput}
              value={tagInput}
              onChangeText={setTagInput}
              placeholder="Add tags..."
              placeholderTextColor="#666"
              onSubmitEditing={addTag}
              returnKeyType="done"
            />
            <TouchableOpacity style={styles.addTagButton} onPress={addTag}>
              <MaterialIcons name="add" size={20} color="#8B00FF" />
            </TouchableOpacity>
          </View>
          
          {reelData.tags.length > 0 && (
            <View style={styles.tagsContainer}>
              {reelData.tags.map((tag, index) => (
                <View key={index} style={styles.tag}>
                  <Text style={styles.tagText}>#{tag}</Text>
                  <TouchableOpacity onPress={() => removeTag(tag)}>
                    <MaterialIcons name="close" size={16} color="#666" />
                  </TouchableOpacity>
                </View>
              ))}
            </View>
          )}
        </View>
      </View>

      <TouchableOpacity 
        style={[styles.uploadButtonMain, uploading && styles.uploadButtonDisabled]}
        onPress={handleUpload}
        disabled={uploading || !selectedFile}
      >
        {uploading ? (
          <>
            <ActivityIndicator size="small" color="#FFFFFF" />
            <View style={{ width: 8 }} />
            <Text style={styles.uploadButtonMainText}>
              Uploading... {uploadProgress}%
            </Text>
          </>
        ) : (
          <>
            <MaterialIcons name="upload" size={24} color="#FFFFFF" />
            <View style={{ width: 8 }} />
            <Text style={styles.uploadButtonMainText}>Upload Reel</Text>
          </>
        )}
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#000000',
  },
  uploadArea: {
    padding: 8,
  },
  uploadButton: {
    flex: 1,
    borderWidth: 2,
    borderColor: '#333333',
    borderRadius: 12,
    padding: 16,
    alignItems: 'center',
    backgroundColor: '#1A1A1A',
  },
  uploadButtonText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#FFFFFF',
    marginTop: 8,
  },
  uploadButtonSubtext: {
    fontSize: 12,
    color: '#666666',
    marginTop: 4,
  },
  selectedFileContainer: {
    marginTop: 12,
    padding: 12,
    backgroundColor: '#1A1A1A',
    borderRadius: 8,
  },
  selectedFileName: {
    fontSize: 14,
    fontWeight: '500',
    color: '#FFFFFF',
    marginBottom: 4,
  },
  selectedFileSize: {
    fontSize: 12,
    color: '#CCCCCC',
  },
  formSection: {
    padding: 16,
  },
  inputGroup: {
    marginBottom: 16,
  },
  label: {
    fontSize: 14,
    fontWeight: '600',
    color: '#FFFFFF',
    marginBottom: 8,
  },
  input: {
    backgroundColor: '#1A1A1A',
    borderWidth: 1,
    borderColor: '#333333',
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 12,
    fontSize: 14,
    color: '#FFFFFF',
  },
  textArea: {
    height: 80,
    paddingTop: 12,
  },
  categoryScroll: {
    flexDirection: 'row',
  },
  categoryChip: {
    backgroundColor: '#1A1A1A',
    borderWidth: 1,
    borderColor: '#333333',
    borderRadius: 20,
    paddingHorizontal: 12,
    paddingVertical: 6,
    marginRight: 8,
  },
  categoryChipSelected: {
    backgroundColor: '#8B00FF',
    borderColor: '#8B00FF',
  },
  categoryChipText: {
    fontSize: 12,
    color: '#FFFFFF',
    fontWeight: '500',
  },
  categoryChipTextSelected: {
    color: '#FFFFFF',
  },
  tagInputContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1A1A1A',
    borderWidth: 1,
    borderColor: '#333333',
    borderRadius: 8,
    paddingHorizontal: 12,
  },
  tagInput: {
    flex: 1,
    paddingVertical: 12,
    fontSize: 14,
    color: '#FFFFFF',
  },
  addTagButton: {
    backgroundColor: '#8B00FF',
    borderRadius: 6,
    padding: 8,
  },
  tagsContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    marginTop: 8,
  },
  tag: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#8B00FF',
    paddingHorizontal: 8,
    paddingVertical: 4,
    borderRadius: 12,
    marginRight: 6,
    marginBottom: 6,
  },
  tagText: {
    fontSize: 12,
    color: '#FFFFFF',
    fontWeight: '500',
  },
  uploadButtonMain: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#8B00FF',
    marginHorizontal: 16,
    marginBottom: 16,
    paddingVertical: 16,
    borderRadius: 8,
  },
  uploadButtonDisabled: {
    backgroundColor: '#333333',
  },
  uploadButtonMainText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#FFFFFF',
  },
});
