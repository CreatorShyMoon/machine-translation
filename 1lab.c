#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <mpfr.h>


void vesh_to_bits(void *num, size_t size, char *out);
void format_bits(const char *bits, int total_bits, char *out);
float bits_to_float(const char *bits);
double bits_to_double(const char *bits);
void bits_to_mpfr128(char *bits, mpfr_t rop);
void mpfr_to_bits(mpfr_t x, char *bits);
void generate_random_string(double a, double b, int P, char *buffer, size_t buf_size);


/**
 * Преобразование числа в строку битов старший бит первый
 * num это указатель на число
 * size это размер числа в байтах
 * out это строка, куда записываются биты 
 */
void vesh_to_bits(void *num, size_t size, char *out) {
	unsigned char *bytes = (unsigned char *)num;
	size_t k = 0;
	for (size_t i = size; i > 0; i--) {
		for (int b = 7; b >= 0; b--) {
			out[k++] = (bytes[i-1] >> b) & 1 ? '1' : '0';
		}
	}
}

/**
 * Форматирование битовой строки в вид: знак | порядок | мантисса
 * bits это строка битов (без пробелов)
 * total_bits это общее число бит (32, 64 или 128)
 * out это буфер для отформатированной строки
 */
void format_bits(const char *bits, int total_bits, char *out) {
	if (total_bits == 32) {
		sprintf(out, "%.*s  %.*s  %s", 1, bits, 8, bits+1, bits+9);
	} else if (total_bits == 64) {
		sprintf(out, "%.*s  %.*s  %s", 1, bits, 11, bits+1, bits+12);
	} else if (total_bits == 128) {
		sprintf(out, "%.*s  %.*s  %s", 1, bits, 15, bits+1, bits+16);
	}
}



/**
 * Восстановление 32‑битного float из битовой строки.
 */
float bits_to_float(const char *bits) {
	uint32_t u = 0;
	for (int i = 0; i < 32; i++) {
		u = (u << 1) | (bits[i] == '1');
	}
	float f;
	memcpy(&f, &u, sizeof(f));
	return f;
}

/**
 * Восстановление 64‑битного double из битовой строки.
 */
double bits_to_double(const char *bits) {
	uint64_t u = 0;
	for (int i = 0; i < 64; i++) {
		u = (u << 1) | (bits[i] == '1');
	}
	double d;
	memcpy(&d, &u, sizeof(d));
	return d;
}

/**
 * Восстановление 128‑битного binary128 из битовой строки.
 * Результат помещается в mpfr_t с точностью 113 бит.
 */
void bits_to_mpfr128(char *bits, mpfr_t rop) {
	int sign = (bits[0] == '1') ? 1 : 0;

	// Извлекаем порядок (15 бит)
	int exp = 0;
	for (int i = 0; i < 15; i++) {
		exp = (exp << 1) | (bits[1+i] == '1');
	}
	if (exp == 0x7FFF) {
	    mpfr_set_inf(rop, sign ? -1 : 1);
	    return;
	}
	int biased_exp = exp;
	int real_exp = biased_exp - 16383;
	printf("real_exp = %d\n", real_exp);
	// Мантисса (112 бит) как целое число
	mpz_t mant;

	// Функция mpz_init(число)
	// Инициализирует переменную типа mpz_t (выделяет память для целого числа)
	mpz_init(mant);

	// Функция mpz_set_ui(число, значение)
	// Присваивает беззнаковое целое значение переменной mpz_t
	mpz_set_ui(mant, 0);

	// Последовательно собираем целое значение мантиссы из 112 бит
	for (int i = 0; i < 112; i++) {
		// Функция mpz_mul_2exp(число, число, степень)
		// Умножает число на 2^степень (сдвиг влево на 1 бит)
		mpz_mul_2exp(mant, mant, 1);

		// Функция mpz_add_ui(число, значение)
		// Прибавляет беззнаковое целое значение к mpz_t числу
		if (bits[16 + i] == '1')
			mpz_add_ui(mant, mant, 1);
	}

	// Строим число с плавающей точкой:
	// Функция mpfr_set_z(rop, op, rnd)
	// Преобразует целое число mpz_t в число с плавающей точкой mpfr_t
	mpfr_set_z(rop, mant, MPFR_RNDN);

	// Функция mpfr_div_2ui(rop, op, n, rnd)
	// Делит число на 2^n с указанным режимом округления
	mpfr_div_2ui(rop, rop, 112, MPFR_RNDN);

	// Функция mpfr_add_ui(rop, op, value, rnd)
	// Прибавляет беззнаковое целое значение к числу mpfr_t
	mpfr_add_ui(rop, rop, 1, MPFR_RNDN);

	// Функция mpfr_mul_2exp(rop, op, n, rnd)
	// Умножает число на 2^n с указанным режимом округления
	mpfr_mul_2exp(rop, rop, real_exp, MPFR_RNDN);

	// Функция mpfr_neg(rop, op, rnd)
	// Меняет знак числа (умножает на -1) с указанным режимом округления
	if (sign)
		mpfr_neg(rop, rop, MPFR_RNDN);

	// Функция mpz_clear(число)
	// Освобождает память, занятую переменной mpz_t
	mpz_clear(mant);
}
/**
 * Получение битов binary128 из числа MPFR.
 * Результат записывается в строку bits (должна быть длиной >= 128).
 */
