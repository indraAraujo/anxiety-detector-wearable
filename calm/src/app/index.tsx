import { Alert, PermissionsAndroid, Platform } from 'react-native';

import { HomeView } from '@/components/homeView';
import { AuthorizationStatus, getMessaging, type RemoteMessage } from '@react-native-firebase/messaging';
import { useEffect } from 'react';

export default function HomeScreen() {

  useEffect(() => {
    // 1. Solicita as permissões do usuário
    requestUserPermission();

    // 2. Ouve notificações recebidas com o app em primeiro plano (Foreground)
    const unsubscribe = getMessaging().onMessage(async (remoteMessage: RemoteMessage) => {
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
    const unsubscribe = getMessaging().onNotificationOpenedApp((remoteMessage: RemoteMessage) => {
      console.log('App aberto a partir do segundo plano:', remoteMessage.notification);
      // Exemplo: navigation.navigate(remoteMessage.data.screen);
    });

    // Caso 2: O app estava TOTALMENTE FECHADO e foi aberto pelo clique na notificação
    getMessaging()
      .getInitialNotification()
      .then((remoteMessage: RemoteMessage | null) => {
        if (remoteMessage) {
          console.log('App aberto a partir do estado totalmente fechado:', remoteMessage.notification);
          // Exemplo: navigation.navigate(remoteMessage.data.screen);
        }
      });

    return unsubscribe;
  }, []);
  return (
    <HomeView />
  );
}
