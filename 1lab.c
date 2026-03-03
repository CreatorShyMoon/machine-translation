#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <mpfr.h>


typedef enum {
		type_fl,  // float (32 бита)
		type_d,   // double (64 бита)
		type_ld   // long double (обычно 80 или 128 бит)
} Type;

/**
 * Функция для автоматического определения типа числа по размеру и вывода его значения
 * number - указатель на число
 * size - размер числа в байтах
 * prefix - строка, выводимая перед числом
 */
void poisknumber_auto(void* number, size_t size, const char* prefix) {
		if (size == sizeof(float)) {  // если размер совпадает с float
				float f = *(float*)number;  // разыменовываем указатель как float
				printf("%s float: %f\n", prefix, f);  // выводим значение
		} else if (size == sizeof(double)) {  // если double
				double d = *(double*)number;
				printf("%s double: %lf\n", prefix, d);
		} else if (size == sizeof(long double)) {  // если long double
				long double ld = *(long double*)number;
				printf("%s long double: %Lf\n", prefix, ld);
		} else {
				printf("%s неизвестный тип (size = %zu)\n", prefix, size); // если неизвестный размер
		}
}

/**
 * Преобразование числа в строку битов (для визуализации)
 * num - указатель на число
 * size - размер числа в байтах
 * out - строка, куда записываются биты 
 */
void bits_to_string(void* num, size_t size, char* out) {
		unsigned char* bytes = (unsigned char*)num; // представляем число как массив байтов
		size_t k = 0;  // индекс записи в строку

		for (size_t i = size; i > 0; i--) { // идём по байтам с конца, старший байт первый
				for (int b = 7; b >= 0; b--) { // по битам внутри байта, старший бит первый
						out[k++] = (bytes[i - 1] >> b) & 1 ? '1' : '0';
				}
		}
		out[k] = '\0'; // завершаем строку
}

/**
 * Форматирование битовой строки в привычный вид: знак | порядок | мантисса
 * bits - строка с битами
 * total_bits - разрядность числа
 * out - строка результата
 */
void format_bits(const char* bits, int total_bits, char* out) {
		if (total_bits == 32) 
		{ // float 1|8|23
				sprintf(out, "%.*s  %.*s  %s", 1, bits, 8, bits + 1, bits + 9);
		} 
		else if (total_bits == 64) 
		{ // double 1|11|52
				sprintf(out, "%.*s  %.*s  %s", 1, bits, 11, bits + 1, bits + 12);
		} 
		else if (total_bits == 128) 
		{ // binary128 1|15|112
				sprintf(out, "%.*s  %.*s  %s", 1, bits, 15, bits + 1, bits + 16);
		} 
		else {
				strcpy(out, "Неподдерживаемый формат");
		}
}

/*
 * Генерация случайного числа в диапазоне [a, b] с округлением до P знаков
 * buffer - сюда записывается результат в виде строки
 */
void generate_random_string(double a, double b, int P, char *buffer, size_t buf_size) {
		double r = (double)rand() / RAND_MAX; // случайное число в [0,1]
		r = a + (b - a) * r; // переносим в диапазон [a, b]
		double scale = pow(10.0, P); // множитель для округления
		r = round(r * scale) / scale; // округляем до P знаков
		snprintf(buffer, buf_size, "%.*f", P, r); // сохраняем в строку
}
// Функция mpfr_to_ieee128_bits(число MPFR, строка bits)
// Переводит число mpfr_t в строковое представление 128-битного IEEE (binary128)
void mpfr_to_ieee128_bits(mpfr_t x, char *bits) {
		if (mpfr_zero_p(x)) {                 // Функция mpfr_zero_p(x) проверяет, равно ли число нулю
				memset(bits, '0', 128);           // Если ноль, все биты равны 0
				bits[128] = '\0';                 // Завершаем C-строку
				return;
		}

		int sign = mpfr_signbit(x);           // Функция mpfr_signbit(x) возвращает 1, если число отрицательное, 0 — если положительное
		bits[0] = sign ? '1' : '0';           // Первый бит — знак

		mpfr_exp_t exp;                        // Переменная для хранения порядка
		mpz_t mantissa;                        // Целая часть мантиссы
		mpz_init(mantissa);                    

		mpfr_get_z_2exp(mantissa, x);          // Функция mpfr_get_z_2exp(mantissa, x) извлекает мантиссу и порядок: x = mantissa * 2^exp

		size_t mant_bits = mpz_sizeinbase(mantissa, 2); // mpz_sizeinbase — количество бит в мантиссе
		int biased_exp = exp - (int)mant_bits + 1 + 16383; // смещённый порядок (смещение 16383 для binary128)

		// Записываем 15 бит порядка
		for (int i = 0; i < 15; i++) {
				bits[1 + i] = (biased_exp >> (14 - i)) & 1 ? '1' : '0';
		}

		mpz_t mant_frac, mask;
		mpz_init(mant_frac);
		mpz_init(mask);

		mpz_set_ui(mask, 1);
		mpz_mul_2exp(mask, mask, 112); // маска для 112 бит мантиссы
		mpz_sub_ui(mask, mask, 1);

		if (mant_bits > 113) {
				mpz_tdiv_q_2exp(mant_frac, mantissa, mant_bits - 113); // усечение лишних бит
		} else {
				mpz_mul_2exp(mant_frac, mantissa, 112 - (mant_bits - 1)); // выравнивание в младшие 112 бит
		}
		mpz_and(mant_frac, mant_frac, mask); // применяем маску

		char *mant_str = mpz_get_str(NULL, 2, mant_frac); // преобразуем мантиссу в строку битов
		int mant_len = strlen(mant_str);
		int pos = 16; // после знака и порядка
		for (int i = 0; i < 112 - mant_len; i++) bits[pos++] = '0'; // дополняем ведущими нулями
		for (int i = 0; i < mant_len; i++) bits[pos++] = mant_str[i]; // копируем мантиссу
		bits[128] = '\0';

		mpz_clear(mantissa);   // очищаем память
		mpz_clear(mant_frac);
		mpz_clear(mask);
		free(mant_str);
}