void mpfr_to_bits(mpfr_t x, char *bits) {
    if (mpfr_zero_p(x)) {
        memset(bits, '0', 128);
        return;
    }

    int sign = mpfr_signbit(x);
    bits[0] = sign ? '1' : '0';

    mpfr_t t;
    mpfr_init2(t, 113);
    mpfr_abs(t, x, MPFR_RNDN);

    mpfr_exp_t exp = mpfr_get_exp(t);

    int biased_exp = (int)exp + 16383 - 1;

    if (biased_exp >= 0x7FFF) biased_exp = 0x7FFE;
    if (biased_exp <= 0) biased_exp = 1;

    for (int i = 0; i < 15; i++) {
        bits[1 + i] = (biased_exp >> (14 - i)) & 1 ? '1' : '0';
    }

    mpfr_div_2exp(t, t, exp - 1, MPFR_RNDN);
    mpfr_sub_ui(t, t, 1, MPFR_RNDN);

    for (int i = 0; i < 112; i++) {
        mpfr_mul_2exp(t, t, 1, MPFR_RNDN);
        if (mpfr_cmp_ui(t, 1) >= 0) {
            bits[16 + i] = '1';
            mpfr_sub_ui(t, t, 1, MPFR_RNDN);
        } else {
            bits[16 + i] = '0';
        }
    }

    mpfr_clear(t);
}


/**
 * Генерирует случайное число в диапазоне [a, b] и записывает его в строку
 * с P знаками после десятичной точки (используя округление к ближайшему).
 * buffer это буфер для результата
 * buf_size это размер буфера
 */
void generate_random_string(double a, double b, int P, char *buffer, size_t buf_size) {
	mpfr_t r;
	mpfr_init2(r, 512);               // высокая точность для генерации
	gmp_randstate_t state;
	gmp_randinit_default(state);
	gmp_randseed_ui(state, (unsigned long)time(NULL) + rand());
	mpfr_urandom(r, state, MPFR_RNDN);
	mpfr_mul_d(r, r, b - a, MPFR_RNDN);
	mpfr_add_d(r, r, a, MPFR_RNDN);
	mpfr_sprintf(buffer, "%.*Rf", P, r);
	mpfr_clear(r);
	gmp_randclear(state);
}

