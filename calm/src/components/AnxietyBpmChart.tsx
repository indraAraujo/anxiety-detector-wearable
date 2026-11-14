import { useState } from 'react';
import { useWindowDimensions } from 'react-native';
import { LineChart } from 'react-native-gifted-charts';
import styled, { useTheme } from 'styled-components/native';
import {
  AnxietyEpisode,
  weeklyChartData,
} from '@/data/mockAnxietyData';

export const ChartCard = styled.View`
  background-color: ${({ theme }) => theme.colors.cardElevated};
  border-radius: 20px;
  padding: 20px 16px;
  margin-horizontal: 16px;
  margin-vertical: 12px;
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
  shadow-color: ${({ theme }) => theme.colors.calm.dark};
  shadow-offset: 0px 4px;
  shadow-opacity: 0.06;
  shadow-radius: 12px;
  elevation: 2;
`;

export const ChartHeader = styled.View`
  margin-bottom: 16px;
`;

export const ChartTitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 18px;
  color: ${({ theme }) => theme.colors.textMain};
`;

export const ChartSubtitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 13px;
  color: ${({ theme }) => theme.colors.textMuted};
  margin-top: 4px;
`;

export const YAxisBadge = styled.View`
  align-self: flex-start;
  background-color: ${({ theme }) => theme.colors.alertCrisisBg};
  padding-horizontal: 8px;
  padding-vertical: 3px;
  border-radius: 6px;
  margin-bottom: 8px;
`;

export const YAxisBadgeText = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 11px;
  color: ${({ theme }) => theme.colors.alertCrisis};
`;

export const ChartWrapper = styled.View`
  align-items: center;
  justify-content: center;
  overflow: hidden;
  padding-top: 8px;
`;

export const DetailCard = styled.View`
  margin-top: 16px;
  padding: 12px 16px;
  border-radius: 14px;
  background-color: #f8f4f0;
  border-left-width: 4px;
  border-left-color: ${({ theme }) => theme.colors.alertCrisis};
`;

export const DetailRow = styled.View`
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
`;

export const DetailInfo = styled.View``;

export const DetailDay = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 15px;
  color: ${({ theme }) => theme.colors.textMain};
`;

export const DetailTime = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 13px;
  color: ${({ theme }) => theme.colors.textMuted};
  margin-top: 2px;
`;

export const DetailBpmBadge = styled.View`
  flex-direction: row;
  align-items: baseline;
  background-color: ${({ theme }) => theme.colors.alertCrisis};
  padding-horizontal: 10px;
  padding-vertical: 4px;
  border-radius: 12px;
`;

export const DetailBpmNumber = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 16px;
  color: #ffffff;
`;

export const DetailBpmUnit = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  font-size: 11px;
  color: #ffffff;
  margin-left: 3px;
`;

export const LegendRow = styled.View`
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  margin-top: 14px;
  padding-top: 10px;
  border-top-width: 1px;
  border-top-color: ${({ theme }) => theme.colors.borderSubtle};
`;

export const LegendItem = styled.View`
  flex-direction: row;
  align-items: center;
`;

export const LegendDot = styled.View<{ color: string }>`
  width: 8px;
  height: 8px;
  border-radius: 4px;
  background-color: ${({ color }) => color};
  margin-right: 6px;
`;

export const LegendLabel = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 11px;
  color: ${({ theme }) => theme.colors.textMuted};
`;

export function AnxietyBpmChart() {
  const theme = useTheme();
  const { width } = useWindowDimensions();
  const [selectedEpisode, setSelectedEpisode] = useState<AnxietyEpisode>(
    weeklyChartData[3].episode // Quarta-feira (pico máximo) selecionada por padrão
  );

  // Calcula largura responsiva para o gráfico
  const availableWidth = Math.min(width - 64, 460);
  const spacing = Math.floor((availableWidth - 50) / 6.2);

  // Adiciona handler de clique para cada ponto
  const formattedData = weeklyChartData.map((item) => ({
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