int main(void) {
		// Открытие файла input.txt для чтения параметров
		FILE *input = fopen("input.txt", "r");
		if (!input) { // проверка успешного открытия
				printf("Ошибка открытия input.txt\n");
				return 1;
		}

		int N, K, bits, P;
		double a, b;

		// Считываем 6 параметров из файла
		if (fscanf(input, "%d%d%d%lf%lf%d", &N, &K, &bits, &a, &b, &P) != 6) {
				printf("Ошибка чтения параметров\n");
				fclose(input);
				return 1;
		}
		fclose(input); // файл больше не нужен

		srand((unsigned int)time(NULL)); // инициализация генератора rand()
		system("mkdir -p zadanie"); // создаём папку для заданий
		system("mkdir -p proverka"); // создаём папку для проверки

		mpfr_set_default_prec(113); // MPFR: точность для binary128 (113 бит)
		mpfr_set_default_rounding_mode(MPFR_RNDN); // MPFR: округление к ближайшему

		gmp_randstate_t state;
		gmp_randinit_default(state); // GMP: инициализация генератора
		gmp_randseed_ui(state, time(NULL)); // GMP: засеваем генератор временем

		// Цикл по вариантам
		for (int v = 1; v <= N; v++) {
				char file1[100], file2[100];
				sprintf(file1, "zadanie/variant_%d.md", v); 
				sprintf(file2, "proverka/variant_%d.md", v); 

				FILE *f1 = fopen(file1, "w");
				FILE *f2 = fopen(file2, "w");
				if (!f1 || !f2) continue; // если файлы не открылось, переходим к следующему варианту

				// Заголовки таблиц
				fprintf(f1, "| N | Вещ число |\n|---|------------|\n");
				fprintf(f2, "| N | Вещ число | Машинное представление | Ошибка |\n|---|------------|------------------------|----------|\n");

				// Генерация чисел
				for (int i = 1; i <= K; i++) {
						char original_str[100];

						// Генерация числа для обычной разрядности
						if (bits != 128) {
								generate_random_string(a, b, P, original_str, sizeof(original_str));
						} else { // для binary128
								mpfr_t r;
								mpfr_init2(r, 113); // Функция mpfr_init2(переменная, точность) выделяет память и задаёт точность числа)
								// r = случайное число в [0,1]
								mpfr_urandom(r, state, MPFR_RNDN); // Функция mpfr_urandom(результат, генератор, режим округления)
								// масштабируем на диапазон
								mpfr_mul_d(r, r, b - a, MPFR_RNDN);  // mpfr_mul_d(результат, число, множитель, режим округления)
								mpfr_add_d(r, r, a, MPFR_RNDN);     // mpfr_add_d(результат, число, слагаемое, режим округления)
								// переводим в строку
								mpfr_sprintf(original_str, "%.*Rf", P, r); // mpfr_sprintf(буфер, формат, число)
								mpfr_clear(r); // очищаем память
						}

						// 32
						if (bits == 32) {
								float temp = (float)atof(original_str); // перевод строки в float
								char bits_str[200];
								bits_to_string(&temp, sizeof(float), bits_str); // получаем биты
								char formatted[300];
								format_bits(bits_str, 32, formatted); // форматируем биты
								double error = fabs(atof(original_str) - temp); // абсолютная ошибка
								fprintf(f1, "| %d | %s |\n", i, original_str);
								fprintf(f2, "| %d | %s | `%s` | %.12e |\n", i, original_str, formatted, error);
								poisknumber_auto(&temp, sizeof(temp), "32-bit"); // выводим тип и значение
						}
						// 64
						else if (bits == 64) {
								// Создаём переменную MPFR для "точного" числа с высокой точностью (256 бит)
								mpfr_t high_prec_mpfr;
								mpfr_init2(high_prec_mpfr, 256); // void mpfr_init2(mpfr_t rop, mpfr_prec_t prec) { инициализация MPFR с prec бит }

								// Генерация случайного числа в [a, b]
								mpfr_t r;
								mpfr_init2(r, 256); // инициализация переменной r для генерации
								mpfr_urandomb(r, state);           // void mpfr_urandomb(mpfr_t rop, gmp_randstate_t state) { rop = [0,1) }
								mpfr_mul_d(r, r, b - a, MPFR_RNDN); // int mpfr_mul_d(mpfr_t rop, const mpfr_t op1, double op2, mpfr_rnd_t rnd) { rop = op1 * op2 }
								mpfr_add_d(r, r, a, MPFR_RNDN);     // int mpfr_add_d(mpfr_t rop, const mpfr_t op1, double op2, mpfr_rnd_t rnd) { rop = op1 + op2 }

								mpfr_set(high_prec_mpfr, r, MPFR_RNDN); // int mpfr_set(mpfr_t rop, const mpfr_t op, mpfr_rnd_t rnd) { rop = op }
								mpfr_clear(r);                           // void mpfr_clear(mpfr_t rop) { освобождает память }

								// Конвертируем "точное" число в строку для записи в файл
								char original_str[100];
								mpfr_sprintf(original_str, "%.*Re", P, high_prec_mpfr); // int mpfr_sprintf(char *str, const char *format, ...) { запись числа MPFR в строку }

								// Переводим в double для машинного представления
								double temp = mpfr_get_d(high_prec_mpfr, MPFR_RNDN); // double mpfr_get_d(const mpfr_t op, mpfr_rnd_t rnd) { возвращает double }

								// Получаем битовое представление double
								char bits_str[200];
								bits_to_string(&temp, sizeof(double), bits_str); // пользовательская функция
								char formatted[300];
								format_bits(bits_str, 64, formatted);           // пользовательская функция

								// Вычисляем абсолютную ошибку: high_prec_mpfr - temp
								mpfr_t error_mpfr;
								mpfr_init2(error_mpfr, 256);                  // инициализация переменной для ошибки
								mpfr_set_d(error_mpfr, temp, MPFR_RNDN);     // int mpfr_set_d(mpfr_t rop, double op, mpfr_rnd_t rnd) { rop = op }
								mpfr_sub(error_mpfr, high_prec_mpfr, error_mpfr, MPFR_RNDN); // int mpfr_sub(mpfr_t rop, const mpfr_t op1, const mpfr_t op2, mpfr_rnd_t rnd) { rop = op1 - op2 }

								char error_str[100];
								mpfr_sprintf(error_str, "%.12Re", error_mpfr); // запись ошибки в строку

								fprintf(f1, "| %d | %s |\n", i, original_str);                         // исходное число
								fprintf(f2, "| %d | %s | `%s` | %s |\n", i, original_str, formatted, error_str); // с машинным представлением и ошибкой

								mpfr_clear(high_prec_mpfr);
								mpfr_clear(error_mpfr);

								// Вывод типа и значения
								poisknumber_auto(&temp, sizeof(temp), "64-bit");
						}
						// 128
						else if (bits == 128) {
								mpfr_t original_mpfr, rounded_mpfr, diff;
								mpfr_init2(original_mpfr, 256); /* Функция mpfr_init2(переменная, точность) —  
								создаёт число MPFR с высокой точностью для "точного" значения */ 
								mpfr_init2(rounded_mpfr, 113);  /* Функция mpfr_init2(переменная, точность) — 
								создаёт число MPFR с точностью binary128 (113 бит) */
								mpfr_init2(diff, 256); /* Функция mpfr_init2(переменная, точность) — 
								создаёт число MPFR для хранения разницы с высокой точностью */


								mpfr_set_str(original_mpfr, original_str, 10, MPFR_RNDN); /* Функция mpfr_set_str(результат, строка, основание, режим округления) — 
								перевод строки в число MPFR */
								mpfr_set(rounded_mpfr, original_mpfr, MPFR_RNDN); /* Функция mpfr_set(результат, источник, режим округления) — 
								округляет число до precision=113 бит */

								char bits_str[129];
								mpfr_to_ieee128_bits(rounded_mpfr, bits_str); /* Функция mpfr_to_ieee128_bits(число, строка) — 
								получает битовое представление числа в виде строки */ 
								char formatted[300];
								format_bits(bits_str, 128, formatted); /* Функция format_bits(строка битов, разрядность, выход) — 
								форматирует строку: знак | порядок | мантисса
*/

								mpfr_sub(diff, original_mpfr, rounded_mpfr, MPFR_RNDN); /*Функция mpfr_sub(результат, число1, число2, режим округления) — 
								вычисляет разницу точного и округлённого числа*/
								char err_str[64];
								mpfr_sprintf(err_str, "%.12Re", diff); /*// Функция mpfr_sprintf(буфер, формат, число) — 
								переводит MPFR число в строку с экспоненциальным форматом
*/

								fprintf(f1, "| %d | %s |\n", i, original_str);
								fprintf(f2, "| %d | %s | `%s` | %s |\n", i, original_str, formatted, err_str);

								mpfr_clear(original_mpfr);
								mpfr_clear(rounded_mpfr);
								mpfr_clear(diff);
						}
				}

				fclose(f1);
				fclose(f2);
		}

		gmp_randclear(state); 
		printf("Готово. Результаты в папках zadanie/ и proverka/\n");

		return 0;
}