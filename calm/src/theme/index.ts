export const theme = {
  colors: {
    calm: {
      light: '#E1CBB9',
      medium: '#CAA484',
      dark: '#62452B',
    },
    background: '#E1CBB9',
    card: 'rgba(255, 255, 255, 0.6)',
    cardElevated: '#FFFFFF',
    textMain: '#62452B',
    textMuted: 'rgba(98, 69, 43, 0.7)',
    borderSubtle: 'rgba(98, 69, 43, 0.15)',
    alertCrisis: '#C86D51',
    alertCrisisBg: 'rgba(200, 109, 81, 0.12)',
  },
  fonts: {
    title: 'Modak',
    body: 'Nunito_400Regular',
    bodyMedium: 'Nunito_600SemiBold',
    bodyBold: 'Nunito_700Bold',
  },
};

export type ThemeType = typeof theme;