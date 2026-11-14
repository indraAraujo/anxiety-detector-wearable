import styled from "styled-components/native";

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