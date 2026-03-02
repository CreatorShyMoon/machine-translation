#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ---------- Перевод в битовую строку ----------
void bits_to_string(void* num, size_t size, char* out) {
    unsigned char* bytes = num;
    size_t k = 0;

    for (size_t i = size; i > 0; i--) {
        for (int b = 7; b >= 0; b--) {
            out[k++] = (bytes[i - 1] >> b) & 1 ? '1' : '0';
        }
    }
    out[k] = '\0';
}


void format_ieee(const char* bits, int total_bits, char* out) {
		if (total_bits == 32) {
				sprintf(out, "%.*s | %.*s | %s",
								1, bits,
								8, bits + 1,
								bits + 9);
		}
		else if (total_bits == 64) {
				sprintf(out, "%.*s | %.*s | %s",
								1, bits,
								11, bits + 1,
								bits + 12);
		}
		else {
				strcpy(out, "long double (зависит от компилятора)");
		}
}

// ---------- Генерация числа ----------
double generate_random(double a, double b, int P) {
		double scale = pow(10, P);
		double r = ((double)rand() / RAND_MAX) * (b - a) + a;
		return round(r * scale) / scale;
}


int main() {

		FILE *input = fopen("input.txt", "r");
		if (!input) {
				printf("Ошибка открытия input.txt\n");
				return 1;
		}

		int N, K, bits, P;
		double a, b;

		fscanf(input, "%d%d%d%lf%lf%d",
					 &N, &K, &bits, &a, &b, &P);

		fclose(input);

		srand((unsigned int)time(NULL));

		// Создание каталогов 
		system("mkdir -p zadanie");
		system("mkdir -p proverka");

		for (int v = 1; v <= N; v++) {

				char file1[100], file2[100];
				sprintf(file1, "zadanie/variant_%d.md", v);
				sprintf(file2, "proverka/variant_%d.md", v);

				FILE *f1 = fopen(file1, "w");
				FILE *f2 = fopen(file2, "w");

				fprintf(f1, "| N | Вещ число |\n");
				fprintf(f1, "|---|------------|\n");

				fprintf(f2, "| N | Вещ число | Машинное представление | Ошибка |\n");
				fprintf(f2, "|---|------------|------------------------|----------|\n");

				for (int i = 1; i <= K; i++) {

						double original = generate_random(a, b, P);
						double restored = 0;
						double error = 0;

						char bits_str[200];
						char formatted[300];

						if (bits == 32) {
								float temp = (float)original;
								bits_to_string(&temp, sizeof(float), bits_str);
								format_ieee(bits_str, 32, formatted);
								restored = temp;
						}
						else if (bits == 64) {
								double temp = original;
								bits_to_string(&temp, sizeof(double), bits_str);
								format_ieee(bits_str, 64, formatted);
								restored = temp;
						}
						else {
								long double temp = (long double)original;
								bits_to_string(&temp, sizeof(long double), bits_str);
								strcpy(formatted, "long double (платформозависимо)");
								restored = (double)temp;
						}

						error = fabs(original - restored);

						fprintf(f1, "| %d | %.*f |\n", i, P, original);

						fprintf(f2, "| %d | %.*f / `%s` / %.10e |\n",
										i, P, original, formatted, error);
				}

				fclose(f1);
				fclose(f2);
		}

		printf("Готово. Создано %d файлов.\n", 2*N);
		return 0;
}
