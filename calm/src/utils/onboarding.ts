import AsyncStorage from '@react-native-async-storage/async-storage';

const ONBOARDING_KEY = '@calm_onboarding_completed';

export async function hasOnboardingCompleted(): Promise<boolean> {
  try {
    const completed = await AsyncStorage.getItem(ONBOARDING_KEY);
    return completed === 'true';
  } catch (error) {
    console.error('Error checking onboarding completion:', error);
    return false;
  }
}

export async function markOnboardingCompleted(): Promise<void> {
  try {
    await AsyncStorage.setItem(ONBOARDING_KEY, 'true');
  } catch (error) {
    console.error('Error marking onboarding as completed:', error);
  }
}