int main(void) {
	FILE *input = fopen("input.txt", "r");
	if (!input) {
		printf("Ошибка открытия input.txt\n");
		return 1;
	}

	int N, K, bits, P;
	double a, b;
	if (fscanf(input, "%d%d%d%lf%lf%d", &N, &K, &bits, &a, &b, &P) != 6) {
		printf("Ошибка чтения параметров\n");
		fclose(input);
		return 1;
	}
	fclose(input);

	srand((unsigned int)time(NULL));

	// Создаём папки для результатов
	system("mkdir -p zadanie");
	system("mkdir -p proverka");

	// Инициализируем MPFR с высокой точностью для эталонных вычислений
	mpfr_set_default_prec(512);
	mpfr_set_default_rounding_mode(MPFR_RNDN);

	// Генератор GMP для случайных чисел (используется в generate_random_string)
	gmp_randstate_t gmp_state;
	gmp_randinit_default(gmp_state);
	gmp_randseed_ui(gmp_state, (unsigned long)time(NULL));

	for (int v = 1; v <= N; v++) {
		char file_zadanie[100], file_proverka[100];
		sprintf(file_zadanie, "zadanie/variant_%d.md", v);
		sprintf(file_proverka, "proverka/variant_%d.md", v);

		FILE *fz = fopen(file_zadanie, "w");
		FILE *fp = fopen(file_proverka, "w");
		if (!fz || !fp) {
			if (fz) fclose(fz);
			if (fp) fclose(fp);
			continue;
		}

		// Заголовки таблиц
		fprintf(fz, "| N | Вещественное число |\n|---|-------------------|\n");
		fprintf(fp, "| N | Вещественное число | Машинное представление | Восстановленное число | Ошибка |\n|---|-------------------|------------------------|-----------------------|--------|\n");

		for (int i = 1; i <= K; i++) {
			char original_str[100];
			// Генерируем строку с P знаками 
			generate_random_string(a, b, P, original_str, sizeof(original_str));

			// число с высокой точностью (MPFR, 256 бит)
			// mpfr_t — тип числа произвольной точности
			mpfr_t exact;

			// Функция mpfr_init2(переменная, precision)
			// Инициализирует mpfr_t с заданной точностью в битах
			mpfr_init2(exact, 512);

			// Функция mpfr_set_str(rop, str, base, rnd)
			// Преобразует строку в число mpfr_t
			// base — основание системы счисления (10), rnd — режим округления
			mpfr_set_str(exact, original_str, 10, MPFR_RNDN);

			if (bits == 32) {

				// Функция mpfr_get_d(op, rnd)
				// Преобразует mpfr_t число в double с округлением
				float machine_f = (float)mpfr_get_d(exact, MPFR_RNDN);

				// Битовое представление float
				char bits_str[33];

				// Функция vesh_to_bits(num, size, out)
				// Преобразует бинарное представление числа в строку битов
				vesh_to_bits(&machine_f, sizeof(float), bits_str);

				// Форматирование битовой строки (знак | порядок | мантисса)
				char formatted[64];

				// Функция format_bits(bits, total_bits, out)
				// Разбивает битовую строку на поля IEEE 754
				format_bits(bits_str, 32, formatted);

				// Восстановление float из битов
				// Функция bits_to_float(bits)
				float recovered_f = bits_to_float(bits_str);

				// Ошибка = |exact - recovered_f|
				mpfr_t err;

				// Инициализация mpfr_t числа ошибки
				mpfr_init2(err, 512);

				// Функция mpfr_set_d(rop, op, rnd)
				// Преобразует double в mpfr_t
				mpfr_set_d(err, recovered_f, MPFR_RNDN);

				// Функция mpfr_sub(rop, op1, op2, rnd)
				// Вычисляет rop = op1 − op2
				mpfr_sub(err, exact, err, MPFR_RNDN);

				// Функция mpfr_abs(rop, op, rnd)
				// Вычисляет модуль числа
				mpfr_abs(err, err, MPFR_RNDN);

				char err_str[512];

				// Функция mpfr_sprintf(buffer, format, op)
				// Форматированный вывод mpfr_t в строку
				mpfr_sprintf(err_str, "%.12Re", err);

				// Запись результатов
				fprintf(fz, "| %d | %s |\n", i, original_str);
				fprintf(fp, "| %d | %s | `%s` | %.10g | %s |\n",
						i, original_str, formatted, recovered_f, err_str);

				// Функция mpfr_clear(число)
				// Освобождает память mpfr_t
				mpfr_clear(err);
				mpfr_clear(exact);
			}
			else if (bits == 64) {

				// Преобразование mpfr double
				double machine_d = mpfr_get_d(exact, MPFR_RNDN);

				char bits_str[65];

				vesh_to_bits(&machine_d, sizeof(double), bits_str);

				char formatted[128];
				format_bits(bits_str, 64, formatted);

				// Восстановление double из битов
				double recovered_d = bits_to_double(bits_str);

				mpfr_t err;
				mpfr_init2(err, 512);

				mpfr_set_d(err, recovered_d, MPFR_RNDN);
				mpfr_sub(err, exact, err, MPFR_RNDN);
				mpfr_abs(err, err, MPFR_RNDN);

				char err_str[512];
				mpfr_sprintf(err_str, "%.12Re", err);

				fprintf(fz, "| %d | %s |\n", i, original_str);
				fprintf(fp, "| %d | %s | `%s` | %.15g | %s |\n",
						i, original_str, formatted, recovered_d, err_str);

				mpfr_clear(err);
				mpfr_clear(exact);
			}
			else if (bits == 128) {

				// mpfr_t число с точностью binary128 (113 бит)
				mpfr_t rounded;

				// Инициализация с точностью 113 бит
				mpfr_init2(rounded, 113);

				// Округление эталонного числа до 113 бит
				mpfr_set(rounded, exact, MPFR_RNDN);

				char bits_str[129];

				// Преобразование mpfr битовая строка IEEE 754 binary128
				mpfr_to_bits(rounded, bits_str);

				char formatted[256];
				format_bits(bits_str, 128, formatted);

				mpfr_t recovered;

				// Инициализация восстановленного числа
				mpfr_init2(recovered, 113);

				// Восстановление числа из битовой строки
				bits_to_mpfr128(bits_str, recovered);

				mpfr_t err;
				mpfr_init2(err, 512);

				mpfr_sub(err, exact, recovered, MPFR_RNDN);
				mpfr_abs(err, err, MPFR_RNDN);

				char err_str[512];
				mpfr_sprintf(err_str, "%.12Re", err);

				char rec_str[100];
				mpfr_sprintf(rec_str, "%.*Rf", P, recovered);

				fprintf(fz, "| %d | %s |\n", i, original_str);
				fprintf(fp, "| %d | %s | `%s` | %s | %s |\n",
						i, original_str, formatted, rec_str, err_str);

				mpfr_clear(rounded);
				mpfr_clear(recovered);
				mpfr_clear(err);
				mpfr_clear(exact);
			}
			else {
				fprintf(stderr, "Неподдерживаемый размер битов: %d\n", bits);
				break;
			}
		}

		fclose(fz);
		fclose(fp);
	}

	gmp_randclear(gmp_state);
	printf("Готово. Результаты в папках zadanie/ и proverka/\n");
	return 0;
}