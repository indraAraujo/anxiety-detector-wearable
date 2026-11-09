#ifndef MODEL_WESAD_H
#define MODEL_WESAD_H

#ifdef __cplusplus
extern "C" {
#endif

// Parâmetros de Normalização (StandardScaler)
static const double SCALER_MEAN[4] = { -1.1657827420429709e-17, 55.77662978155673, 0.027666135953868735, 0.03722936226020499 };
static const double SCALER_SCALE[4] = { 8.195896864867729e-16, 43.74784553658487, 0.02074796204522243, 0.03778134695743864 };

/*
  Código C gerado pelo m2cgen para o Linear SVM
*/
double score(double * input) {
    return -0.019593838634049517 + input[0] * -0.001170248998127103 + input[1] * 0.10243574257376287 + input[2] * 0.45507261054406334 + input[3] * -0.020223244750094266;
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
