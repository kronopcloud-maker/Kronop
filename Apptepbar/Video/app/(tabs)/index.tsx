import { View, Text, StyleSheet, TextInput, FlatList } from 'react-native';
import { useRouter } from 'expo-router';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { MaterialIcons } from '@expo/vector-icons';
import { colors, spacing, typography } from '../../ThemeConstants';
import { VideoCard, CategoryBar } from '../../components';
import { useLongVideos } from '../../hooks/useVideos';
import { useSearchEngine } from '../../searchEngine';
import { useCategoryFilter } from '../../hooks/useCategoryFilter';
import { getAllCategories } from '../../services/categoryService';

const SEARCH_BAR_HEIGHT = 56;

export default function Index() {
  const router = useRouter();
  const insets = useSafeAreaInsets();
  const { videos, toggleLike } = useLongVideos();
  
  
  // Category Filter
  const { selectedCategory, setSelectedCategory, filteredVideos: categoryFilteredVideos } = useCategoryFilter(videos);
  const categories = getAllCategories();
  
  // Search Engine Integration
  const { 
    searchQuery, 
    setSearchQuery, 
    filteredResults: searchFilteredVideos,
    clearSearch,
    hasResults,
    isSearching
  } = useSearchEngine(categoryFilteredVideos);
  
  const finalVideos = searchFilteredVideos;

  const handleVideoPress = (videoId: string) => {
    router.push({ pathname: '/video/[id]', params: { id: videoId, type: 'long' } });
  };

  return (
    <View style={styles.container}>
<View style={[styles.header, { paddingTop: insets.top + spacing.sm }]}>
        <View style={styles.searchContainer}>
          <MaterialIcons name="search" size={20} color={colors.textMuted} style={styles.searchIcon} />
          <TextInput
            style={styles.searchInput}
            placeholder="Search videos..."
            placeholderTextColor={colors.textMuted}
            value={searchQuery}
            onChangeText={setSearchQuery}
          />
          {isSearching && (
            <MaterialIcons 
              name="close" 
              size={20} 
              color={colors.textMuted} 
              style={styles.clearIcon}
              onPress={clearSearch}
            />
          )}
        </View>
      </View>

<View style={styles.categoryBarWrapper}>
        <CategoryBar
          categories={categories}
          selectedCategory={selectedCategory}
          onSelectCategory={setSelectedCategory}
        />
      </View>

      <FlatList
        data={finalVideos}
        keyExtractor={item => item.id}
        renderItem={({ item }) => (
          <VideoCard
            video={item}
            onPress={() => handleVideoPress(item.id)}
            onLike={() => toggleLike(item.id)}
          />
        )}
        contentContainerStyle={styles.list}
        showsVerticalScrollIndicator={false}
        ListEmptyComponent={
          isSearching ? (
            <View style={styles.emptyContainer}>
              <MaterialIcons name="search-off" size={64} color={colors.textMuted} />
              <Text style={styles.emptyText}>No videos found for "{searchQuery}"</Text>
            </View>
          ) : (
            <View style={styles.emptyContainer}>
              <MaterialIcons name="videocam-off" size={64} color={colors.textMuted} />
              <Text style={styles.emptyText}>No videos in this category</Text>
            </View>
          )
        }
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  header: {
    backgroundColor: colors.background,
    paddingHorizontal: spacing.xs,
    paddingBottom: spacing.sm,
  },
  list: {
    paddingTop: spacing.xs,
    paddingBottom: spacing.md,
  },
  searchContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: colors.surfaceLight,
    borderRadius: 16,
    paddingHorizontal: spacing.md,
    height: 40,
    marginHorizontal: 0,
  },
  searchIcon: {
    marginRight: spacing.sm,
  },
  searchInput: {
    flex: 1,
    color: colors.text,
    fontSize: 16,
    paddingVertical: spacing.sm,
  },
  clearIcon: {
    marginLeft: spacing.sm,
    padding: 4,
  },
  emptyContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: spacing.xxl * 2,
  },
  emptyText: {
    ...typography.body,
    color: colors.textMuted,
    marginTop: spacing.md,
  },
  categoryBarWrapper: {
    marginTop: 0,
  },
});

