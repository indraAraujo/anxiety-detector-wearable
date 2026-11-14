import {
  AnxietyEpisode,
  ChartDataPoint,
} from '@/types/anxietyDetections';
import {
  fetchAnxietyDataFromInflux
} from '@/services/influxService';
import { getDeviceId } from '@/utils/onboarding';
import { useCallback, useRef, useState } from 'react';
import { useFocusEffect } from 'expo-router';

export interface AnxietyMetrics {
  lastBpm: number;
  peakBpm: number;
  avgBpm: number;
  totalEpisodes: number;
}

export interface UseAnxietyDataResult {
  episodes: AnxietyEpisode[];
  chartData: ChartDataPoint[];
  metrics: AnxietyMetrics;
  deviceId: string | null;
  loading: boolean;
  error: string | null;
  isMock: boolean;
  refetch: () => Promise<void>;
}

const DAYS = ['Dom', 'Seg', 'Ter', 'Qua', 'Qui', 'Sex', 'Sáb'] as const;

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
 * Agrupa os episódios dos últimos 7 dias nos 7 dias da semana (Dom a Sáb).
 * Para dias com múltiplos episódios de ansiedade, seleciona o pico de BPM.
 */
function buildWeeklyChart(episodes: AnxietyEpisode[]): ChartDataPoint[] {
  return DAYS.map((dayLabel, idx) => {
    const dayEpisodes = episodes.filter((ep) => ep.dayLabel === dayLabel);

    if (dayEpisodes.length > 0) {
      // Pega o episódio com o maior BPM (pico de ansiedade do dia)
      const peak = dayEpisodes.reduce((prev, curr) =>
        curr.bpm > prev.bpm ? curr : prev
      );

      return {
        value: peak.bpm,
        label: dayLabel,
        dataPointText: `${peak.bpm}`,
        episode: peak,
      };
    }

    // Dia sem episódio de ansiedade registrado
    return {
      value: 60,
      label: dayLabel,
      dataPointText: '',
      episode: {
        id: `empty-${dayLabel}`,
        dayLabel,
        dayFull: DAY_FULL_NAMES[idx],
        time: '--:--',
        bpm: 0,
        intensity: 'Leve',
        context: 'Nenhum episódio de ansiedade detectado neste dia',
      },
    };
  });
}

/**
 * Calcula métricas resumidas da semana.
 */
function calculateMetrics(episodes: AnxietyEpisode[]): AnxietyMetrics {
  if (episodes.length === 0) {
    return { lastBpm: 0, peakBpm: 0, avgBpm: 0, totalEpisodes: 0 };
  }

  const bpms = episodes.map((e) => e.bpm).filter((b) => b > 0);
  if (bpms.length === 0) {
    return { lastBpm: 0, peakBpm: 0, avgBpm: 0, totalEpisodes: 0 };
  }

  const lastBpm = bpms[bpms.length - 1];
  const peakBpm = Math.max(...bpms);
  const avgBpm = Math.round(bpms.reduce((acc, curr) => acc + curr, 0) / bpms.length);

  return {
    lastBpm,
    peakBpm,
    avgBpm,
    totalEpisodes: episodes.length,
  };
}

/**
 * Hook para leitura dos dados de ansiedade (BPM) da pulseira a partir do InfluxDB.
 *
 * - Lê o device_id armazenado no cache do onboarding.
 * - Filtra registros dos últimos 7 dias onde device_id corresponde à pulseira.
 * - Transforma os registros em pontos para o gráfico semanal (Domingo a Sábado, 50 a 180 BPM).
 */
export function useAnxietyData(): UseAnxietyDataResult {
  const [deviceId, setDeviceId] = useState<string | null>(null);
  const [episodes, setEpisodes] = useState<AnxietyEpisode[]>([]);
  const [chartData, setChartData] = useState<ChartDataPoint[]>([]);
  const [metrics, setMetrics] = useState<AnxietyMetrics>({
    lastBpm: 0,
    peakBpm: 0,
    avgBpm: 0,
    totalEpisodes: 0,
  });
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [isMock, setIsMock] = useState(false);
  const isMounted = useRef(true);

  const loadData = useCallback(async () => {
    setLoading(true);
    setError(null);

    try {
      // 1. Obtém o device_id da pulseira gravado no onboarding
      const storedDeviceId = await getDeviceId();
      if (!isMounted.current) return;
      setDeviceId(storedDeviceId);

      if (!storedDeviceId) {
        setEpisodes([]);
        setChartData([]);
        setError('Nenhum código de pulseira cadastrado no onboarding.');
        setLoading(false);
        return;
      }

      // 2. Consulta o InfluxDB pelo bucket e device_id nos últimos 7 dias
      const realEpisodes = await fetchAnxietyDataFromInflux(storedDeviceId);
      if (!isMounted.current) return;

      if (realEpisodes.length > 0) {
        setIsMock(false);
        setEpisodes(realEpisodes);
        setChartData(buildWeeklyChart(realEpisodes));
        setMetrics(calculateMetrics(realEpisodes));
      } else {
        setIsMock(false);
        setEpisodes([]);
        setChartData(buildWeeklyChart([]));
        setMetrics({ lastBpm: 0, peakBpm: 0, avgBpm: 0, totalEpisodes: 0 });
      }
    } catch (err: unknown) {
      if (!isMounted.current) return;
      const message = err instanceof Error ? err.message : String(err);
      setError(message);
    } finally {
      if (isMounted.current) {
        setLoading(false);
      }
    }
  }, []);

  useFocusEffect(
    useCallback(() => {
      isMounted.current = true;
      loadData();

      return () => {
        isMounted.current = false;
      };
    }, [loadData])
  );

  return {
    episodes,
    chartData,
    metrics,
    deviceId,
    loading,
    error,
    isMock,
    refetch: loadData,
  };
}

