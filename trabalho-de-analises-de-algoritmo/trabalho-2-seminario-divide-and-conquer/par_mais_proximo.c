#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <time.h>

// Estrutura para representar um ponto no plano 2D com coordenadas (x, y)
typedef struct {
    double x, y;
} Point;

// Calcula a distância euclidiana entre dois pontos p1 e p2
double dist(Point p1, Point p2) {
    return sqrt((p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y));
}

// Implementação da solução por força bruta: verifica todas as combinações possíveis e encontra a menor distância
// Complexidade: O(n²)
double closestPairBruteForce(Point points[], int n) {
    double min = DBL_MAX;
    for(int i=0; i<n-1; i++) {
        for(int j=i+1; j<n; j++) {
            double d = dist(points[i], points[j]);
            if(d < min) min = d;
        }
    }
    return min;
}

// Funções de comparação para qsort: ordenar pontos pelo eixo X
int compareX(const void* a, const void* b) {
    Point *p1 = (Point*)a, *p2 = (Point*)b;
    return (p1->x > p2->x) - (p1->x < p2->x);
}

// Funções de comparação para qsort: ordenar pontos pelo eixo Y
int compareY(const void* a, const void* b) {
    Point *p1 = (Point*)a, *p2 = (Point*)b;
    return (p1->y > p2->y) - (p1->y < p2->y);
}

// Função para encontrar a menor distância em uma "faixa" (strip) de pontos ordenados por Y
// Aplica a otimização de verificar número constante de pontos vizinhos por propriedades geométricas
// Complexidade dessa etapa é O(n)
double stripClosest(Point strip[], int size, double d) {
    double min = d;
    // strip já está ordenado por Y, não é necessário ordenar aqui
    for(int i=0; i<size; i++) {
        for(int j=i+1; j<size && (strip[j].y - strip[i].y) < min; j++) {
            double distC = dist(strip[i], strip[j]);
            if(distC < min) min = distC;
        }
    }
    return min;
}

// Função recursiva principal que implementa a técnica de Divisão e Conquista para o problema do Par Mais Próximo
// pointsX: pontos ordenados por X
// pointsY: pontos ordenados por Y
// n: número de pontos no subproblema atual
double closestUtil(Point pointsX[], Point pointsY[], int n) {
    // Caso base: se poucos pontos, use força bruta
    if (n <= 3) return closestPairBruteForce(pointsX, n);

    int mid = n / 2;
    Point midPoint = pointsX[mid];

    // Aloca arrays para dividir os pontos ordenados por Y nas metades esquerda e direita
    Point* pointsYL = malloc(mid * sizeof(Point));
    Point* pointsYR = malloc((n - mid) * sizeof(Point));
    int li = 0, ri = 0;

    // Distribui os pontos em pointsYL e pointsYR garantindo que pointsYL tenha exatamente mid pontos
    for(int i = 0; i < n; i++) {
        int isLeft = (pointsY[i].x < midPoint.x) ||
                     (pointsY[i].x == midPoint.x && pointsY[i].y < midPoint.y);

        if (isLeft && li < mid) {
            pointsYL[li++] = pointsY[i];
        } else {
            pointsYR[ri++] = pointsY[i];
        }
    }

    // Calcula a menor distância na metade esquerda e na metade direita, recursivamente
    double dl = closestUtil(pointsX, pointsYL, mid);
    double dr = closestUtil(pointsX + mid, pointsYR, n - mid);

    // Libera a memória alocada para os subarrays
    free(pointsYL);
    free(pointsYR);

    // Escolhe a menor distância entre as duas metades
    double d = (dl < dr) ? dl : dr;

    // Constroi a faixa (strip) contendo pontos com x próximo ao ponto médio dentro da distância d
    Point* strip = malloc(n * sizeof(Point));
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (fabs(pointsY[i].x - midPoint.x) < d) {
            strip[j++] = pointsY[i];
        }
    }

    // Calcula a menor distância dentro da faixa (strip)
    double stripDist = stripClosest(strip, j, d);
    free(strip);

    // Retorna a menor distância encontrada em todo o domínio
    return (d < stripDist) ? d : stripDist;
}

// Função "wrapper" que prepara os dados e chama a função recursiva
double closest(Point points[], int n) {
    Point* pointsX = malloc(n * sizeof(Point));
    Point* pointsY = malloc(n * sizeof(Point));

    // Copia os pontos originais para dois arrays para ordenação independente
    for(int i = 0; i < n; i++) {
        pointsX[i] = points[i];
        pointsY[i] = points[i];
    }

    // Ordena pontos por X e por Y no início
    qsort(pointsX, n, sizeof(Point), compareX);
    qsort(pointsY, n, sizeof(Point), compareY);

    // Chama a função recursiva principal
    double result = closestUtil(pointsX, pointsY, n);

    // Libera memória temporária
    free(pointsX);
    free(pointsY);

    return result;
}

// Função para rodar experimentos com diferentes tamanhos e medir tempos dos dois métodos
void runExperiments(int sizes[], int length) {
    // Abre arquivo CSV para gravar resultados
    FILE *csv_file = fopen("results.csv", "w");
    if (csv_file == NULL) {
        printf("ERRO: Não foi possível abrir o arquivo results.csv\n");
        return;
    }

    // Cabeçalho do arquivo CSV
    fprintf(csv_file, "n,time_brute,time_dc\n");

    // Loop pelos tamanhos para os testes
    for(int idx = 0; idx < length; idx++) {
        int n = sizes[idx];
        Point* points = malloc(n * sizeof(Point));

        // Gera pontos aleatórios para teste
        printf("\n========== Testando n = %d ==========\n", n);
        printf("Gerando %d pontos aleatórios:\n", n);
        for(int i = 0; i < n; i++) {
            points[i].x = (double)rand() / RAND_MAX * 10000.0;
            points[i].y = (double)rand() / RAND_MAX * 10000.0;
            if(i < 10)
                printf("Ponto %d: (%.2f, %.2f)\n", i+1, points[i].x, points[i].y);
        }

        // Executa força bruta e mede o tempo
        clock_t start_t = clock();
        double minDistBrute = closestPairBruteForce(points, n);
        clock_t end_t = clock();
        double time_brute = ((double)(end_t - start_t)) / CLOCKS_PER_SEC;

        printf("\nMenor distancia (Força Bruta): %.6f\n", minDistBrute);
        printf("Tempo Força Bruta: %f segundos\n", time_brute);

        // Executa divisão e conquista e mede o tempo
        start_t = clock();
        double minDistDC = closest(points, n);
        end_t = clock();
        double time_dc = ((double)(end_t - start_t)) / CLOCKS_PER_SEC;

        printf("\nMenor distancia (Divide and Conquer): %.6f\n", minDistDC);
        printf("Tempo Divide and Conquer: %f segundos\n", time_dc);

        // Grava o resultado no arquivo CSV
        fprintf(csv_file, "%d,%f,%f\n", n, time_brute, time_dc);

        // Libera a memória do vetor de pontos
        free(points);
    }

    // Fecha o arquivo CSV depois dos testes
    fclose(csv_file);

    printf("\n========================================\n");
    printf("Resultados dos testes salvos em 'results.csv'\n");
    printf("========================================\n");
}

// Função principal que inicia os experimentos com uma lista variada de tamanhos
int main() {
    srand(time(NULL));
    int test_sizes[] = {
        100, 200, 300, 400, 500, 600, 700, 800, 900,
        1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
        10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
        100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
        1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000
    };
    int length = sizeof(test_sizes) / sizeof(test_sizes[0]);

    runExperiments(test_sizes, length);

    return 0;
}
