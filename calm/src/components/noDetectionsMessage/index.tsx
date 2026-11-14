import {
  Container,
  Description,
  Divider,
  Hint,
  IconCircle,
  IconText,
  Title,
} from './styled';

export function NoDetectionsMessage() {
  return (
    <Container>
      <IconCircle>
        <IconText>💚</IconText>
      </IconCircle>

      <Title>Tudo tranquilo por aqui!</Title>

      <Description>
        Nenhuma detecção de ansiedade foi registrada pela pulseira nos últimos 7
        dias.
      </Description>

      <Divider />

      <Hint>
        Quando a pulseira detectar um episódio de ansiedade, os dados aparecerão
        aqui no gráfico.
      </Hint>
    </Container>
  );
}
