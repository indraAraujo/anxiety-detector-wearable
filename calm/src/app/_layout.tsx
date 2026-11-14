import { Modak_400Regular, useFonts } from '@expo-google-fonts/modak';
import {
  Nunito_400Regular,
  Nunito_600SemiBold,
  Nunito_700Bold,
} from '@expo-google-fonts/nunito';
import { Stack } from 'expo-router';
import * as SplashScreen from 'expo-splash-screen';
import { useEffect, useState } from 'react';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { ThemeProvider } from 'styled-components/native';
import { theme } from '../theme';

SplashScreen.preventAutoHideAsync();

const ONBOARDING_KEY = '@calm_onboarding_completed';

export default function RootLayout() {
  const [loaded, error] = useFonts({
    Modak: Modak_400Regular,
    Nunito_400Regular,
    Nunito_600SemiBold,
    Nunito_700Bold,
  });

  const [showOnboarding, setShowOnboarding] = useState(true);

  useEffect(() => {
    if (loaded || error) {
      SplashScreen.hideAsync();
    }
  }, [loaded, error]);

  useEffect(() => {
    async function checkOnboarding() {
      const completed = await AsyncStorage.getItem(ONBOARDING_KEY);
      setShowOnboarding(completed !== 'true');
    }
    checkOnboarding();
  }, []);

  if (!loaded && !error) {
    return null;
  }

  return (
    <ThemeProvider theme={theme}>
      <Stack
        screenOptions={{ headerShown: false }}
        initialRouteName={showOnboarding ? 'onboarding-1/index' : 'index'}
      >
        <Stack.Screen name="index" />
        <Stack.Screen name="onboarding-1/index" />
        <Stack.Screen name="onboarding-2/index" />
        <Stack.Screen name="onboarding-3/index" />
      </Stack>
    </ThemeProvider>
  );
}
