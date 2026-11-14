export interface AnxietyEpisode {
    id: string;
    dayLabel: 'Dom' | 'Seg' | 'Ter' | 'Qua' | 'Qui' | 'Sex' | 'Sáb';
    dayFull: string;
    time: string;
    bpm: number;
    intensity: 'Leve' | 'Moderada' | 'Alta';
    context: string;
}

export interface ChartDataPoint {
    value: number;
    label: string;
    dataPointText: string;
    episode: AnxietyEpisode;
}