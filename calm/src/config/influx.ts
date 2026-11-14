export const INFLUX_CONFIG = {
  /**
   * URL do servidor InfluxDB (local ou nuvem).
   */
  url: process.env.EXPO_PUBLIC_INFLUX_URL || 'http://localhost:8086',

  /**
   * Token de autorização com permissão de leitura no bucket.
   */
  token: process.env.EXPO_PUBLIC_INFLUX_TOKEN || '',

  /**
   * Nome ou ID da Organização no InfluxDB.
   */
  org: process.env.EXPO_PUBLIC_INFLUX_ORG || '',

  /**
   * ID do bucket informado para leitura dos dados da pulseira.
   */
  bucketId: process.env.EXPO_PUBLIC_INFLUX_BUCKET_ID || '',
};

