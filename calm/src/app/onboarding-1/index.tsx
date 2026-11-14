import { useRouter } from 'expo-router';
import {
  Box,
  ButtonContainer,
  Container,
  Content,
  Description,
  NextButton,
  NextButtonText,
  Scroll,
  Subtitle,
  Title,
  Trail,
} from './styled';

export default function Onboarding1Screen() {
  const router = useRouter();

  const handleNext = () => {
    router.push('/onboarding-2');
  };

  return (
    <Container>
      <Scroll>
        <Content>
          <Title>Bem-vindo ao CALM</Title>

          <Subtitle>
            O aplicativo para se conectar à pulseira que detecta ansiedade da
            pessoa que você se importa e monitorar os momentos de ansiedade
            dela.
          </Subtitle>

          <Description>
            Com o CALM, você pode acompanhar os momentos de ansiedade da pessoa
            que mais importa e oferecer apoio no momento difícil.
          </Description>
        </Content>

        <Trail>
          <Box active />
          <Box />
          <Box />
        </Trail>

        <ButtonContainer>
          <NextButton onPress={handleNext}>
            <NextButtonText>Próximo</NextButtonText>
          </NextButton>
        </ButtonContainer>
      </Scroll>
    </Container>
  );
}
