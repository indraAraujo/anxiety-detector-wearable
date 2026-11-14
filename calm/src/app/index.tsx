import { AnxietyBpmChart } from '@/components/AnxietyBpmChart';
import { mockAnxietyEpisodes } from '@/data/mockAnxietyData';
import { hasOnboardingCompleted } from '@/utils/onboarding';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { Redirect, useRouter } from 'expo-router';
import { useEffect, useState } from 'react';
import { ActivityIndicator, Alert } from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import styled from 'styled-components/native';

const Container = styled(SafeAreaView)`
  flex: 1;
  background-color: ${({ theme }) => theme.colors.background};
`;

const Scroll = styled.ScrollView.attrs(() => ({
  contentContainerStyle: {
    paddingBottom: 40,
  },
}))``;

const Header = styled.View`
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  padding-horizontal: 20px;
  padding-top: 12px;
  padding-bottom: 8px;
`;

const HeaderLeft = styled.View``;

const AppTitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.title};
  font-size: 32px;
  color: ${({ theme }) => theme.colors.textMain};
`;

const Subtitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 13px;
  color: ${({ theme }) => theme.colors.textMuted};
  margin-top: -4px;
`;

const StatusBadge = styled.View`
  flex-direction: row;
  align-items: center;
  background-color: rgba(98, 69, 43, 0.08);
  padding-horizontal: 10px;
  padding-vertical: 6px;
  border-radius: 12px;
`;

const StatusDot = styled.View`
  width: 8px;
  height: 8px;
  border-radius: 4px;
  background-color: #48bb78;
  margin-right: 6px;
`;

const StatusText = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textMain};
`;

const MetricsContainer = styled.View`
  flex-direction: row;
  justify-content: space-between;
  padding-horizontal: 16px;
  margin-top: 12px;
  margin-bottom: 4px;
`;

const MetricCard = styled.View`
  flex: 1;
  background-color: ${({ theme }) => theme.colors.cardElevated};
  border-radius: 14px;
  padding: 12px 10px;
  margin-horizontal: 4px;
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
  align-items: center;
`;

const MetricValue = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 18px;
  color: ${({ theme }) => theme.colors.alertCrisis};
`;

const MetricLabel = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 11px;
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-top: 2px;
`;


const ResetButton = styled.Pressable`
  align-self: center;
  margin-top: 24px;
  padding: 10px 16px;
  border-radius: 10px;
  background-color: rgba(98, 69, 43, 0.08);
`;

const ResetButtonText = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textMuted};
`;

export default function HomeScreen() {
  const router = useRouter();
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

  const handleResetOnboarding = async () => {
    Alert.alert(
      'Refazer Onboarding',
      'Deseja redefinir o status de onboarding para testar as telas novamente?',
      [
        { text: 'Cancelar', style: 'cancel' },
        {
          text: 'Redefinir',
          style: 'destructive',
          onPress: async () => {
            await AsyncStorage.removeItem('@calm_onboarding_completed');
            router.replace('/onboarding-1');
          },
        },
      ]
    );
  };

  if (checking) {
    return (
      <Container style={{ justifyContent: 'center', alignItems: 'center' }}>
        <ActivityIndicator size="large" />
      </Container>
    );
  }

  if (!completed) {
    return <Redirect href="/onboarding-1" />;
  }

  // Estatísticas calculadas com base nos dados dos últimos 7 dias
  const bpms = mockAnxietyEpisodes.map((ep) => ep.bpm);
  const peakBpm = Math.max(...bpms);
  const avgBpm = Math.round(bpms.reduce((acc, curr) => acc + curr, 0) / bpms.length);
  const lastBpm = mockAnxietyEpisodes[mockAnxietyEpisodes.length - 1].bpm;

  return (
    <Container>
      <Scroll>
        <Header>
          <HeaderLeft>
            <AppTitle>CALM</AppTitle>
            <Subtitle>Monitoramento da Pulseira</Subtitle>
          </HeaderLeft>
          <StatusBadge>
            <StatusDot />
            <StatusText>Conectado</StatusText>
          </StatusBadge>
        </Header>

        {/* Métricas rápidas da semana */}
        <MetricsContainer>
          <MetricCard>
            <MetricValue>{lastBpm}</MetricValue>
            <MetricLabel>Último Pico (BPM)</MetricLabel>
          </MetricCard>
          <MetricCard>
            <MetricValue>{peakBpm}</MetricValue>
            <MetricLabel>Máximo 7 dias</MetricLabel>
          </MetricCard>
          <MetricCard>
            <MetricValue>{avgBpm}</MetricValue>
            <MetricLabel>Média em Crise</MetricLabel>
          </MetricCard>
        </MetricsContainer>

        {/* Gráfico de Batimentos Cardíacos em Ansiedade */}
        <AnxietyBpmChart />

      </Scroll>
    </Container>
  );
}
