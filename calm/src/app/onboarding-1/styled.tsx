import styled from 'styled-components/native';
import { SafeAreaView } from 'react-native-safe-area-context';

export const Container = styled(SafeAreaView)`
  flex: 1;
  background-color: ${({ theme }) => theme.colors.background};
`;

export const Scroll = styled.ScrollView.attrs(() => ({
  contentContainerStyle: {
    flexGrow: 1,
    justifyContent: 'space-between',
    paddingVertical: 24,
  },
}))``;

export const Content = styled.View`
  flex: 1;
  padding: 24px;
  align-items: center;
  justify-content: center;
  max-width: 480px;
  width: 100%;
  align-self: center;
`;

export const Title = styled.Text`
  font-size: 38px;
  font-family: ${({ theme }) => theme.fonts.title};
  color: ${({ theme }) => theme.colors.textMain};
  text-align: center;
  margin-bottom: 16px;
`;

export const Subtitle = styled.Text`
  font-size: 18px;
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-bottom: 16px;
  line-height: 26px;
`;

export const Description = styled.Text`
  font-size: 15px;
  font-family: ${({ theme }) => theme.fonts.body};
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-bottom: 24px;
  line-height: 22px;
`;

export const Trail = styled.View`
  flex-direction: row;
  justify-content: center;
  margin-vertical: 24px;
  padding-horizontal: 16px;
`;

export const Box = styled.View<{ active?: boolean }>`
  width: 40px;
  height: 8px;
  border-radius: 4px;
  margin-horizontal: 4px;
  background-color: ${({ active, theme }) =>
    active ? theme.colors.calm.dark : theme.colors.calm.medium};
`;

export const ButtonContainer = styled.View`
  padding: 16px 24px;
  width: 100%;
  max-width: 480px;
  align-self: center;
`;

export const NextButton = styled.Pressable`
  background-color: ${({ theme }) => theme.colors.calm.dark};
  padding-vertical: 14px;
  padding-horizontal: 24px;
  border-radius: 12px;
  align-items: center;
  justify-content: center;
`;

export const NextButtonText = styled.Text`
  color: ${({ theme }) => theme.colors.cardElevated};
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 16px;
`;

export default function StyledRoute() {
  return null;
}
