import { SafeAreaView } from "react-native-safe-area-context";
import styled from "styled-components/native";

export const Container = styled(SafeAreaView)`
  flex: 1;
  background-color: ${({ theme }) => theme.colors.background};
`;

export const Scroll = styled.ScrollView.attrs(() => ({
    contentContainerStyle: {
        paddingBottom: 40,
    },
}))``

export const Header = styled.View`
  flex-direction: row;
  justify-content: space-between;
  align-items: center;
  padding-horizontal: 20px;
  padding-top: 12px;
  padding-bottom: 8px;
`;

export const HeaderLeft = styled.View``;

export const AppTitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.title};
  font-size: 32px;
  color: ${({ theme }) => theme.colors.textMain};
`;

export const Subtitle = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 13px;
  color: ${({ theme }) => theme.colors.textMuted};
  margin-top: -4px;
`;

export const StatusBadge = styled.View`
  flex-direction: row;
  align-items: center;
  background-color: rgba(98, 69, 43, 0.08);
  padding-horizontal: 10px;
  padding-vertical: 6px;
  border-radius: 12px;
`;

export const StatusDot = styled.View`
  width: 8px;
  height: 8px;
  border-radius: 4px;
  background-color: #48bb78;
  margin-right: 6px;
`;

export const StatusText = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textMain};
`;

export const MetricsContainer = styled.View`
  flex-direction: row;
  justify-content: space-between;
  padding-horizontal: 16px;
  margin-top: 12px;
  margin-bottom: 4px;
`;

export const MetricCard = styled.View`
  flex: 1;
  background-color: ${({ theme }) => theme.colors.cardElevated};
  border-radius: 14px;
  padding: 12px 10px;
  margin-horizontal: 4px;
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
  align-items: center;
`;

export const MetricValue = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 18px;
  color: ${({ theme }) => theme.colors.alertCrisis};
`;

export const MetricLabel = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 11px;
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-top: 2px;
`;

export const ResetButton = styled.Pressable`
  align-self: center;
  margin-top: 24px;
  padding: 10px 16px;
  border-radius: 10px;
  background-color: rgba(98, 69, 43, 0.08);
`;

export const ResetButtonText = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textMuted};
`;