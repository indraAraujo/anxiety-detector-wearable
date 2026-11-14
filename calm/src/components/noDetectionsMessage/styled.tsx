import styled from 'styled-components/native';

export default function StyledRoute() {
  return null;
}

export const Container = styled.View`
  margin: 16px;
  background-color: ${({ theme }) => theme.colors.cardElevated};
  border-radius: 20px;
  padding: 32px 24px;
  border-width: 1px;
  border-color: ${({ theme }) => theme.colors.borderSubtle};
  align-items: center;
`;

export const IconCircle = styled.View`
  width: 80px;
  height: 80px;
  border-radius: 40px;
  background-color: rgba(72, 187, 120, 0.12);
  align-items: center;
  justify-content: center;
  margin-bottom: 20px;
`;

export const IconText = styled.Text`
  font-size: 36px;
`;

export const Title = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyBold};
  font-size: 18px;
  color: ${({ theme }) => theme.colors.textMain};
  text-align: center;
  margin-bottom: 8px;
`;

export const Description = styled.Text`
  font-family: ${({ theme }) => theme.fonts.body};
  font-size: 14px;
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  line-height: 20px;
  max-width: 280px;
`;

export const Divider = styled.View`
  width: 40px;
  height: 3px;
  border-radius: 2px;
  background-color: rgba(72, 187, 120, 0.3);
  margin: 16px 0;
`;

export const Hint = styled.Text`
  font-family: ${({ theme }) => theme.fonts.bodyMedium};
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textMuted};
  text-align: center;
  margin-top: 4px;
`;

