import { INFLUX_CONFIG } from '@/config/influx';
import { AnxietyEpisode } from '@/types/anxietyDetections';

export interface InfluxQueryOptions {
  url?: string;
  token?: string;
  org?: string;
  bucketId?: string;
}

const DAY_LABELS = ['Dom', 'Seg', 'Ter', 'Qua', 'Qui', 'Sex', 'Sáb'] as const;

const DAY_FULL_NAMES = [
  'Domingo',
  'Segunda-feira',
  'Terça-feira',
  'Quarta-feira',
  'Quinta-feira',
  'Sexta-feira',
  'Sábado',
] as const;

/**
 * Constrói a consulta em linguagem Flux para o InfluxDB.
 * Filtra pelos últimos 7 dias e pelo device_id fornecido.
 */
function buildFluxQuery(bucketParam: string, deviceId: string): string {
  return `from(${bucketParam})
  |> range(start: -7d)
  |> filter(fn: (r) => r["device_id"] == "${deviceId}")
  |> filter(fn: (r) => r["_field"] == "heart_rate")
  |> sort(columns: ["_time"], desc: false)`;
}

/**
 * Faz o parse do formato Annotated CSV retornado pela API do InfluxDB.
 */
export function parseInfluxCsvToEpisodes(csvText: string): AnxietyEpisode[] {
  const lines = csvText.trim().split('\n');
  let headerIndex = -1;
  let headers: string[] = [];
  const episodes: AnxietyEpisode[] = [];

  for (let i = 0; i < lines.length; i++) {
    const rawLine = lines[i].trim();
    // Pula metadados (#datatype, #group, #default) e linhas em branco
    if (rawLine.startsWith('#') || !rawLine) {
      continue;
    }

    // Identifica o cabeçalho das colunas
    if (headerIndex === -1) {
      headers = rawLine.split(',').map((h) => h.trim());
      headerIndex = i;
      continue;
    }

    const columns = rawLine.split(',').map((c) => c.trim());
    if (columns.length < headers.length) {
      continue;
    }

    const timeIndex = headers.indexOf('_time');
    const valueIndex = headers.indexOf('_value');

    if (timeIndex === -1 || valueIndex === -1) {
      continue;
    }

    const timeRaw = columns[timeIndex];
    const valueRaw = parseFloat(columns[valueIndex]);

    if (!timeRaw || isNaN(valueRaw)) {
      continue;
    }

    const date = new Date(timeRaw);
    if (isNaN(date.getTime())) {
      continue;
    }

    const bpm = Math.round(valueRaw);
    const dayOfWeek = date.getDay();
    const dayLabel = DAY_LABELS[dayOfWeek];
    const dayFull = DAY_FULL_NAMES[dayOfWeek];

    // Formata o horário em HH:MM
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const timeFormatted = `${hours}:${minutes}`;

    // Estima a intensidade do episódio pelo BPM
    let intensity: 'Leve' | 'Moderada' | 'Alta' = 'Moderada';
    if (bpm >= 135) {
      intensity = 'Alta';
    } else if (bpm <= 110) {
      intensity = 'Leve';
    }

    episodes.push({
      id: `influx-${date.getTime()}-${i}`,
      dayLabel,
      dayFull,
      time: timeFormatted,
      bpm,
      intensity,
      context: 'Episódio de ansiedade registrado pela pulseira',
    });
  }

  return episodes;
}

/**
 * Executa a consulta no endpoint /api/v2/query do InfluxDB.
 */
export async function fetchAnxietyDataFromInflux(
  deviceId: string,
  options?: InfluxQueryOptions
): Promise<AnxietyEpisode[]> {
  const url = (options?.url || INFLUX_CONFIG.url).replace(/\/$/, '');
  const token = options?.token ?? INFLUX_CONFIG.token;
  const org = options?.org || INFLUX_CONFIG.org;
  const bucketId = options?.bucketId || INFLUX_CONFIG.bucketId;

  const endpoint = `${url}/api/v2/query?org=${encodeURIComponent(org)}`;

  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    Accept: 'application/csv',
  };

  if (token) {
    headers['Authorization'] = `Token ${token}`;
  }

  // Tenta primeiro com bucketID, e se der erro de parâmetro, tenta com bucket (nome)
  const queryAttempts = [
    buildFluxQuery(`bucketID: "${bucketId}"`, deviceId),
    buildFluxQuery(`bucket: "${bucketId}"`, deviceId),
  ];

  let lastError: Error | null = null;

  for (const query of queryAttempts) {
    try {
      const response = await fetch(endpoint, {
        method: 'POST',
        headers,
        body: JSON.stringify({
          query,
          type: 'flux',
        }),
      });

      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(
          `InfluxDB HTTP ${response.status} (${response.statusText}): ${errorText}`
        );
      }

      const csvData = await response.text();
      return parseInfluxCsvToEpisodes(csvData);
    } catch (err: unknown) {
      lastError = err instanceof Error ? err : new Error(String(err));
      // Se for erro de rede/conexão (ex: servidor desligado), não adianta tentar o segundo formato de bucket
      if (lastError.message.includes('Network') || lastError.message.includes('Failed to fetch')) {
        break;
      }
    }
  }

  throw lastError || new Error('Falha ao consultar dados no InfluxDB.');
}

