import { useState } from 'react';
import { ActivityIndicator, Alert } from 'react-native';
import { useRouter } from 'expo-router';
import { useTheme } from 'styled-components/native';
import {
  ActionButton,
  ActionButtonText,
  BackButton,
  BackText,
  Box,
  Card,
  Container,
  Input,
  LoadingContainer,
  Scroll,
  Subtitle,
  Title,
  Trail,
} from './styled';

export default function Onboarding2Screen() {
  const [code, setCode] = useState('');
  const [isSaving, setIsSaving] = useState(false);
  const router = useRouter();
  const theme = useTheme();

  const handleInsert = async () => {
    if (!code.trim()) {
      Alert.alert('Atenção', 'Por favor, digite o código da pulseira');
      return;
    }
    setIsSaving(true);
    router.push('/onboarding-3');
  };

  const handleBack = () => {
    if (router.canGoBack()) {
      router.back();
    }
  };

  return (
    <Container>
      <Scroll>
        <BackButton onPress={handleBack}>
          <BackText>← Voltar</BackText>
        </BackButton>

        <Card>
          <Title>Código da pulseira</Title>

          <Subtitle>
            Digite o código identificador da pulseira em uso
          </Subtitle>

          <Input
            placeholder="Código da pulseira"
            placeholderTextColor={theme.colors.textMuted}
            value={code}
            onChangeText={setCode}
            autoCapitalize="none"
            returnKeyType="done"
          />

          <ActionButton onPress={handleInsert} disabled={!code.trim() || isSaving}>
            <ActionButtonText>Inserir</ActionButtonText>
          </ActionButton>

          {isSaving && (
            <LoadingContainer>
              <ActivityIndicator size="small" color={theme.colors.calm.dark} />
            </LoadingContainer>
          )}
        </Card>

        <Trail>
          <Box />
          <Box active />
          <Box />
        </Trail>
      </Scroll>
    </Container>
  );
}

