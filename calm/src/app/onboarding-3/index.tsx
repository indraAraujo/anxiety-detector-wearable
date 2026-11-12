import { useState } from 'react';
import { ActivityIndicator } from 'react-native';
import { useRouter } from 'expo-router';
import { useTheme } from 'styled-components/native';
import { markOnboardingCompleted } from '@/utils/onboarding';
import {
  BackButton,
  BackText,
  Box,
  ButtonContainer,
  Container,
  Content,
  Description,
  FinishButton,
  FinishButtonText,
  Scroll,
  Subtitle,
  Title,
  Trail,
} from './styled';

export default function Onboarding3Screen() {
  const router = useRouter();
  const theme = useTheme();
  const [isCompleting, setIsCompleting] = useState(false);

  const handleComplete = async () => {
    setIsCompleting(true);
    await markOnboardingCompleted();
    setTimeout(() => {
      router.replace('/');
    }, 1500);
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

        <Content>
          <Title>Seja bem-vindo!</Title>

          <Subtitle>
            Sua conexão com a pulseira foi estabelecida com sucesso.
          </Subtitle>

          <Description>
            Agora você pode monitorar os momentos de ansiedade e oferecer apoio
            quando mais precisar.
          </Description>
        </Content>

        <Trail>
          <Box />
          <Box />
          <Box active />
        </Trail>

        <ButtonContainer>
          <FinishButton onPress={handleComplete} disabled={isCompleting}>
            {isCompleting ? (
              <ActivityIndicator size="small" color={theme.colors.cardElevated} />
            ) : (
              <FinishButtonText>Finalizar</FinishButtonText>
            )}
          </FinishButton>
        </ButtonContainer>
      </Scroll>
    </Container>
  );
}

