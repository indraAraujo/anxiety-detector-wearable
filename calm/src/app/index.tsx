import { useEffect, useState } from 'react';
import { ActivityIndicator, Text, View } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Redirect } from 'expo-router';
import { hasOnboardingCompleted } from '@/utils/onboarding';

export default function HomeScreen() {
  const [checking, setChecking] = useState(true);
  const [completed, setCompleted] = useState<boolean | null>(null);

  useEffect(() => {
    async function check() {
      const isCompleted = await hasOnboardingCompleted();
      setCompleted(isCompleted);
      setChecking(false);
    }
    check();
  }, []);

  if (checking) {
    return (
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        <ActivityIndicator size="large" />
      </View>
    );
  }

  if (!completed) {
    return <Redirect href="/onboarding-1" />;
  }

  return (
    <SafeAreaView style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Text>Home</Text>
    </SafeAreaView>
  );
}
