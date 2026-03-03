#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>


// Перечисление для определения типа числа с плавающей точкой
typedef enum {
		type_fl,    // float (32 бита)
		type_d,     // double (64 бита)
		type_ld     // long double (80+ бит)
} Type;

/**
 * Функция для определения типа числа и вывода информации в консоль
 * 
 * number - указатель на число (void* - может указывать на любой тип)
 * type - тип числа из перечисления Type
 * prefix - строка, которая будет выведена перед числом
 * 
 * Используется для отладки - показывает, как число хранится в памяти
 */
void poisknumber(void* number, Type type, const char* prefix) {
		switch(type) {
				case type_fl: {  // Если число типа float
						float f = *(float*)number;  // Разыменовываем указатель как float
						printf("%s float: %f\n", prefix, f);
						break;
				}
				case type_d: {   // Если число типа double
						double d = *(double*)number;  // Разыменовываем как double
						printf("%s double: %lf\n", prefix, d);
						break;
				}
				case type_ld: {  // Если число типа long double
						long double ld = *(long double*)number;  // Разыменовываем как long double
						printf("%s long double: %Lf\n", prefix, ld);
						break;
				}
		}
}

/**
 * Преобразует любое число в строку с его двоичным представлением
 * 
 * num - указатель на число в памяти
 * size - размер типа в байтах (sizeof(float) = 4, sizeof(double) = 8)
 * out - выходная строка, куда будет записан результат
 * 
 * Алгоритм:
 * 1) Интерпретируем память как массив байтов (unsigned char*)
 * 2) Идем по байтам с конца (чтобы получить правильный порядок)
 * 3) Для каждого байта идем по битам от старшего (7) к младшему (0)
 * 4) Извлекаем бит с помощью сдвига и маски & 1
 * 5) Записываем '1' или '0' в выходную строку
 */
void bits_to_string(void* num, size_t size, char* out) {
		unsigned char* bytes = num;  // Приводим void* к unsigned char* для побайтового доступа
		size_t k = 0;                 // Индекс для записи в выходную строку

		// Цикл по байтам с конца (важно для правильного порядка!)
		for (size_t i = size; i > 0; i--) {
				// Цикл по битам в байте от старшего (7) к младшему (0)
				for (int b = 7; b >= 0; b--) {
						// Извлекаем b-й бит: сдвигаем вправо на b и берем младший бит (& 1)
						// Если бит = 1, записываем '1', иначе '0'
						out[k++] = (bytes[i - 1] >> b) & 1 ? '1' : '0';
				}
		}
		out[k] = '\0';  // Завершаем строку нулевым символом
}

/**
 * Форматирует битовую строку в формат (знак | экспонента | мантисса)
 * 
 * bits - исходная битовая строка (например, "01000000010010001111010111000011")
 * total_bits - общее количество бит (32 или 64)
 * out - выходная строка с форматированием
 * 
 * Для float (32 бита): 1 бит знака | 8 бит экспоненты | 23 бита мантиссы
 * Для double (64 бита): 1 бит знака | 11 бит экспоненты | 52 бита мантиссы
 */
void format(const char* bits, int total_bits, char* out) {
		if (total_bits == 32) {  // Для float
				sprintf(out, "%.*s  %.*s  %s",
								1, bits,           // 1 бит знака
								8, bits + 1,       // 8 бит экспоненты (начиная со 2-го бита)
								bits + 9);         // 23 бита мантиссы (оставшиеся биты)
		}
		else if (total_bits == 64) {  // Для double
				sprintf(out, "%.*s  %.*s  %s",
								1, bits,            // 1 бит знака
								11, bits + 1,       // 11 бит экспоненты
								bits + 12);         // 52 бита мантиссы
		}
		else {
				strcpy(out, "long double (зависит от компилятора)");
		}
}


/**
 * a - левая граница диапазона
 * b - правая граница диапазона
 * P - количество знаков после запятой
 * return сгенерированное число
 * 
 * Алгоритм:
 * 1. Генерируем случайное число r в диапазоне [a, b]
 * 2. Вычисляем scale = 10^P для округления до P знаков
 * 3. Умножаем r на scale, округляем, делим обратно на scale
 */
double generate_random(double a, double b, int P) {
		double scale = pow(10, P);                    // Множитель для округления (10^P)
		double r = ((double)rand() / RAND_MAX) * (b - a) + a;  // Случайное число в [a, b]
		return round(r * scale) / scale;              // Округляем до P знаков
}

/**
 * читаю параметры из input.txt:
 * N - количество вариантов
 * K - количество чисел в каждом варианте
 * bits - разрядность чисел (32, 64 или другое для long double)
 * a, b - диапазон генерации чисел
 * P - количество знаков после запятой
 * 
 * Создает:
 * - zadanie/variant_X.md - таблица с исходными числами
 * - proverka/variant_X.md - таблица с числами, их машинным представлением и ошибками
 */
