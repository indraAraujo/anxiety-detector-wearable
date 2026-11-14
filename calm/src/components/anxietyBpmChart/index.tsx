import { AnxietyEpisode, ChartDataPoint } from '@/types/anxietyDetections';
import { useEffect, useState } from 'react';
import { useWindowDimensions } from 'react-native';
import { LineChart } from 'react-native-gifted-charts';
import { useTheme } from 'styled-components/native';
import {
  ChartCard,
  ChartHeader,
  ChartSubtitle,
  ChartTitle,
  ChartWrapper,
  DetailBpmBadge,
  DetailBpmNumber,
  DetailBpmUnit,
  DetailCard,
  DetailDay,
  DetailInfo,
  DetailRow,
  DetailTime,
  LegendDot,
  LegendItem,
  LegendLabel,
  LegendRow,
  YAxisBadge,
  YAxisBadgeText,
} from './styled';

export interface AnxietyBpmChartProps {
  data?: ChartDataPoint[];
}

export function AnxietyBpmChart({ data }: AnxietyBpmChartProps = {}) {
  const theme = useTheme();
  const { width } = useWindowDimensions();

  const chartPoints = data && data.length > 0 ? data : [];

  const getInitialEpisode = (points: ChartDataPoint[]): AnxietyEpisode => {
    const withBpm = points.filter((p) => p.episode.bpm > 0);
    if (withBpm.length > 0) {
      return withBpm.reduce((prev, curr) =>
        curr.episode.bpm > prev.episode.bpm ? curr : prev
      ).episode;
    }
    return points[0]?.episode ?? {};
  };

  const [selectedEpisode, setSelectedEpisode] = useState<AnxietyEpisode>(() =>
    getInitialEpisode(chartPoints)
  );

  useEffect(() => {
    setSelectedEpisode(getInitialEpisode(chartPoints));
  }, [data]);

  // Calcula largura responsiva para o gráfico
  const availableWidth = Math.min(width - 64, 460);
  const spacing = Math.floor((availableWidth - 50) / 6.2);

  // Adiciona handler de clique para cada ponto
  const formattedData = chartPoints.map((item) => ({
    ...item,
    onPress: () => setSelectedEpisode(item.episode),
    dataPointsColor:
      selectedEpisode.id === item.episode.id
        ? theme.colors.calm.dark
        : theme.colors.alertCrisis,
    dataPointsRadius: selectedEpisode.id === item.episode.id ? 7 : 5,
  }));

  return (
    <ChartCard>
      <ChartHeader>
        <ChartTitle>Frequência Cardíaca em Ansiedade</ChartTitle>
        <ChartSubtitle>
          BPM registrado pela pulseira nos últimos 7 dias (Dom a Sáb)
        </ChartSubtitle>
      </ChartHeader>

      <YAxisBadge>
        <YAxisBadgeText>Eixo Y: 50 a 180 BPM</YAxisBadgeText>
      </YAxisBadge>

      <ChartWrapper>
        <LineChart
          data={formattedData}
          height={210}
          width={availableWidth}
          spacing={spacing}
          initialSpacing={16}
          endSpacing={16}
          // Configuração do Eixo Y (50 a 180 BPM)
          yAxisOffset={50}
          maxValue={130} // 50 (offset) + 130 = 180 BPM máx
          noOfSections={5}
          stepValue={26} // Marcas: 50, 76, 102, 128, 154, 180
          yAxisColor={theme.colors.borderSubtle}
          yAxisThickness={1}
          yAxisTextStyle={{
            color: theme.colors.textMuted,
            fontFamily: theme.fonts.body,
            fontSize: 11,
          }}
          // Configuração do Eixo X (Dias: Dom a Sáb)
          xAxisColor={theme.colors.borderSubtle}
          xAxisThickness={1}
          xAxisLabelTextStyle={{
            color: theme.colors.textMain,
            fontFamily: theme.fonts.bodyBold,
            fontSize: 12,
          }}
          // Grade pontilhada
          rulesColor="rgba(98, 69, 43, 0.08)"
          rulesType="dashed"
          rulesThickness={1}
          // Curva estilizada e área preenchida
          curved
          color={theme.colors.alertCrisis}
          thickness={3}
          areaChart
          startFillColor={theme.colors.alertCrisis}
          endFillColor={theme.colors.cardElevated}
          startOpacity={0.35}
          endOpacity={0.02}
          // Rótulos de BPM sobre cada ponto
          textColor={theme.colors.textMain}
          textFontSize={11}
          textShiftY={-12}
          textShiftX={-4}
          isAnimated
          animationDuration={800}
        />
      </ChartWrapper>

      {/* Detalhe do ponto selecionado */}
      <DetailCard>
        <DetailRow>
          <DetailInfo>
            <DetailDay>{selectedEpisode.dayFull}</DetailDay>
            <DetailTime>Horário: {selectedEpisode.time}</DetailTime>
          </DetailInfo>
          <DetailBpmBadge>
            <DetailBpmNumber>{selectedEpisode.bpm}</DetailBpmNumber>
            <DetailBpmUnit>BPM</DetailBpmUnit>
          </DetailBpmBadge>
        </DetailRow>
      </DetailCard>

      <LegendRow>
        <LegendItem>
          <LegendDot color={theme.colors.alertCrisis} />
          <LegendLabel>Pico de Ansiedade (BPM)</LegendLabel>
        </LegendItem>
        <LegendItem>
          <LegendLabel>Toque em um dia para detalhes</LegendLabel>
        </LegendItem>
      </LegendRow>
    </ChartCard>
  );
}
