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
    paddingBottom: 24,
  },
}))``;

export const BackButton = styled.Pressable`
  padding: 16px;
  align-self: flex-start;
`;

export const BackText = styled.Text`
  color: ${({ theme }) => theme.colors.textMain};
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  font-size: 16px;
`;

export const Card = styled.View`
  background-color: ${({ theme }) => theme.colors.cardElevated};
  border-radius: 16px;
  padding: 24px;
  margin: 16px 24px;
  max-width: 480px;
  width: 90%;
  align-self: center;
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
`;

export const Title = styled.Text`
  font-size: 32px;
  font-family: ${({ theme }) => theme.fonts.title};
  color: ${({ theme }) => theme.colors.textMain};
  text-align: center;
  margin-bottom: 8px;
`;

export const Subtitle = styled.Text`
  font-size: 16px;
  font-family: ${({ theme }) => theme.fonts.body};
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-bottom: 24px;
  line-height: 22px;
`;

export const Input = styled.TextInput`
  background-color: #f5f0eb;
  border-radius: 12px;
  padding: 14px 16px;
  font-size: 16px;
  font-family: ${({ theme }) => theme.fonts.body};
  color: ${({ theme }) => theme.colors.textMain};
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
  margin-bottom: 20px;
`;

export const ActionButton = styled.Pressable<{ disabled?: boolean }>`
  background-color: ${({ theme }) => theme.colors.calm.dark};
  padding-vertical: 14px;
  padding-horizontal: 24px;
  border-radius: 12px;
  align-items: center;
  justify-content: center;
  opacity: ${({ disabled }) => (disabled ? 0.6 : 1)};
`;

export const ActionButtonText = styled.Text`
  color: ${({ theme }) => theme.colors.cardElevated};
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 16px;
`;

export const LoadingContainer = styled.View`
  margin-top: 16px;
  align-items: center;
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

export default function StyledRoute() {
  return null;
}