int main(void) {
		FILE *input = fopen("input.txt", "r");
		if (!input) {  // Проверяем, открылся ли файл
				printf("Ошибка открытия input.txt\n");
				return 1;  // Завершаем программу с кодом ошибки
		}

		int N, K, bits, P;
		double a, b;

		// Читаем параметры из файла
		fscanf(input, "%d%d%d%lf%lf%d",
					 &N, &K, &bits, &a, &b, &P);

		fclose(input);  


		srand((unsigned int)time(NULL));  // Инициализируем генератор случайных чисел текущим временем

		// Создаем директории для выходных файлов (флаг -p создает промежуточные директории)
		system("mkdir -p zadanie");
		system("mkdir -p proverka");

		// Цикл по вариантам (от 1 до N)
		for (int v = 1; v <= N; v++) {
				// Формируем имена выходных файлов
				char file1[100], file2[100];
				sprintf(file1, "zadanie/variant_%d.md", v);
				sprintf(file2, "proverka/variant_%d.md", v);

				// Открываем файлы для записи
				FILE *f1 = fopen(file1, "w");
				FILE *f2 = fopen(file2, "w");

				// Таблица для zadanie (только числа)
				fprintf(f1, "| N | Вещ число |\n");
				fprintf(f1, "|---|------------|\n");

				// Таблица для proverka (числа, машинное представление, ошибки)
				fprintf(f2, "| N | Вещ число | Машинное представление | Ошибка | Проверка типа |\n");
				fprintf(f2, "|---|------------|------------------------|----------|---------------|\n");

				// Цикл по числам (от 1 до K)
				for (int i = 1; i <= K; i++) {
						// Генерируем исходное число
						double original = generate_random(a, b, P);
						double restored = 0;  // Число после преобразования
						double error = 0;      // Ошибка (разница между original и restored)

						char bits_str[200];     // Строка с битовым представлением
						char formatted[300];    // Отформатированное представление
						char type_check[100] = "";  // Строка с информацией о типе

						// ОБРАБОТКА В ЗАВИСИМОСТИ ОТ РАЗРЯДНОСТИ 
						if (bits == 32) {  // Для 32-битных чисел (float)
								// Преобразуем double в float (с потерями точности!)
								float temp = (float)original;
								
								// Получаем битовое представление float
								bits_to_string(&temp, sizeof(float), bits_str);
								
								// Форматируем в нужный нам формат
								format(bits_str, 32, formatted);
								
								// Восстановленное число - это float, преобразованный обратно в double
								restored = temp;
								
								// Вывод в консоль
								poisknumber(&temp, type_fl, "Сгенерировано:");
								
								// Информация о типе для таблицы
								sprintf(type_check, "float (32 бита)");
						}
						else if (bits == 64) {  // Для 64-битных чисел (double)
								// Для double нет преобразования - original и так double
								double temp = original;
								
								// Получаем битовое представление double
								bits_to_string(&temp, sizeof(double), bits_str);
								
								// Форматируем в нужный нам формат
								format(bits_str, 64, formatted);
								
								// Восстановленное число равно исходному
								restored = temp;
								
								// Отладочный вывод в консоль
								poisknumber(&temp, type_d, "Сгенерировано:");
								
								// Информация о типе для таблицы
								sprintf(type_check, "double (64 бита)");
						}
						else {  // Для других разрядностей (long double)
								// Преобразуем в long double
								long double temp = (long double)original;
								
								// Получаем битовое представление long double
								bits_to_string(&temp, sizeof(long double), bits_str);
								
								// Для long double формат зависит от компилятора
								strcpy(formatted, "long double (платформозависимо)");
								
								// Восстановленное число - long double преобразованный обратно в double
								restored = (double)temp;
								
								// Отладочный вывод в консоль
								poisknumber(&temp, type_ld, "Сгенерировано:");
								
								// Информация о типе для таблицы
								sprintf(type_check, "long double (80+ бит)");
						}

						// Ошибка - абсолютная разница между исходным и восстановленным числом
						error = fabs(original - restored);


						// Запись в файл zadanie (только исходные числа)
						fprintf(f1, "| %d | %.*f |\n", i, P, original);

						// Запись в файл proverka (полная информация)  
						// %.10e - ошибка в экспоненциальном формате с 10 знаками после запятой
						fprintf(f2, "| %d | %.*f | `%s` | %.10e | %s |\n",
										i, P, original, formatted, error, type_check);
				}

				fclose(f1);
				fclose(f2);
		}

		printf("Готово. Создано %d файлов.\n", 2*N);  // 2 файла на каждый вариант
		return 0;  // Программа завершена успешно
}