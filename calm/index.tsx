import * as Notifications from 'expo-notifications';
import { AndroidImportance } from 'expo-notifications';
import { getMessaging } from '@react-native-firebase/messaging';

async function createNotificationChannel() {
  const { status: existingStatus } = await Notifications.getPermissionsAsync();
  let finalStatus = existingStatus;
  if (existingStatus !== 'granted') {
    const { status } = await Notifications.requestPermissionsAsync();
    finalStatus = status;
  }
  if (finalStatus !== 'granted') {
    return;
  }

  await Notifications.setNotificationChannelAsync('default', {
    name: 'Default Channel',
    importance: AndroidImportance.DEFAULT,
    sound: 'default',
  });
}

createNotificationChannel();

// Registra o handler de segundo plano antes de carregar o app
getMessaging().setBackgroundMessageHandler(async remoteMessage => {
    console.log('Notificação recebida em segundo plano/fechado:', remoteMessage);
});

// Inicializa o Expo Router normalmente
import 'expo-router/entry';