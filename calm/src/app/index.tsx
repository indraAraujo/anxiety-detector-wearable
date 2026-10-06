import * as Device from 'expo-device';
import { Alert, AppRegistry, PermissionsAndroid, Platform, StyleSheet } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';

import { AnimatedIcon } from '@/components/animated-icon';
import { HintRow } from '@/components/hint-row';
import { ThemedText } from '@/components/themed-text';
import { ThemedView } from '@/components/themed-view';
import { WebBadge } from '@/components/web-badge';
import { BottomTabInset, MaxContentWidth, Spacing } from '@/constants/theme';
import { useEffect } from 'react';
import  {AuthorizationStatus, getMessaging} from '@react-native-firebase/messaging';

//SET ICON AND COLOR OF THE NOTIFICATION
getMessaging().setBackgroundMessageHandler(async remoteMessage => {
  console.log('Notificação recebida em segundo plano/fechado:', remoteMessage);
});

AppRegistry.registerComponent('main', () => HomeScreen);

function getDevMenuHint() {
  if (Platform.OS === 'web') {
    return <ThemedText type="small">use browser devtools</ThemedText>;
  }
  if (Device.isDevice) {
    return (
      <ThemedText type="small">
        shake device or press <ThemedText type="code">m</ThemedText> in terminal
      </ThemedText>
    );
  }
  const shortcut = Platform.OS === 'android' ? 'cmd+m (or ctrl+m)' : 'cmd+d';
  return (
    <ThemedText type="small">
      press <ThemedText type="code">{shortcut}</ThemedText>
    </ThemedText>
  );
}

export default function HomeScreen() {

  useEffect(() => {
    // 1. Solicita as permissões do usuário
    requestUserPermission();

    // 2. Ouve notificações recebidas com o app em primeiro plano (Foreground)
    const unsubscribe = getMessaging().onMessage(async remoteMessage => {
      Alert.alert(
        remoteMessage.notification?.title || 'Nova Notificação',
        remoteMessage.notification?.body || ''
      );
    });

    return unsubscribe;
  }, []);

  async function requestUserPermission() {
    try {
      if (Platform.OS === 'android' && Platform.Version >= 33) {
        const granted = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.POST_NOTIFICATIONS
        );

        if (granted !== PermissionsAndroid.RESULTS.GRANTED) {
          console.log('Permissão de notificação negada no Android.');
          return;
        }
      }

      const authStatus = await getMessaging().requestPermission();
      const enabled =
        authStatus === AuthorizationStatus.AUTHORIZED ||
        authStatus === AuthorizationStatus.PROVISIONAL;

      if (enabled) {
        console.log('Status de autorização:', authStatus);
        getFcmToken();
      } else {
        console.log('Permissão de notificação não concedida.');
      }
    } catch (error) {
      console.error('Erro ao solicitar permissão:', error);
    }
  }

  async function getFcmToken() {
    try {
      const token = await getMessaging().getToken();
      if (token) {
        console.log('Token FCM do dispositivo:', token);
      } else {
        console.log('Nenhum token FCM retornado.');
      }
    } catch (error) {
      console.error('Erro ao obter o token FCM:', error);
    }
  }

  useEffect(() => {
    // Caso 1: O app estava em SEGUNDO PLANO e o usuário clicou na notificação
    const unsubscribe = getMessaging().onNotificationOpenedApp(remoteMessage => {
      console.log('App aberto a partir do segundo plano:', remoteMessage.notification);
      // Exemplo: navigation.navigate(remoteMessage.data.screen);
    });

    // Caso 2: O app estava TOTALMENTE FECHADO e foi aberto pelo clique na notificação
    getMessaging()
      .getInitialNotification()
      .then(remoteMessage => {
        if (remoteMessage) {
          console.log('App aberto a partir do estado totalmente fechado:', remoteMessage.notification);
          // Exemplo: navigation.navigate(remoteMessage.data.screen);
        }
      });

    return unsubscribe;
  }, []);
  return (
    <ThemedView style={styles.container}>
      <SafeAreaView style={styles.safeArea}>
        <ThemedView style={styles.heroSection}>
          <AnimatedIcon />
          <ThemedText type="title" style={styles.title}>
            Welcome to&nbsp;Expo
          </ThemedText>
        </ThemedView>

        <ThemedText type="code" style={styles.code}>
          get started
        </ThemedText>

        <ThemedView type="backgroundElement" style={styles.stepContainer}>
          <HintRow
            title="Try editing"
            hint={<ThemedText type="code">src/app/index.tsx</ThemedText>}
          />
          <HintRow title="Dev tools" hint={getDevMenuHint()} />
          <HintRow
            title="Fresh start"
            hint={<ThemedText type="code">npm run reset-project</ThemedText>}
          />
        </ThemedView>

        {Platform.OS === 'web' && <WebBadge />}
      </SafeAreaView>
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    flexDirection: 'row',
  },
  safeArea: {
    flex: 1,
    paddingHorizontal: Spacing.four,
    alignItems: 'center',
    gap: Spacing.three,
    paddingBottom: BottomTabInset + Spacing.three,
    maxWidth: MaxContentWidth,
  },
  heroSection: {
    alignItems: 'center',
    justifyContent: 'center',
    flex: 1,
    paddingHorizontal: Spacing.four,
    gap: Spacing.four,
  },
  title: {
    textAlign: 'center',
  },
  code: {
    textTransform: 'uppercase',
  },
  stepContainer: {
    gap: Spacing.three,
    alignSelf: 'stretch',
    paddingHorizontal: Spacing.three,
    paddingVertical: Spacing.four,
    borderRadius: Spacing.four,
  },
});
