#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <wait.h>

int main() {
    int pipe_channels[2];
    int target_num, current_guess;
    int lower, upper;
    int total_rounds = 10;
    int round_counter = 0;

    printf("Укажите верхнюю границу диапазона (целое число больше 1): ");
    scanf("%d", &upper);
    
    if (upper <= 1) {
        printf("Неверный ввод: верхняя граница должна превышать 1!\n");
        return EXIT_FAILURE;
    }

    if (pipe(pipe_channels) == -1) {
        perror("Ошибка создания канала");
        exit(EXIT_FAILURE);
    }

    while (round_counter < total_rounds) {
        if (fork() == 0) {
            close(pipe_channels[0]); 
            srand(time(NULL) + round_counter);
            target_num = rand() % upper + 1;
            write(pipe_channels[1], &target_num, sizeof(target_num));
            printf("Участник 1 выбрал число от 1 до %d: %d\n", upper, target_num);
            close(pipe_channels[1]);
            exit(EXIT_SUCCESS);
        }

        wait(NULL);

        if (fork() == 0) {
            close(pipe_channels[1]);
            read(pipe_channels[0], &target_num, sizeof(target_num));
            printf("Участник 1 загадал число. Участник 2, начинает угадывать\n");

            lower = 1;
            int current_upper = upper;
            int tries = 0;

            while (1) {
                current_guess = lower + (current_upper - lower) / 2;
                printf("Попытка Участника 2: %d\n", current_guess);
                tries++;

                if (current_guess == target_num) {
                    printf(" Участник 2 верно угадал число %d за %d попыток!\n", current_guess, tries);
                    break;
                } else if (current_guess < target_num) {
                    printf(" Предположение слишком маленькое.\n");
                    lower = current_guess + 1;
                } else {
                    printf(" Предположение слишком большое.\n");
                    current_upper = current_guess - 1;
                }
                
                if (lower > current_upper) {
                    printf(" Участник 2 не смог определить число\n");
                    break;
                }
            }
            
            close(pipe_channels[0]);
            exit(EXIT_SUCCESS);
        }

        wait(NULL);
        round_counter++;

        if (round_counter >= total_rounds) break;

        printf("\n--- Этап %d завершён! Смена ролей участников. ---\n", round_counter);

        pipe(pipe_channels);

        if (fork() == 0) {
            close(pipe_channels[0]);
            srand(time(NULL) + round_counter);
            target_num = rand() % upper + 1;
            write(pipe_channels[1], &target_num, sizeof(target_num));
            printf("Участник 2 выбрал число от 1 до %d: %d\n", upper, target_num);
            close(pipe_channels[1]);
            exit(EXIT_SUCCESS);
        }

        wait(NULL);

        if (fork() == 0) {
            close(pipe_channels[1]);
            read(pipe_channels[0], &target_num, sizeof(target_num));
            printf("Участник 2 загадал число. Участник 1, начинает угадывать\n");

            lower = 1;
            int current_upper = upper;
            int tries = 0;

            while (1) {
                current_guess = lower + (current_upper - lower) / 2;
                printf("Попытка Участника 1: %d\n", current_guess);
                tries++;

                if (current_guess == target_num) {
                    printf(" Участник 1 верно угадал число %d за %d попыток!\n", current_guess, tries);
                    break;
                } else if (current_guess < target_num) {
                    printf(" Предположение слишком маленькое.\n");
                    lower = current_guess + 1;
                } else {
                    printf(" Предположение слишком большое.\n");
                    current_upper = current_guess - 1;
                }

                if (lower > current_upper) {
                    printf(" Участник 1 не смог определить число\n");
                    break;
                }
            }
            
            close(pipe_channels[0]);
            exit(EXIT_SUCCESS);
        }

        wait(NULL);
    }

    printf(" Игра завершена! Проведено %d этапов в диапазоне 1-%d.\n", total_rounds, upper);
    return EXIT_SUCCESS;
}
