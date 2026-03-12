import { AppColors } from '../../appColor/AppColors';

export const colors = {
  // Base colors mapped from main app
  background: AppColors.background.primary, // '#000000'
  surface: AppColors.background.secondary, // '#0A0A0A'
  surfaceLight: AppColors.background.elevated, // '#1A1A1A'

  // Brand colors
  primary: AppColors.primary.main, // '#8B00FF'

  // Text colors
  text: AppColors.text.primary, // '#FFFFFF'
  textSubtle: AppColors.text.secondary, // '#CCCCCC'
  textMuted: AppColors.text.tertiary, // '#999999'

  // Semantic colors
  success: AppColors.success,
  error: AppColors.error,
  warning: AppColors.warning,

  // Overlays
  overlay: AppColors.overlay,
};

export const spacing = {
  xs: 4,
  sm: 8,
  md: 16,
  lg: 24,
  xl: 32,
  xxl: 48,
};

export const typography = {
  h1: {
    fontSize: 28,
    fontWeight: '700' as const,
    lineHeight: 36,
  },
  h2: {
    fontSize: 22,
    fontWeight: '600' as const,
    lineHeight: 28,
  },
  h3: {
    fontSize: 18,
    fontWeight: '600' as const,
    lineHeight: 24,
  },
  body: {
    fontSize: 16,
    fontWeight: '400' as const,
    lineHeight: 24,
  },
  bodySmall: {
    fontSize: 14,
    fontWeight: '400' as const,
    lineHeight: 20,
  },
  caption: {
    fontSize: 12,
    fontWeight: '500' as const,
    lineHeight: 16,
  },
};

export const borderRadius = {
  sm: 8,
  md: 12,
  lg: 16,
  xl: 24,
  full: 9999,
};
