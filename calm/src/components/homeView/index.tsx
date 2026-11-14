import { useAnxietyData } from "@/hooks/useAnxietyData";
import { hasOnboardingCompleted } from "@/utils/onboarding";
import { Redirect, useRouter } from "expo-router";
import { useEffect, useState } from "react";
import { ActivityIndicator } from "react-native";
import { AnxietyBpmChart } from "../anxietyBpmChart";
import { NoDetectionsMessage } from "../noDetectionsMessage";
import { AppTitle, Container, Header, HeaderLeft, MetricCard, MetricLabel, MetricsContainer, MetricValue, Scroll, StatusBadge, StatusDot, StatusText, Subtitle } from "./styled";


export function HomeView() {
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

    const { episodes, chartData, metrics, loading: dataLoading } = useAnxietyData();

    const hasData = episodes.length > 0;

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

                {dataLoading ? (
                    <Container style={{ justifyContent: 'center', alignItems: 'center', minHeight: 200 }}>
                        <ActivityIndicator size="large" />
                    </Container>
                ) : hasData ? (
                    <>
                        {/* Métricas rápidas da semana */}
                        <MetricsContainer>
                            <MetricCard>
                                <MetricValue>{metrics.lastBpm}</MetricValue>
                                <MetricLabel>Último Pico (BPM)</MetricLabel>
                            </MetricCard>
                            <MetricCard>
                                <MetricValue>{metrics.peakBpm}</MetricValue>
                                <MetricLabel>Máximo 7 dias</MetricLabel>
                            </MetricCard>
                            <MetricCard>
                                <MetricValue>{metrics.avgBpm}</MetricValue>
                                <MetricLabel>Média em Crise</MetricLabel>
                            </MetricCard>
                        </MetricsContainer>

                        {/* Gráfico de Batimentos Cardíacos em Ansiedade */}
                        <AnxietyBpmChart data={chartData} />
                    </>
                ) : (
                    <NoDetectionsMessage />
                )}

            </Scroll>
        </Container>
    );
}