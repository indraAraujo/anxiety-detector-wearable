import AsyncStorage from '@react-native-async-storage/async-storage';

const ONBOARDING_KEY = '@calm_onboarding_completed';
const DEVICE_ID_KEY = '@calm_device_id';

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

export async function saveDeviceId(deviceId: string): Promise<void> {
  try {
    await AsyncStorage.setItem(DEVICE_ID_KEY, deviceId);
  } catch (error) {
    console.error('Error saving device ID:', error);
  }
}

export async function getDeviceId(): Promise<string | null> {
  try {
    return await AsyncStorage.getItem(DEVICE_ID_KEY);
  } catch (error) {
    console.error('Error getting device ID:', error);
    return null;
  }
}

export async function clearOnboarding(): Promise<void> {
  try {
    await AsyncStorage.multiRemove([ONBOARDING_KEY, DEVICE_ID_KEY]);
  } catch (error) {
    console.error('Error clearing onboarding:', error);
  }
}
