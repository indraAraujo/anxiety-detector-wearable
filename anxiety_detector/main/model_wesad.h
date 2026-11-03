#ifndef MODEL_WESAD_H
#define MODEL_WESAD_H

#ifdef __cplusplus
extern "C" {
#endif

// Parâmetros de Normalização (StandardScaler)
static const double SCALER_MEAN[4] = { 0.005881020341207343, 55.77662978155673, 63.619473170535095, 2.760968410593984 };
static const double SCALER_SCALE[4] = { 0.4479428851870975, 43.74784553658487, 0.8788102727967684, 2.8075322148187922 };

/*
  Código C gerado pelo m2cgen para o Linear SVM
*/
double score(double * input) {
    return -0.02918568638935162 + input[0] * 0.001058647250838142 + input[1] * 0.10632457380820085 + input[2] * -0.045625050408816405 + input[3] * 0.36581299498966957;
}


/*
  Função utilitária que aplica a normalização e executa a inferência
  Entrada: double features[4] -> [ppg_mean, ppg_std, acc_mean, acc_std]
  Retorno: int (0 = Repouso, 1 = Estresse)
*/
static inline int predict_wesad_stress(const double features[4]) {
    double normalized_input[4];
    for (int i = 0; i < 4; i++) {
        normalized_input[i] = (features[i] - SCALER_MEAN[i]) / SCALER_SCALE[i];
    }
    double res = score(normalized_input);
    return (res > 0.0) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif // MODEL_WESAD_H
