import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Alert, TextInput, ScrollView, ActivityIndicator, FlatList } from 'react-native';
import { MaterialIcons } from '@expo/vector-icons';
import * as DocumentPicker from 'expo-document-picker';
import { useRouter } from 'expo-router';

interface SongData {
  title: string;
  artist: string;
  tags: string[];
  genre: string;
}

interface LiveUploadProps {
  onClose: () => void;
  onUpload?: (files: any[], metadata: any) => Promise<void>;
  uploading?: boolean;
  uploadProgress?: number;
}

export default function SongUpload({ 
  onClose, 
  onUpload, 
  uploading = false, 
  uploadProgress = 0 
}: LiveUploadProps) {
  const router = useRouter();
  const [selectedFiles, setSelectedFiles] = useState<any[]>([]);
  const [songData, setSongData] = useState<SongData>({ 
    title: '', 
    artist: '', 
    tags: [], 
    genre: '' 
  });
  const [tagInput, setTagInput] = useState('');

  const genres = [
    'Pop', 'Rock', 'Hip Hop', 'Jazz', 'Classical', 'Electronic',
    'Country', 'R&B', 'Reggae', 'Blues', 'Metal', 'Folk', 'Other'
  ];

  const pickSongs = async () => {
    try {
      const result = await DocumentPicker.getDocumentAsync({
        type: ['audio/*'],
        multiple: true,
        copyToCacheDirectory: true,
      });

      if (!result.canceled && result.assets && result.assets.length > 0) {
        const files = result.assets;
        
        // Basic validation
        const MAX_SIZE = 50 * 1024 * 1024; // 50MB per file
        const validFiles = files.filter(file => {
          if (file.size && file.size > MAX_SIZE) {
            Alert.alert('File Too Large', `${file.name} is too large. Maximum size is 50MB.`);
            return false;
          }
          return true;
        });

        setSelectedFiles(prev => [...prev, ...validFiles]);
        
        // Auto-fill title with first file name if empty
        if (!songData.title && validFiles.length > 0) {
          const firstName = validFiles[0].name.split('.')[0];
          setSongData(prev => ({ ...prev, title: firstName }));
        }
      }
    } catch (error) {
      console.error('Error picking songs:', error);
      Alert.alert('Error', 'Failed to pick audio files');
    }
  };

  const removeFile = (index: number) => {
    setSelectedFiles(prev => prev.filter((_, i) => i !== index));
  };

  const addTag = () => {
    if (tagInput.trim() && !songData.tags.includes(tagInput.trim())) {
      setSongData(prev => ({
        ...prev,
        tags: [...prev.tags, tagInput.trim()]
      }));
      setTagInput('');
    }
  };

  const removeTag = (tagToRemove: string) => {
    setSongData(prev => ({
      ...prev,
      tags: prev.tags.filter(tag => tag !== tagToRemove)
    }));
  };

  const handleUpload = async () => {
    if (selectedFiles.length === 0) {
      Alert.alert('No Files Selected', 'Please select at least one audio file');
      return;
    }

    if (!songData.title.trim()) {
      Alert.alert('Missing Title', 'Please enter a title for your songs');
      return;
    }

    // UI Component - Delegate to Bridge Controller
    if (onUpload) {
      // Bridge controls the upload
      await onUpload(selectedFiles, {
        ...songData,
        totalFiles: selectedFiles.length,
        fileNames: selectedFiles.map(f => f.name)
      });
    } else {
      // Fallback for standalone usage
      Alert.alert('Upload Disabled', 'Song upload is currently disabled. The app will work normally once a new storage service is configured.');
    }
  };

  const renderFileItem = ({ item, index }: { item: any, index: number }) => (
    <View style={styles.fileItem}>
      <MaterialIcons name="audio-file" size={20} color="#6A5ACD" />
      <Text style={styles.fileName} numberOfLines={1}>
        {item.name}
      </Text>
      <Text style={styles.fileSize}>
        {item.size ? `${(item.size / 1024 / 1024).toFixed(2)} MB` : 'Unknown size'}
      </Text>
      <TouchableOpacity 
        style={styles.removeButton}
        onPress={() => removeFile(index)}
      >
        <MaterialIcons name="close" size={16} color="#666" />
      </TouchableOpacity>
    </View>
  );

  return (
    <ScrollView style={styles.container} showsVerticalScrollIndicator={false}>
      <View style={styles.uploadArea}>
        <TouchableOpacity 
          style={styles.selectButton}
          onPress={pickSongs}
          disabled={uploading}
        >
          <MaterialIcons name="music-note" size={24} color="#6A5ACD" />
          <Text style={styles.selectButtonText}>Choose Audio Files</Text>
          <Text style={styles.selectButtonSubtext}>MP3, WAV, AAC, FLAC</Text>
        </TouchableOpacity>
        
        {selectedFiles.length > 0 && (
          <View style={styles.filesList}>
            <Text style={styles.filesListTitle}>
              Selected Files ({selectedFiles.length})
            </Text>
            <FlatList
              data={selectedFiles}
              renderItem={renderFileItem}
              keyExtractor={(item, index) => index.toString()}
              scrollEnabled={false}
            />
          </View>
        )}
      </View>

      <View style={styles.formSection}>
        <View style={styles.inputGroup}>
          <Text style={styles.label}>Title *</Text>
          <TextInput
            style={styles.input}
            value={songData.title}
            onChangeText={(text) => setSongData(prev => ({ ...prev, title: text }))}
            placeholder="Enter song title..."
            placeholderTextColor="#666"
            maxLength={100}
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.label}>Artist</Text>
          <TextInput
            style={styles.input}
            value={songData.artist}
            onChangeText={(text) => setSongData(prev => ({ ...prev, artist: text }))}
            placeholder="Enter artist name..."
            placeholderTextColor="#666"
            maxLength={100}
          />
        </View>

        <View style={styles.inputGroup}>
          <Text style={styles.label}>Genre *</Text>
          <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.genreScroll}>
            {genres.map((genre) => (
              <TouchableOpacity
                key={genre}
                style={[
                  styles.genreChip,
                  songData.genre === genre && styles.genreChipSelected
                ]}
                onPress={() => setSongData(prev => ({ ...prev, genre }))}
              >
                <Text style={[
                  styles.genreChipText,
                  songData.genre === genre && styles.genreChipTextSelected
                ]}>
                  {genre}
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
              <MaterialIcons name="add" size={20} color="#6A5ACD" />
            </TouchableOpacity>
          </View>
          
          {songData.tags.length > 0 && (
            <View style={styles.tagsContainer}>
              {songData.tags.map((tag, index) => (
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
        disabled={uploading || selectedFiles.length === 0}
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
            <Text style={styles.uploadButtonMainText}>Upload Songs</Text>
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
  selectButton: {
    borderWidth: 2,
    borderColor: '#333333',
    borderRadius: 12,
    padding: 20,
    alignItems: 'center',
    backgroundColor: '#1A1A1A',
  },
  selectButtonText: {
    fontSize: 16,
    fontWeight: '600',
    color: '#FFFFFF',
    marginTop: 8,
  },
  selectButtonSubtext: {
    fontSize: 12,
    color: '#666666',
    marginTop: 4,
  },
  filesList: {
    marginTop: 16,
  },
  filesListTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: '#FFFFFF',
    marginBottom: 12,
  },
  fileItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#1A1A1A',
    padding: 12,
    borderRadius: 8,
    marginBottom: 8,
  },
  fileName: {
    flex: 1,
    fontSize: 14,
    color: '#FFFFFF',
    marginLeft: 12,
  },
  fileSize: {
    fontSize: 12,
    color: '#CCCCCC',
    marginLeft: 8,
  },
  removeButton: {
    padding: 4,
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
  genreScroll: {
    flexDirection: 'row',
  },
  genreChip: {
    backgroundColor: '#1A1A1A',
    borderWidth: 1,
    borderColor: '#333333',
    borderRadius: 20,
    paddingHorizontal: 12,
    paddingVertical: 6,
    marginRight: 8,
  },
  genreChipSelected: {
    backgroundColor: '#6A5ACD',
    borderColor: '#6A5ACD',
  },
  genreChipText: {
    fontSize: 12,
    color: '#FFFFFF',
    fontWeight: '500',
  },
  genreChipTextSelected: {
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
    backgroundColor: '#6A5ACD',
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
    backgroundColor: '#6A5ACD',
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
    backgroundColor: '#6A5ACD',
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
